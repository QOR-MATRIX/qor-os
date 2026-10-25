// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "shellscript.h"

#include "gamepadmanager.h"
#include "keycodes.h"
#include "services.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtGui/QKeySequence>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <qpa/qwindowsysteminterface.h>

#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <optional>

using namespace Qt::StringLiterals;

namespace qtos {

namespace {

// Linux evdev codes; Wayland clients receive keys by scan code (evdev + 8).
QJsonArray rectJson(const QRectF &r) { return { qRound(r.x()), qRound(r.y()), qRound(r.width()), qRound(r.height()) }; }

QJsonArray rectJson(const QVariant &value) { return rectJson(value.toRectF()); }

class ShellScript : public QObject
{
public:
    ShellScript(QQuickWindow *window, const QJsonArray &steps, const QString &baseDir)
        : QObject(window), m_window(window), m_steps(steps), m_baseDir(baseDir)
    {
        m_clock.start();
        m_desktop = window->findChild<QObject *>(u"desktop"_s);
        m_engine = m_desktop ? qmlEngine(m_desktop) : nullptr;
    }

    void start()
    {
        QTimer::singleShot(0, this, [this] { pump(); });
    }

private:
    // An action returns the delay in ms before the next one, or -1 on failure.
    using Action = std::function<int()>;

    void pump()
    {
        if (m_queue.empty()) {
            if (m_next >= m_steps.size()) {
                std::fputs("script: done\n", stderr);
                QCoreApplication::exit(0);
                return;
            }
            const QJsonObject step = m_steps.at(m_next++).toObject();
            if (!expand(step)) {
                fail(step);
                return;
            }
        }
        Action action = std::move(m_queue.front());
        m_queue.pop_front();
        const int delay = action();
        if (delay < 0) {
            fail(m_steps.at(m_next - 1).toObject());
            return;
        }
        QTimer::singleShot(delay, this, [this] { pump(); });
    }

    void fail(const QJsonObject &step)
    {
        const QByteArray json = QJsonDocument(step).toJson(QJsonDocument::Compact);
        std::fprintf(stderr, "script: step %lld %s failed: %s\n", static_cast<long long>(m_next), json.constData(),
                     qPrintable(m_error));
        QCoreApplication::exit(4);
    }

    QString path(const QString &relative) const { return QDir(m_baseDir).absoluteFilePath(relative); }

    // ── Shell access ────────────────────────────────────────────────────────

    QJSValue js(QObject *object) const { return m_engine->toScriptValue(object); }

    QObject *layer() const { return m_desktop ? m_desktop->property("windowLayer").value<QObject *>() : nullptr; }

    QList<QQuickItem *> windows() const
    {
        QList<QQuickItem *> list;
        if (!layer())
            return list;
        const QJSValue array = js(layer()).property(u"windows"_s);
        const int count = array.property(u"length"_s).toInt();
        for (int i = 0; i < count; ++i)
            if (auto *item = qobject_cast<QQuickItem *>(array.property(quint32(i)).toQObject()))
                list.append(item);
        return list;
    }

    // A window spec is a string (matches the title or the app id) or an
    // object {"title": ..., "appId": ...}; each matches by substring.
    // By objectName, in the QObject tree and then the visual tree: delegates a
    // Repeater creates are only reachable through the latter.
    QQuickItem *findItem(const QString &name) const
    {
        if (auto *item = m_window->findChild<QQuickItem *>(name))
            return item;
        QList<QQuickItem *> pending { m_window->contentItem() };
        while (!pending.isEmpty()) {
            QQuickItem *item = pending.takeFirst();
            if (item->objectName() == name)
                return item;
            pending.append(item->childItems());
        }
        return nullptr;
    }

    QQuickItem *findWindow(const QJsonValue &spec) const
    {
        const QString title = spec.isObject() ? spec[u"title"_s].toString() : spec.toString();
        const QString appId = spec.isObject() ? spec[u"appId"_s].toString() : spec.toString();
        for (QQuickItem *w : windows()) {
            if (w->property("closing").toBool())
                continue;
            if ((!title.isEmpty() && w->property("title").toString().contains(title))
                || (!appId.isEmpty() && w->property("appId").toString().contains(appId)))
                return w;
        }
        return nullptr;
    }

