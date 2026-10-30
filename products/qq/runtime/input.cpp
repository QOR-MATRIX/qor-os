// Changed for QOR OS: gamepads through QOR.Input's GamepadManager (QOR OS's input/gamepads), in place of XInput, and
// only for the application in front; the left stick's four ways as pad buttons ("LS Up" and the rest).

#include "input.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>

#include <algorithm>
#include <cmath>

#ifndef Q_OS_WASM
#include "gamepadmanager.h"
#endif

namespace qq {

namespace {

bool isText(const QObject *o)
{
    return o && (o->inherits("QQuickTextInput") || o->inherits("QQuickTextEdit"));
}

/// Whether a key on its way to `receiver` is being typed into a text field, which a game must not read: the receiver
/// itself, the item with focus in the window it is sent to, or the application's focus.
bool typing(const QObject *receiver)
{
    if (isText(receiver))
        return true;
    if (const auto *window = qobject_cast<const QQuickWindow *>(receiver))
        return isText(window->activeFocusItem());
    return isText(QGuiApplication::focusObject());
}

QVector2D clampedToOne(QVector2D v)
{
    const float length = v.length();
    return length > 1.0f ? v / length : v;
}

QString mouseName(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton: return QStringLiteral("Mouse Left");
    case Qt::RightButton: return QStringLiteral("Mouse Right");
    case Qt::MiddleButton: return QStringLiteral("Mouse Middle");
    default: return {};
    }
}

/// Jump and run, as they always were: what a scene's InputAction restates, and what forget() goes back to.
struct BuiltIn {
    const char *name;
    QStringList keys;
    QStringList pad;
};
const QList<BuiltIn> &builtIns()
{
    static const QList<BuiltIn> list{
        {"jump", {QStringLiteral("Space")}, {QStringLiteral("A")}},
        {"run", {QStringLiteral("Shift")}, {QStringLiteral("LS"), QStringLiteral("LB"), QStringLiteral("RB")}},
    };
    return list;
}

QStringList cleaned(const QStringList &names)
{
    QStringList out;
    for (const QString &n : names) {
        const QString t = n.trimmed();
        if (!t.isEmpty() && !out.contains(t, Qt::CaseInsensitive))
            out << t;
    }
    return out;
}

}  // namespace

QVector2D withDeadZone(QVector2D stick, float dead)
{
    const float length = stick.length();
    if (length <= dead)
        return {};
    const float scaled = std::min(1.0f, (length - dead) / (1.0f - dead));
    return stick / length * scaled;
}

QString Input::keyName(int key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return QString(QChar(char('A' + (key - Qt::Key_A))));
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return QString(QChar(char('0' + (key - Qt::Key_0))));
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12)
        return QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
    switch (key) {
    case Qt::Key_Space: return QStringLiteral("Space");
    case Qt::Key_Return: case Qt::Key_Enter: return QStringLiteral("Enter");
    case Qt::Key_Escape: return QStringLiteral("Escape");
    case Qt::Key_Tab: return QStringLiteral("Tab");
    case Qt::Key_Backspace: return QStringLiteral("Backspace");
    case Qt::Key_Shift: return QStringLiteral("Shift");
    case Qt::Key_Control: return QStringLiteral("Ctrl");
    case Qt::Key_Alt: return QStringLiteral("Alt");
    case Qt::Key_Up: return QStringLiteral("Up");
    case Qt::Key_Down: return QStringLiteral("Down");
    case Qt::Key_Left: return QStringLiteral("Left");
    case Qt::Key_Right: return QStringLiteral("Right");
    default: return {};
    }
}

int Input::keyOf(const QString &name)
{
    static const QHash<QString, int> keys = [] {
        QHash<QString, int> all;
        for (int k = Qt::Key_A; k <= Qt::Key_Z; ++k)
            all.insert(keyName(k).toLower(), k);
        for (int k = Qt::Key_0; k <= Qt::Key_9; ++k)
            all.insert(keyName(k).toLower(), k);
        for (int k = Qt::Key_F1; k <= Qt::Key_F12; ++k)
            all.insert(keyName(k).toLower(), k);
        for (int k : {Qt::Key_Space, Qt::Key_Return, Qt::Key_Escape, Qt::Key_Tab, Qt::Key_Backspace, Qt::Key_Shift,
                      Qt::Key_Control, Qt::Key_Alt, Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right})
            all.insert(keyName(k).toLower(), k);
        all.insert(QStringLiteral("esc"), Qt::Key_Escape);
        all.insert(QStringLiteral("return"), Qt::Key_Return);
        all.insert(QStringLiteral("control"), Qt::Key_Control);
        return all;
    }();
    return keys.value(name.trimmed().toLower(), 0);
}