    QList<QQuickItem *> popups() const
    {
        QList<QQuickItem *> found;
        std::function<void(QQuickItem *)> walk = [&](QQuickItem *item) {
            if (item->inherits("QWaylandQuickShellSurfaceItem") && item->isVisible()) {
                const auto *surface = item->property("shellSurface").value<QObject *>();
                if (surface && surface->property("popup").value<QObject *>())
                    found.append(item);
            }
            for (QQuickItem *child : item->childItems())
                walk(child);
        };
        walk(m_window->contentItem());
        return found;
    }

    // Resolves a target to a point in window coordinates:
    //   [x, y]
    //   {"window": spec, "part": "frame|titlebar|content|close|maximize|minimize", "at": [x, y]}
    //   {"item": "<objectName>", "at": [x, y]}
    // "at" is relative to the part's top-left corner (negative values from the
    // right and bottom edges); without it, the centre.
    std::optional<QPointF> resolve(const QJsonValue &target)
    {
        if (target.isArray()) {
            const QJsonArray a = target.toArray();
            return QPointF(a.at(0).toDouble(), a.at(1).toDouble());
        }
        const QJsonObject t = target.toObject();
        QQuickItem *item = nullptr;
        if (t.contains(u"window"_s)) {
            QQuickItem *window = findWindow(t[u"window"_s]);
            if (!window) {
                m_error = u"no window matches the target"_s;
                return std::nullopt;
            }
            const QString part = t[u"part"_s].toString(u"frame"_s);
            static const QHash<QString, QString> objectNames { { u"titlebar"_s, u"titleBar"_s },
                                                               { u"content"_s, u"content"_s },
                                                               { u"close"_s, u"close"_s },
                                                               { u"maximize"_s, u"maximize"_s },
                                                               { u"minimize"_s, u"minimize"_s } };
            item = part == u"frame"_s ? window : window->findChild<QQuickItem *>(objectNames.value(part));
            if (!item) {
                m_error = u"window has no part \"%1\""_s.arg(part);
                return std::nullopt;
            }
        } else if (t.contains(u"item"_s)) {
            item = findItem(t[u"item"_s].toString());
            if (!item) {
                m_error = u"no item named \"%1\""_s.arg(t[u"item"_s].toString());
                return std::nullopt;
            }
        } else {
            m_error = u"unknown target"_s;
            return std::nullopt;
        }
        // Negative "at" values count from the right and bottom edges.
        const QJsonArray at = t[u"at"_s].toArray();
        const auto coordinate = [](double value, double extent) { return value < 0 ? extent + value : value; };
        const QPointF local = at.size() == 2 ? QPointF(coordinate(at.at(0).toDouble(), item->width()),
                                                       coordinate(at.at(1).toDouble(), item->height()))
                                             : QPointF(item->width() / 2, item->height() / 2);
        const QPointF point = item->mapToScene(local);
        // An item target must be on screen: not hidden, scrolled away or clipped.
        if (t.contains(u"item"_s)) {
            if (!item->isVisible()) {
                m_error = u"\"%1\" is not visible"_s.arg(t[u"item"_s].toString());
                return std::nullopt;
            }
            for (QQuickItem *ancestor = item->parentItem(); ancestor; ancestor = ancestor->parentItem()) {
                if (ancestor->clip()
                    && !ancestor->mapRectToScene(QRectF(0, 0, ancestor->width(), ancestor->height())).contains(point)) {
                    m_error = u"\"%1\" is scrolled or clipped out of view"_s.arg(t[u"item"_s].toString());
                    return std::nullopt;
                }
            }
        }
        // A window target must be the window a person would hit there.
        if (t.contains(u"window"_s)) {
            QQuickItem *window = findWindow(t[u"window"_s]);
            for (QQuickItem *other : windows()) {
                if (other == window || !other->isVisible() || other->property("minimized").toBool()
                    || other->property("closing").toBool() || other->z() <= window->z())
                    continue;
                if (other->mapRectToScene(QRectF(0, 0, other->width(), other->height())).contains(point)) {
                    m_error = u"the target point is covered by \"%1\""_s.arg(other->property("title").toString());
                    return std::nullopt;
                }
            }
        }
        return point;
    }

    // ── Input ───────────────────────────────────────────────────────────────

    // Input enters through QWindowSystemInterface, as a device's (and
    // QTest's) does, so QGuiApplication's pointer and button state stay
    // consistent for everything downstream, including the Wayland seat.
    void mouse(QEvent::Type type, QPointF pos, Qt::MouseButton button)
    {
        if (type == QEvent::MouseButtonPress)
            m_buttons |= button;
        else if (type == QEvent::MouseButtonRelease)
            m_buttons &= ~button;
        // QWindowSystemInterface takes native pixels; scripts use logical ones.
        QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(
            m_window, ulong(m_clock.elapsed()), QHighDpi::toNativeLocalPosition(pos, m_window),
            QHighDpi::toNativeGlobalPosition(m_window->mapToGlobal(pos), m_window), m_buttons, button, type);
    }

    void key(int key, Qt::KeyboardModifiers modifiers, const QString &text)
    {
        const quint32 scanCode = input::evdevKeyCode(key) + 8;
        for (QEvent::Type type : { QEvent::KeyPress, QEvent::KeyRelease })
            QWindowSystemInterface::handleExtendedKeyEvent(m_window, ulong(m_clock.elapsed()), type, key, modifiers,
                                                           scanCode, 0, 0, text);
    }

    void enqueueClick(const QJsonValue &target, int clicks)
    {
        m_queue.push_back([this, target] {
            const auto p = resolve(target);
            if (!p)
                return -1;
            m_pointer = *p;
            mouse(QEvent::MouseMove, m_pointer, Qt::NoButton);
            return 30;
        });
        for (int i = 0; i < clicks; ++i) {
            m_queue.push_back([this] {
                mouse(QEvent::MouseButtonPress, m_pointer, Qt::LeftButton);
                return 40;
            });
            m_queue.push_back([this] {
                mouse(QEvent::MouseButtonRelease, m_pointer, Qt::LeftButton);
                return 60;
            });
        }
    }

    void enqueueDrag(const QJsonObject &spec)
    {
        const int steps = std::max(2, spec[u"steps"_s].toInt(16));
        auto from = std::make_shared<QPointF>();
        auto to = std::make_shared<QPointF>();
        m_queue.push_back([this, spec, from, to] {
            const auto start = resolve(spec[u"from"_s]);
            if (!start)
                return -1;
            *from = *start;
            if (spec.contains(u"by"_s)) {
                const QJsonArray by = spec[u"by"_s].toArray();
                *to = *start + QPointF(by.at(0).toDouble(), by.at(1).toDouble());
            } else {
                const auto end = resolve(spec[u"to"_s]);
                if (!end)
                    return -1;
                *to = *end;
            }
            mouse(QEvent::MouseMove, *from, Qt::NoButton);
            return 30;
        });
        m_queue.push_back([this, from] {
            mouse(QEvent::MouseButtonPress, *from, Qt::LeftButton);
            return 50;
        });
        for (int i = 1; i <= steps; ++i) {
            m_queue.push_back([this, from, to, i, steps] {
                const qreal f = qreal(i) / steps;
                mouse(QEvent::MouseMove, *from + (*to - *from) * f, Qt::NoButton);
                return 16;
            });
        }
        m_queue.push_back([this, to, spec] {
            mouse(QEvent::MouseButtonRelease, *to, Qt::LeftButton);
            return spec[u"settle"_s].toInt(600);
        });
    }

    // ── Virtual game controller ─────────────────────────────────────────────