Input::Input(QObject *parent) : QObject(parent)
{
    for (const BuiltIn &b : builtIns())
        m_actions.insert(QLatin1String(b.name), {b.keys, b.pad});
    if (qApp)
        qApp->installEventFilter(this);
    m_front = QGuiApplication::applicationState() == Qt::ApplicationActive;
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        m_front = state == Qt::ApplicationActive;
        if (!m_front)
            release();
        readPads();
    });
#ifndef Q_OS_WASM
    // QOR OS's controller service: every controller connected, combined, in one standard layout.
    auto *pads = qor::input::GamepadManager::instance();
    connect(pads, &qor::input::GamepadManager::connected, this, &Input::readPads);
    connect(pads, &qor::input::GamepadManager::disconnected, this, &Input::readPads);
    connect(pads, &qor::input::GamepadManager::mergedChanged, this, &Input::readPads);
    readPads();
#endif
}

Input::~Input()
{
    if (qApp)
        qApp->removeEventFilter(this);
}

Input *Input::create(QQmlEngine *, QJSEngine *)
{
    return new Input;
}

QVector2D Input::move() const
{
    if (m_replaying)
        return m_replayMove;
    auto held = [this](int a, int b = 0) { return m_keys.contains(a) || (b && m_keys.contains(b)) ? 1.0f : 0.0f; };
    const QVector2D keys(held(Qt::Key_D) - held(Qt::Key_A), held(Qt::Key_W, Qt::Key_Up) - held(Qt::Key_S, Qt::Key_Down));
    return clampedToOne(keys + (m_padDeadZoned ? m_padLeft : withDeadZone(m_padLeft)));
}

QVector2D Input::look() const
{
    if (m_replaying)
        return m_replayLook;
    const float turn = (m_keys.contains(Qt::Key_Right) ? 1.0f : 0.0f) - (m_keys.contains(Qt::Key_Left) ? 1.0f : 0.0f);
    return clampedToOne(QVector2D(turn, 0) + (m_padDeadZoned ? m_padRight : withDeadZone(m_padRight)));
}

void Input::setReadsHardware(bool reads)
{
    if (reads == m_readsHardware)
        return;
    m_readsHardware = reads;
    emit readsHardwareChanged();
    readPads();
}

void Input::pointerMoved(QPointF delta)
{
    m_pointer += delta;
}

QPointF Input::takePointer()
{
    const QPointF taken = m_pointer;
    m_pointer = {};
    return taken;
}

void Input::gamepadState(QVector2D left, QVector2D right, bool jump, bool run, bool connected)
{
    QSet<QString> buttons;
    if (jump)
        buttons.insert(QStringLiteral("A"));
    if (run)
        buttons.insert(QStringLiteral("LB"));
    padState(left, right, buttons, connected, false);
}

void Input::gamepadButtons(const QStringList &down)
{
    QSet<QString> buttons;
    for (const QString &b : down)
        buttons.insert(b.trimmed());
    padState(m_padLeft, m_padRight, buttons, true, m_padDeadZoned);
}

void Input::padState(QVector2D left, QVector2D right, const QSet<QString> &buttons, bool connected, bool deadZoned)
{
    const bool wasConnected = m_padConnected;
    QSet<QString> down = connected ? buttons : QSet<QString>();
    if (!connected)
        left = right = {};
    // A pad button pressed while listening becomes the binding, and counts for nothing else until it is let go.
    m_swallowedPad &= down;
    if (!m_listening.isEmpty()) {
        for (const QString &b : std::as_const(down)) {
            if (!m_pad.contains(b) && !m_swallowedPad.contains(b)) {
                m_swallowedPad.insert(b);
                bind(true, b);
                break;
            }
        }
    }
    down -= m_swallowedPad;
    if (left == m_padLeft && right == m_padRight && down == m_pad && connected == wasConnected
        && deadZoned == m_padDeadZoned)
        return;
    m_padLeft = left;
    m_padRight = right;
    m_pad = down;
    m_padConnected = connected;
    m_padDeadZoned = deadZoned;
    settle();
    emit changed();
    if (connected != wasConnected)
        emit gamepadChanged();
}

void Input::release()
{
    m_pointer = {};
    if (m_keys.isEmpty() && m_mouse.isEmpty() && m_holds.isEmpty())
        return;
    m_keys.clear();
    m_mouse.clear();
    m_holds.clear();
    settle();
    emit changed();
}

void Input::define(const QString &name, const QStringList &keys, const QStringList &pad)
{
    if (name.trimmed().isEmpty())
        return;
    m_actions.insert(name.trimmed(), {cleaned(keys), cleaned(pad)});
    settle();
}