    // {"gamepad": {"connect": "xbox|playstation|nintendo|generic", "name": ...}}
    // {"gamepad": {"press" | "release" | "tap": "south|east|west|north|lb|rb|lt|rt|
    //              back|start|guide|ls|rs|up|down|left|right"}}
    // {"gamepad": {"stick": [x, y], "ms": 400}}    the left stick, then centred
    // {"gamepad": {"disconnect": true}}
    int gamepad(const QJsonObject &spec)
    {
        using namespace qtos::input;
        GamepadManager *manager = GamepadManager::instance();
        if (spec.contains(u"connect"_s)) {
            static const QHash<QString, Family> families { { u"xbox"_s, Family::Xbox },
                                                           { u"playstation"_s, Family::PlayStation },
                                                           { u"nintendo"_s, Family::Nintendo },
                                                           { u"generic"_s, Family::Generic } };
            const QString family = spec[u"connect"_s].toString(u"xbox"_s);
            m_pad = {};
            m_gamepad = manager->addVirtual(families.value(family, Family::Xbox),
                                            spec[u"name"_s].toString(u"Virtual Controller"_s));
            return 400;
        }
        if (m_gamepad.isEmpty()) {
            m_error = u"no virtual controller is connected"_s;
            return -1;
        }
        if (spec.contains(u"disconnect"_s)) {
            manager->removeVirtual(m_gamepad);
            m_gamepad.clear();
            return 400;
        }
        if (spec.contains(u"stick"_s)) {
            const QJsonArray xy = spec[u"stick"_s].toArray();
            m_pad.axis(Axis::LeftX) = float(xy.at(0).toDouble());
            m_pad.axis(Axis::LeftY) = float(xy.at(1).toDouble());
            manager->setVirtualState(m_gamepad, m_pad);
            m_queue.push_front([this] {
                m_pad.axis(Axis::LeftX) = 0;
                m_pad.axis(Axis::LeftY) = 0;
                GamepadManager::instance()->setVirtualState(m_gamepad, m_pad);
                return 200;
            });
            return spec[u"ms"_s].toInt(300);
        }
        static const QHash<QString, Button> buttons {
            { u"south"_s, Button::South },    { u"east"_s, Button::East },       { u"west"_s, Button::West },
            { u"north"_s, Button::North },    { u"lb"_s, Button::LeftShoulder }, { u"rb"_s, Button::RightShoulder },
            { u"lt"_s, Button::LeftTrigger }, { u"rt"_s, Button::RightTrigger }, { u"back"_s, Button::Back },
            { u"start"_s, Button::Start },    { u"guide"_s, Button::Guide },     { u"ls"_s, Button::LeftStick },
            { u"rs"_s, Button::RightStick },  { u"up"_s, Button::DPadUp },       { u"down"_s, Button::DPadDown },
            { u"left"_s, Button::DPadLeft },  { u"right"_s, Button::DPadRight },
        };
        const QString name = spec[u"press"_s].toString(spec[u"release"_s].toString(spec[u"tap"_s].toString()));
        if (!buttons.contains(name)) {
            m_error = u"unknown controller button \"%1\""_s.arg(name);
            return -1;
        }
        const Button button = buttons.value(name);
        const auto set = [this, button](bool down) {
            // Triggers are analogue: a press is a full pull.
            if (button == Button::LeftTrigger)
                m_pad.axis(Axis::LeftTrigger) = down ? 1.0f : 0.0f;
            else if (button == Button::RightTrigger)
                m_pad.axis(Axis::RightTrigger) = down ? 1.0f : 0.0f;
            else
                m_pad.set(button, down);
            GamepadManager::instance()->setVirtualState(m_gamepad, m_pad);
        };
        if (spec.contains(u"release"_s)) {
            set(false);
            return 250;
        }
        set(true);
        if (spec.contains(u"tap"_s)) {
            m_queue.push_front([set] {
                set(false);
                return 350;
            });
            return 90;
        }
        return 250;
    }

    // ── Steps ───────────────────────────────────────────────────────────────