void Input::forget(const QString &name)
{
    m_holds.remove(name);
    m_actions.remove(name);
    for (const BuiltIn &b : builtIns()) {
        if (name == QLatin1String(b.name))
            m_actions.insert(name, {b.keys, b.pad});
    }
    settle();
}

bool Input::held(const QString &name) const
{
    return m_replaying ? m_replayHeld.contains(name) : m_down.contains(name);
}

QVariantMap Input::takeStep()
{
    const QVector2D m = move();
    const QVector2D l = look();
    QStringList down(m_down.cbegin(), m_down.cend());
    down.sort();
    const QVariantMap step{{QStringLiteral("move"), QVariantList{double(m.x()), double(m.y())}},
                           {QStringLiteral("look"), QVariantList{double(l.x()), double(l.y())}},
                           {QStringLiteral("held"), down},
                           {QStringLiteral("edges"), m_edges}};
    m_edges.clear();
    return step;
}

void Input::replayHands(const QVariantMap &hands)
{
    const QVariantList m = hands.value(QStringLiteral("move")).toList();
    const QVariantList l = hands.value(QStringLiteral("look")).toList();
    m_replayMove = QVector2D(m.value(0).toFloat(), m.value(1).toFloat());
    m_replayLook = QVector2D(l.value(0).toFloat(), l.value(1).toFloat());
    const QStringList held = hands.value(QStringLiteral("held")).toStringList();
    m_replayHeld = QSet<QString>(held.cbegin(), held.cend());
    if (!m_replaying) {
        m_replaying = true;
        emit replayingChanged();
    }
    emit changed();
}

void Input::endReplay()
{
    m_replayMove = {};
    m_replayLook = {};
    m_replayHeld.clear();
    m_edges.clear();
    if (m_replaying) {
        m_replaying = false;
        emit replayingChanged();
    }
    emit changed();
    readPads();
}

void Input::hold(const QString &name, bool down)
{
    if (down)
        m_holds.insert(name);
    else
        m_holds.remove(name);
    settle();
}

void Input::listen(const QString &name)
{
    if (name == m_listening)
        return;
    m_listening = name;
    emit listeningChanged();
}

QString Input::describe(const QString &name) const
{
    const auto it = m_actions.constFind(name);
    if (it == m_actions.cend())
        return {};
    QStringList parts = it->keys;
    for (const QString &b : it->pad)
        parts << QStringLiteral("Pad ") + b;
    return parts.join(QStringLiteral(" / "));
}

QStringList Input::actions() const
{
    QStringList names = m_actions.keys();
    names.sort();
    return names;
}

bool Input::keyHeld(const QString &binding) const
{
    if (binding.startsWith(QStringLiteral("Mouse "), Qt::CaseInsensitive))
        return m_mouse.contains(binding);
    const int key = keyOf(binding);
    return key != 0 && m_keys.contains(key);
}

void Input::bind(bool pad, const QString &binding)
{
    const QString action = m_listening;
    m_listening.clear();
    Binding &b = m_actions[action];
    (pad ? b.pad : b.keys) = QStringList{binding};
    emit listeningChanged();
    emit rebound(action);
    settle();
}

void Input::settle()
{
    QSet<QString> down;
    for (auto it = m_actions.cbegin(); it != m_actions.cend(); ++it) {
        bool on = m_holds.contains(it.key());
        for (const QString &k : it->keys)
            on = on || keyHeld(k);
        for (const QString &p : it->pad)
            on = on || m_pad.contains(p) || stickHeld(p);
        if (on)
            down.insert(it.key());
    }
    for (const QString &held : std::as_const(m_holds))
        down.insert(held);  // a hold may name an action the scene has not declared: it is still held
    if (down == m_down)
        return;
    const QSet<QString> before = m_down;
    m_down = down;
    QStringList went(before.cbegin(), before.cend());
    QStringList came(down.cbegin(), down.cend());
    went.sort();
    came.sort();
    // Kept for the next tick as well as announced: a press and its release between two ticks still reach logic. Only
    // the most recent are kept, so hands left untaken while nothing plays cannot grow without end.
    auto keep = [this](const char *what, const QString &action) {
        m_edges.append(QVariant(QVariantList{QString::fromLatin1(what), action}));
        if (m_edges.size() > 256)
            m_edges.removeFirst();
    };
    for (const QString &a : std::as_const(went)) {
        if (!down.contains(a)) {
            keep("released", a);
            emit released(a);
        }
    }
    for (const QString &a : std::as_const(came)) {
        if (!before.contains(a)) {
            keep("pressed", a);
            emit pressed(a);
        }
    }
    emit changed();
}