    bool expand(const QJsonObject &step)
    {
        if (!m_desktop || !m_engine) {
            m_error = u"no Desktop (objectName \"desktop\") in the window"_s;
            return false;
        }
        if (step.contains(u"wait"_s)) {
            const int ms = step[u"wait"_s].toInt();
            m_queue.push_back([ms] { return ms; });
        } else if (step.contains(u"waitFor"_s)) {
            const QJsonObject condition = step[u"waitFor"_s].toObject();
            const qint64 deadline = m_clock.elapsed() + condition[u"timeout"_s].toInt(10000);
            enqueueWait(condition, deadline);
        } else if (step.contains(u"click"_s)) {
            enqueueClick(step[u"click"_s], 1);
        } else if (step.contains(u"doubleClick"_s)) {
            enqueueClick(step[u"doubleClick"_s], 2);
        } else if (step.contains(u"move"_s)) {
            const QJsonValue target = step[u"move"_s];
            m_queue.push_back([this, target] {
                const auto p = resolve(target);
                if (!p)
                    return -1;
                mouse(QEvent::MouseMove, *p, Qt::NoButton);
                return 100;
            });
        } else if (step.contains(u"drag"_s)) {
            enqueueDrag(step[u"drag"_s].toObject());
        } else if (step.contains(u"gamepad"_s)) {
            const QJsonObject spec = step[u"gamepad"_s].toObject();
            m_queue.push_back([this, spec] { return gamepad(spec); });
        } else if (step.contains(u"scroll"_s)) {
            // {"scroll": {"at": target, "notches": n}}: turns a mouse wheel n
            // notches (positive scrolls down, towards the end).
            const QJsonObject spec = step[u"scroll"_s].toObject();
            m_queue.push_back([this, spec] {
                const auto p = resolve(spec[u"at"_s]);
                if (!p)
                    return -1;
                const QPoint angle(0, -120 * spec[u"notches"_s].toInt(1));
                QWindowSystemInterface::handleWheelEvent(
                    m_window, ulong(m_clock.elapsed()), QHighDpi::toNativeLocalPosition(*p, m_window),
                    QHighDpi::toNativeGlobalPosition(m_window->mapToGlobal(*p), m_window), QPoint(), angle);
                return 450;
            });
        } else if (step.contains(u"key"_s)) {
            const QKeySequence sequence = QKeySequence::fromString(step[u"key"_s].toString());
            if (sequence.isEmpty()) {
                m_error = u"cannot parse key"_s;
                return false;
            }
            const QKeyCombination combination = sequence[0];
            m_queue.push_back([this, combination] {
                const int k = combination.key();
                const QString text =
                    k == Qt::Key_Escape   ? u"\x1b"_s
                    : k == Qt::Key_Return ? u"\r"_s
                    : (k >= Qt::Key_Space && k <= Qt::Key_AsciiTilde && !combination.keyboardModifiers())
                        ? QString(QChar(k)).toLower()
                        : QString();
                key(k, combination.keyboardModifiers(), text);
                return 150;
            });
        } else if (step.contains(u"call"_s)) {
            const QString method = step[u"call"_s].toString();
            const QJsonArray args = step[u"args"_s].toArray();
            m_queue.push_back([this, method, args] {
                QJSValue desktop = js(m_desktop);
                QJSValue function = desktop.property(method);
                if (!function.isCallable()) {
                    m_error = u"Desktop has no method %1"_s.arg(method);
                    return -1;
                }
                QJSValueList values;
                for (const QJsonValue &arg : args)
                    values.append(m_engine->toScriptValue(arg.toVariant()));
                const QJSValue result = function.callWithInstance(desktop, values);
                if (result.isError()) {
                    m_error = result.toString();
                    return -1;
                }
                return 400;
            });
        } else if (step.contains(u"close"_s)) {
            const QJsonValue spec = step[u"close"_s];
            m_queue.push_back([this, spec] {
                QQuickItem *window = findWindow(spec);
                if (!window) {
                    m_error = u"no window to close"_s;
                    return -1;
                }
                QJSValue manager = js(layer());
                manager.property(u"close"_s).callWithInstance(manager, { js(window), QJSValue(false) });
                return 300;
            });
        } else if (step.contains(u"restore"_s)) {
            // What clicking the window's app on the Shelf does.
            const QJsonValue spec = step[u"restore"_s];
            m_queue.push_back([this, spec] {
                QQuickItem *window = findWindow(spec);
                if (!window) {
                    m_error = u"no window to restore"_s;
                    return -1;
                }
                QJSValue desktop = js(m_desktop);
                desktop.property(u"launch"_s)
                    .callWithInstance(desktop,
                                      { QJSValue(window->property("appId").toString()), m_engine->newObject() });
                return 700;
            });
        } else if (step.contains(u"screenshot"_s)) {
            const QString file = path(step[u"screenshot"_s].toString());
            m_queue.push_back([this, file] {
                if (!m_window->grabWindow().save(file)) {
                    m_error = u"cannot write %1"_s.arg(file);
                    return -1;
                }
                return 0;
            });
        } else if (step.contains(u"state"_s)) {
            const QString file = path(step[u"state"_s].toString());
            m_queue.push_back([this, file] {
                QFile out(file);
                if (!out.open(QIODevice::WriteOnly)) {
                    m_error = u"cannot write %1"_s.arg(file);
                    return -1;
                }
                out.write(QJsonDocument(state()).toJson(QJsonDocument::Indented));
                return 0;
            });
        } else if (step.contains(u"expect"_s)) {
            // {"expect": {"path": "settings.reduceMotion", "equals": true}}
            // {"expect": {"window": spec, "path": "frameRect", "near": [x, y, w, h], "tolerance": 2}}
            const QJsonObject expectation = step[u"expect"_s].toObject();
            m_queue.push_back([this, expectation] {
                QJsonValue value;
                if (expectation.contains(u"window"_s)) {
                    QQuickItem *window = findWindow(expectation[u"window"_s]);
                    if (!window) {
                        if (expectation[u"equals"_s] == QJsonValue(u"absent"_s))
                            return 0;
                        m_error = u"no window matches"_s;
                        return -1;
                    }
                    value = windowState(window);
                } else {
                    value = state();
                }
                const QString path = expectation[u"path"_s].toString();
                for (const QString &key : path.split(u'.', Qt::SkipEmptyParts))
                    value = value.toObject().value(key);
                const QString actual =
                    QString::fromUtf8(QJsonDocument(QJsonArray { value }).toJson(QJsonDocument::Compact));
                if (expectation.contains(u"near"_s)) {
                    // "near" is a literal array, or a path into the state (for
                    // example "workArea"), optionally scaled per component.
                    QJsonArray want = expectation[u"near"_s].toArray();
                    if (expectation[u"near"_s].isString()) {
                        QJsonValue reference = state();
                        for (const QString &key : expectation[u"near"_s].toString().split(u'.', Qt::SkipEmptyParts))
                            reference = reference.toObject().value(key);
                        want = reference.toArray();
                        const QJsonArray scale = expectation[u"scale"_s].toArray();
                        for (qsizetype i = 0; i < want.size() && i < scale.size(); ++i)
                            want[i] = want.at(i).toDouble() * scale.at(i).toDouble();
                    }
                    const QJsonArray have = value.toArray();
                    const double tolerance = expectation[u"tolerance"_s].toDouble(2);
                    bool ok = want.size() == have.size();
                    for (qsizetype i = 0; ok && i < want.size(); ++i)
                        ok = std::abs(want.at(i).toDouble() - have.at(i).toDouble()) <= tolerance;
                    if (!ok) {
                        m_error = u"%1 is %2, expected near %3"_s.arg(
                            path, actual, QString::fromUtf8(QJsonDocument(want).toJson(QJsonDocument::Compact)));
                        return -1;
                    }
                } else if (value != expectation[u"equals"_s]) {
                    m_error = u"%1 is %2, expected %3"_s.arg(
                        path, actual,
                        QString::fromUtf8(
                            QJsonDocument(QJsonArray { expectation[u"equals"_s] }).toJson(QJsonDocument::Compact)));
                    return -1;
                }
                return 0;
            });
        } else if (step.contains(u"log"_s)) {
            const QString message = step[u"log"_s].toString();
            m_queue.push_back([message] {
                std::fprintf(stderr, "script: %s\n", qPrintable(message));
                return 0;
            });
        } else {
            m_error = u"unknown step"_s;
            return false;
        }
        return true;
    }