bool Input::stickHeld(const QString &binding) const
{
    // Past half way toward a side, so a diagonal holds two of them and a stick at rest, or barely off it, none.
    constexpr float past = 0.5f;
    if (binding == QLatin1String("LS Up"))
        return m_padLeft.y() > past;
    if (binding == QLatin1String("LS Down"))
        return m_padLeft.y() < -past;
    if (binding == QLatin1String("LS Left"))
        return m_padLeft.x() < -past;
    if (binding == QLatin1String("LS Right"))
        return m_padLeft.x() > past;
    return false;
}

bool Input::eventFilter(QObject *watched, QEvent *event)
{
    // Only as they reach a window: from there a press goes on to the item with focus, and would be seen twice. While a
    // replay plays, the hands are its, and no device is read.
    if (!watched->isWindowType() || m_replaying)
        return QObject::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->isAutoRepeat())
            break;
        if (event->type() == QEvent::KeyPress) {
            if (typing(watched))
                break;
            const QString name = keyName(key->key());
            if (!m_listening.isEmpty() && !name.isEmpty()) {
                bind(false, name);  // the press that names the binding does nothing else
                break;
            }
            if (!m_keys.contains(key->key())) {
                m_keys.insert(key->key());
                settle();
                emit changed();
            }
        } else if (m_keys.remove(key->key())) {
            settle();
            emit changed();
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease: {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        const QString name = mouseName(mouse->button());
        if (mouse->position() != m_pointerAt) {
            m_pointerAt = mouse->position();
            emit pointerChanged();
        }
        if (name.isEmpty())
            break;
        if (event->type() == QEvent::MouseButtonPress) {
            if (!m_listening.isEmpty()) {
                bind(false, name);
                break;
            }
            if (!m_mouse.contains(name)) {
                m_mouse.insert(name);
                settle();
            }
        } else if (m_mouse.remove(name)) {
            settle();
        }
        break;
    }
    case QEvent::MouseMove: {
        const QPointF at = static_cast<QMouseEvent *>(event)->position();
        if (at != m_pointerAt) {
            m_pointerAt = at;
            emit pointerChanged();
        }
        break;
    }
    case QEvent::FocusOut:
        release();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void Input::readPads()
{
#ifndef Q_OS_WASM
    if (!m_readsHardware || m_replaying)
        return;
    using qor::input::Axis;
    using qor::input::Button;
    const auto *pads = qor::input::GamepadManager::instance();
    // With no controller connected, the pad is whatever a check or the agent gives (gamepadState); the last one
    // unplugged lets go of everything it held.
    if (pads->gamepads().isEmpty()) {
        if (m_padFromHardware) {
            m_padFromHardware = false;
            padState({}, {}, {}, false, true);
        }
        return;
    }
    m_padFromHardware = true;
    // A pad pressed while another application is in front, the shell's included, moves nobody here.
    if (!m_front) {
        padState({}, {}, {}, true, true);
        return;
    }
    const qor::input::GamepadState pad = pads->merged();
    // Guide is the system's, never a game's.
    static const QList<QPair<Button, QString>> names{
        {Button::South, QStringLiteral("A")},
        {Button::East, QStringLiteral("B")},
        {Button::West, QStringLiteral("X")},
        {Button::North, QStringLiteral("Y")},
        {Button::LeftShoulder, QStringLiteral("LB")},
        {Button::RightShoulder, QStringLiteral("RB")},
        {Button::LeftTrigger, QStringLiteral("LT")},
        {Button::RightTrigger, QStringLiteral("RT")},
        {Button::Back, QStringLiteral("Back")},
        {Button::Start, QStringLiteral("Start")},
        {Button::LeftStick, QStringLiteral("LS")},
        {Button::RightStick, QStringLiteral("RS")},
        {Button::DPadUp, QStringLiteral("Up")},
        {Button::DPadDown, QStringLiteral("Down")},
        {Button::DPadLeft, QStringLiteral("Left")},
        {Button::DPadRight, QStringLiteral("Right")},
    };
    QSet<QString> buttons;
    for (const auto &[button, name] : names) {
        if (pad.pressed(button))
            buttons.insert(name);
    }
    // QOR.Input's sticks point down for +y, QQ's up; its triggers count as pressed past half way.
    padState(QVector2D(pad.axis(Axis::LeftX), -pad.axis(Axis::LeftY)),
             QVector2D(pad.axis(Axis::RightX), -pad.axis(Axis::RightY)), buttons, true, true);
#endif
}

}  // namespace qq