    // Supported conditions: {"windows": n} (open, non-closing windows),
    // {"window": spec} (exists), {"gone": spec}, {"popups": n}.
    void enqueueWait(const QJsonObject &condition, qint64 deadline)
    {
        m_queue.push_front([this, condition, deadline] {
            bool met = false;
            if (condition.contains(u"windows"_s)) {
                int open = 0;
                for (QQuickItem *w : windows())
                    open += w->property("closing").toBool() ? 0 : 1;
                met = open == condition[u"windows"_s].toInt();
            } else if (condition.contains(u"window"_s)) {
                met = findWindow(condition[u"window"_s]) != nullptr;
            } else if (condition.contains(u"gone"_s)) {
                met = findWindow(condition[u"gone"_s]) == nullptr;
            } else if (condition.contains(u"popups"_s)) {
                met = popups().size() == condition[u"popups"_s].toInt();
            }
            if (met)
                return 100;
            if (m_clock.elapsed() > deadline) {
                m_error = u"timed out"_s;
                return -1;
            }
            enqueueWait(condition, deadline);
            return 50;
        });
    }

    QJsonObject windowState(QQuickItem *w) const
    {
        QJsonObject entry {
            { u"title"_s, w->property("title").toString() },
            { u"appId"_s, w->property("appId").toString() },
            { u"isClient"_s, w->property("isClient").toBool() },
            { u"decorated"_s, w->property("decorated").toBool() },
            { u"focused"_s, w->property("focused").toBool() },
            { u"demandsAttention"_s, w->property("demandsAttention").toBool() },
            { u"maximized"_s, w->property("maximized").toBool() },
            { u"minimized"_s, w->property("minimized").toBool() },
            { u"closing"_s, w->property("closing").toBool() },
            { u"inOverview"_s, w->property("inOverview").toBool() },
            { u"space"_s, w->property("space").toInt() },
            { u"frameRect"_s, rectJson(w->property("frameRect")) },
            { u"onScreen"_s, rectJson(w->mapRectToScene(QRectF(0, 0, w->width(), w->height()))) },
        };
        if (w->property("isClient").toBool() && layer()) {
            // What the Shelf shows for this app (desktop entry or title).
            QJSValue manager = js(layer());
            const QVariantMap identity = manager.property(u"clientIdentity"_s)
                                             .callWithInstance(manager, { QJSValue(w->property("appId").toString()) })
                                             .toVariant()
                                             .toMap();
            entry.insert(u"identity"_s, QJsonObject {
                                            { u"name"_s, identity.value(u"name"_s).toString() },
                                            { u"icon"_s, identity.value(u"icon"_s).toUrl().toString() },
                                        });
        }
        if (auto *content = w->findChild<QQuickItem *>(u"content"_s)) {
            if (auto *client = content->property("item").value<QObject *>();
                client && w->property("isClient").toBool()) {
                entry.insert(u"clientGeometry"_s, rectJson(client->property("geometry")));
                const QSizeF configured = client->property("configuredSize").toSizeF();
                entry.insert(u"configuredSize"_s,
                             QJsonArray { qRound(configured.width()), qRound(configured.height()) });
                // The client's surface item, as input sees it.
                for (auto *item : client->findChildren<QQuickItem *>()) {
                    if (!item->inherits("QWaylandQuickShellSurfaceItem"))
                        continue;
                    QJsonObject surfaceInfo;
                    if (auto *surface = item->property("surface").value<QObject *>()) {
                        const QSize destination = surface->property("destinationSize").toSize();
                        const QSize buffer = surface->property("bufferSize").toSize();
                        surfaceInfo = {
                            { u"destinationSize"_s, QJsonArray { destination.width(), destination.height() } },
                            { u"bufferSize"_s, QJsonArray { buffer.width(), buffer.height() } },
                            { u"hasContent"_s, surface->property("hasContent").toBool() },
                        };
                    }
                    entry.insert(u"surfaceItem"_s,
                                 QJsonObject {
                                     { u"onScreen"_s,
                                       rectJson(item->mapRectToScene(QRectF(0, 0, item->width(), item->height()))) },
                                     { u"enabled"_s, item->isEnabled() },
                                     { u"visible"_s, item->isVisible() },
                                     { u"inputEventsEnabled"_s, item->property("inputEventsEnabled").toBool() },
                                     { u"acceptedButtons"_s, int(item->acceptedMouseButtons()) },
                                     { u"surface"_s, surfaceInfo },
                                 });
                    break;
                }
            }
            entry.insert(u"contentOnScreen"_s,
                         rectJson(content->mapRectToScene(QRectF(0, 0, content->width(), content->height()))));
        }
        return entry;
    }

    // A QML `var` arrives as a QJSValue; JSON wants plain variants.
    static QVariant plain(const QVariant &value)
    {
        return value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
    }

    QJsonObject state() const
    {
        QJsonArray list;
        for (QQuickItem *w : windows())
            list.append(windowState(w));
        QJsonArray popupRects;
        for (QQuickItem *p : popups())
            popupRects.append(rectJson(p->mapRectToScene(QRectF(0, 0, p->width(), p->height()))));
        QObject *manager = layer();
        const auto snapshot = [](const char *service, std::initializer_list<const char *> names) {
            QJsonObject values;
            if (auto *replica = Services::instance()->property(service).value<QObject *>()) {
                for (const char *name : names)
                    values.insert(QString::fromLatin1(name), QJsonValue::fromVariant(replica->property(name)));
            }
            return values;
        };
        return {
            { u"settings"_s,
              snapshot("settings", { "darkMode", "accentHue", "reduceMotion", "reduceTransparency", "increaseContrast",
                                     "livingWallpaper", "audioReactive", "reactiveIntensity", "reactiveLowHz",
                                     "reactiveHighHz", "controllerInterface", "controllerAutoSwitch" }) },
            { u"audio"_s, snapshot("audio", { "volume", "muted", "outputDevice", "equalizer", "equalizerPreset",
                                              "systemSounds", "systemVolume", "active" }) },
            { u"media"_s,
              snapshot("media", { "playing", "title", "position", "duration", "queueIndex", "shuffle", "repeat" }) },
            { u"windows"_s, list },
            { u"popups"_s, popupRects },
            { u"workArea"_s, rectJson(manager ? manager->property("workArea") : QVariant()) },
            { u"overview"_s, manager ? manager->property("overview").toBool() : false },
            { u"currentSpace"_s, manager ? manager->property("currentSpace").toInt() : 0 },
            { u"surface"_s, m_desktop->property("surface").toString() },
            { u"controller"_s, QJsonValue::fromVariant(plain(m_desktop->property("controllerState"))) },
        };
    }

    QQuickWindow *m_window;
    QJsonArray m_steps;
    QString m_baseDir;
    QObject *m_desktop = nullptr;
    QQmlEngine *m_engine = nullptr;
    QString m_gamepad;
    qtos::input::GamepadState m_pad;
    qsizetype m_next = 0;
    std::deque<Action> m_queue;
    QElapsedTimer m_clock;
    Qt::MouseButtons m_buttons;
    QPointF m_pointer;
    QString m_error;
};

} // namespace

void runShellScript(QQuickWindow *window, const QString &path)
{
    QFile file(path);
    if (!window || !file.open(QIODevice::ReadOnly)) {
        std::fprintf(stderr, "script: cannot read %s\n", qPrintable(path));
        QCoreApplication::exit(4);
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (document.isNull()) {
        std::fprintf(stderr, "script: %s: %s\n", qPrintable(path), qPrintable(error.errorString()));
        QCoreApplication::exit(4);
        return;
    }
    const QJsonArray steps = document.isArray() ? document.array() : document.object()[u"steps"_s].toArray();
    auto *script = new ShellScript(window, steps, QFileInfo(path).absolutePath());
    script->start();
}

} // namespace qtos
