// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "navigator.h"

#include "gamepadmanager.h"
#include "injection.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodQueryEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <algorithm>
#include <cmath>

namespace qor::input {

namespace {

constexpr int kTickMs = 8;
constexpr int kRepeatDelayMs = 380;
constexpr int kRepeatIntervalMs = 90;
constexpr int kVolumeRepeatIntervalMs = 140;

// Wheel units per pixel: Qt Quick scrolls about 72 px per 120 units.
constexpr double kAnglePerPixel = 120.0 / 72.0;

bool isRepeating(Button button)
{
    switch (button) {
    case Button::DPadUp:
    case Button::DPadDown:
    case Button::DPadLeft:
    case Button::DPadRight:
    case Button::LeftTrigger:
    case Button::RightTrigger: return true;
    default: return false;
    }
}

QPoint direction(Button button)
{
    switch (button) {
    case Button::DPadUp: return { 0, -1 };
    case Button::DPadDown: return { 0, 1 };
    case Button::DPadLeft: return { -1, 0 };
    case Button::DPadRight: return { 1, 0 };
    default: return {};
    }
}

} // namespace

ControllerNavigator::ControllerNavigator(QObject *parent) : QObject(parent)
{
    m_tick.setTimerType(Qt::PreciseTimer);
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &ControllerNavigator::tick);
    connect(GamepadManager::instance(), &GamepadManager::mergedChanged, this, &ControllerNavigator::onMerged);
    connect(qApp, &QGuiApplication::focusObjectChanged, this, &ControllerNavigator::updateTextInput);
    m_inputClock.start();
}

ControllerNavigator::~ControllerNavigator()
{
    if (m_hidSystemCursor)
        QGuiApplication::restoreOverrideCursor();
}

void ControllerNavigator::setWindow(QQuickWindow *window)
{
    if (m_window == window)
        return;
    if (m_window)
        m_window->removeEventFilter(this);
    m_window = window;
    if (m_window) {
        m_window->installEventFilter(this);
        m_cursor = QPointF(m_window->width() / 2.0, m_window->height() / 2.0);
        Q_EMIT cursorChanged();
    }
    Q_EMIT windowChanged();
}

void ControllerNavigator::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!enabled) {
        releaseAll();
        setCursorVisible(false);
    }
    updateTicking();
    Q_EMIT enabledChanged();
}

void ControllerNavigator::setCursorSpeed(qreal speed)
{
    speed = std::clamp(speed, 0.0, 1.0);
    if (qFuzzyCompare(m_cursorSpeed, speed))
        return;
    m_cursorSpeed = speed;
    Q_EMIT cursorSpeedChanged();
}

void ControllerNavigator::setVibration(bool vibration)
{
    if (m_vibration == vibration)
        return;
    m_vibration = vibration;
    Q_EMIT vibrationChanged();
}

void ControllerNavigator::setKeyboardOpen(bool open)
{
    if (m_keyboardOpen == open)
        return;
    m_keyboardOpen = open;
    m_repeating = -1;
    m_flick = {};
    Q_EMIT keyboardOpenChanged();
}

// ── Buttons ─────────────────────────────────────────────────────────────────

void ControllerNavigator::onMerged(const GamepadState &now, const GamepadState &before)
{
    if (!m_enabled || !m_window)
        return;
    m_inputClock.restart();
    // Using the controller brings the shell forward (the OS may decline in
    // a desktop simulation; on a device the shell is all there is).
    if (now.buttons != before.buttons && QGuiApplication::focusWindow() != m_window)
        m_window->requestActivate();
    const quint32 pressedNow = now.buttons & ~before.buttons;
    const quint32 releasedNow = before.buttons & ~now.buttons;
    for (int b = 0; b < int(Button::Count); ++b) {
        if (releasedNow & (1u << b))
            release(Button(b));
        if (pressedNow & (1u << b))
            press(Button(b));
    }
    updateTicking();
}

void ControllerNavigator::press(Button button)
{
    // Held D-pad and triggers repeat (and Backspace on the keyboard).
    if (isRepeating(button) || (m_keyboardOpen && button == Button::West)) {
        m_repeating = int(button);
        m_repeatAt = m_inputClock.elapsed() + kRepeatDelayMs;
    }
    fire(button, false);
}

void ControllerNavigator::release(Button button)
{
    if (m_repeating == int(button))
        m_repeating = -1;
    if ((button == Button::South || button == Button::Touchpad) && m_pressed) {
        injectMouse(QEvent::MouseButtonRelease, Qt::LeftButton);
        m_pressed = false;
        Q_EMIT cursorPressedChanged();
    }
}

void ControllerNavigator::fire(Button button, bool repeated)
{
    // The on-screen keyboard takes navigation and the face buttons.
    if (m_keyboardOpen) {
        const QPoint d = direction(button);
        if (!d.isNull()) {
            Q_EMIT keyboardMove(d.x(), d.y());
            return;
        }
        switch (button) {
        case Button::South:
        case Button::East:
        case Button::West:
        case Button::North:
        case Button::Start:
        case Button::LeftShoulder:
        case Button::RightShoulder:
        case Button::LeftTrigger:
        case Button::RightTrigger: Q_EMIT keyboardButton(int(button)); return;
        default: break;
        }
    }

    switch (button) {
    case Button::South:
    case Button::Touchpad:
        if (m_focusMode && !m_cursorVisible && button == Button::South) {
            activateFocus();
        } else {
            setCursorVisible(true);
            m_pressed = true;
            Q_EMIT cursorPressedChanged();
            injectMouse(QEvent::MouseButtonPress, Qt::LeftButton);
        }
        feedback(0.2, 25);
        break;
    case Button::West:
        setCursorVisible(true);
        injectMouse(QEvent::MouseButtonPress, Qt::RightButton);
        injectMouse(QEvent::MouseButtonRelease, Qt::RightButton);
        feedback(0.2, 25);
        break;
    case Button::East:
        // Escape for whatever has focus; if nothing takes it (or the window
        // lacks the OS's focus, so shortcuts cannot fire), the shell's back.
        if (!injectKey(Qt::Key_Escape, Qt::NoModifier, QStringLiteral("\x1b")))
            Q_EMIT action(QStringLiteral("back"));
        break;
    case Button::North: Q_EMIT action(QStringLiteral("search")); break;
    case Button::Start: Q_EMIT action(QStringLiteral("controlCenter")); break;
    case Button::Back: Q_EMIT action(QStringLiteral("overview")); break;
    case Button::Guide: Q_EMIT action(QStringLiteral("home")); break;
    case Button::LeftShoulder: Q_EMIT action(QStringLiteral("previousSpace")); break;
    case Button::RightShoulder: Q_EMIT action(QStringLiteral("nextSpace")); break;
    case Button::LeftTrigger: Q_EMIT action(QStringLiteral("volumeDown")); break;
    case Button::RightTrigger: Q_EMIT action(QStringLiteral("volumeUp")); break;
    case Button::LeftStick: Q_EMIT action(QStringLiteral("keyboard")); break;
    case Button::RightStick: Q_EMIT action(QStringLiteral("playPause")); break;
    case Button::DPadUp:
    case Button::DPadDown:
    case Button::DPadLeft:
    case Button::DPadRight: {
        m_focusMode = true;
        setCursorVisible(false);
        const int key = button == Button::DPadUp     ? Qt::Key_Up
                        : button == Button::DPadDown ? Qt::Key_Down
                        : button == Button::DPadLeft ? Qt::Key_Left
                                                     : Qt::Key_Right;
        // Lists and fields take arrows; elsewhere move focus as Tab does.
        if (!injectKey(key, Qt::NoModifier)) {
            const bool forward = key == Qt::Key_Down || key == Qt::Key_Right;
            injectKey(forward ? Qt::Key_Tab : Qt::Key_Backtab, forward ? Qt::NoModifier : Qt::ShiftModifier);
        }
        break;
    }
    default: break;
    }
    Q_UNUSED(repeated);
}

void ControllerNavigator::activateFocus()
{
    QObject *focus = QGuiApplication::focusObject();
    bool button = false;
    for (const QMetaObject *meta = focus ? focus->metaObject() : nullptr; meta; meta = meta->superClass()) {
        if (qstrcmp(meta->className(), "QQuickAbstractButton") == 0) {
            button = true;
            break;
        }
    }
    // Buttons take Space; text fields, lists and everything else Return.
    if (button && !m_textInputFocused)
        injectKey(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
    else
        injectKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
}

// ── Continuous input: sticks and repeats ────────────────────────────────────

void ControllerNavigator::updateTicking()
{
    const GamepadState state = GamepadManager::instance()->merged();
    const bool sticks = state.axis(Axis::LeftX) != 0 || state.axis(Axis::LeftY) != 0 || state.axis(Axis::RightX) != 0
                        || state.axis(Axis::RightY) != 0;
    const bool needed = m_enabled && m_window && (sticks || m_repeating >= 0);
    if (needed && !m_tick.isActive()) {
        m_clock.restart();
        m_tick.start();
    } else if (!needed && m_tick.isActive()) {
        m_tick.stop();
        m_scroll = {};
    }
}

void ControllerNavigator::tick()
{
    if (!m_window) {
        m_tick.stop();
        return;
    }
    const double dt = std::clamp(double(m_clock.restart()) / 1000.0, 0.0, 0.05);
    const GamepadState state = GamepadManager::instance()->merged();
    const qint64 now = m_inputClock.elapsed();

    const QPointF left(state.axis(Axis::LeftX), state.axis(Axis::LeftY));
    const double leftMagnitude = std::hypot(left.x(), left.y());
    if (m_keyboardOpen) {
        // The stick flicks the keyboard selection, repeating while held.
        if (leftMagnitude > 0.6) {
            const QPoint flick = std::abs(left.x()) > std::abs(left.y()) ? QPoint(left.x() > 0 ? 1 : -1, 0)
                                                                         : QPoint(0, left.y() > 0 ? 1 : -1);
            if (flick != m_flick || now >= m_flickAt) {
                m_flickAt = now + (flick != m_flick ? kRepeatDelayMs : kRepeatIntervalMs + 40);
                m_flick = flick;
                Q_EMIT keyboardMove(flick.x(), flick.y());
            }
        } else if (leftMagnitude < 0.4) {
            m_flick = {};
        }
    } else if (leftMagnitude > 0) {
        m_focusMode = false;
        // Precise near the centre, fast at the rim.
        const double speed = (420.0 + 1700.0 * m_cursorSpeed) * std::pow(leftMagnitude, 1.8);
        moveCursor(m_cursor + left / leftMagnitude * speed * dt);
    }

    const QPointF right(state.axis(Axis::RightX), state.axis(Axis::RightY));
    const double rightMagnitude = std::hypot(right.x(), right.y());
    if (rightMagnitude > 0 && !m_keyboardOpen) {
        m_scroll += right * (1600.0 * std::pow(rightMagnitude, 1.5) * dt);
        const QPoint pixels(int(m_scroll.x()), int(m_scroll.y()));
        if (!pixels.isNull()) {
            m_scroll -= QPointF(pixels);
            const QPoint angle(int(-pixels.x() * kAnglePerPixel), int(-pixels.y() * kAnglePerPixel));
            QWindow *window = m_window.data();
            const QPointF position =
                m_cursorVisible ? m_cursor : QPointF(window->width() / 2.0, window->height() / 2.0);
            const InjectedInput injected;
            QWindowSystemInterface::handleWheelEvent(
                window, ulong(now), QHighDpi::toNativeLocalPosition(position, window),
                QHighDpi::toNativeGlobalPosition(window->mapToGlobal(position), window), QPoint(), angle);
        }
    }

    if (m_repeating >= 0 && now >= m_repeatAt) {
        const Button button = Button(m_repeating);
        m_repeatAt = now
                     + (button == Button::LeftTrigger || button == Button::RightTrigger ? kVolumeRepeatIntervalMs
                                                                                        : kRepeatIntervalMs);
        fire(button, true);
    }
    updateTicking();
}

// ── Pointer ─────────────────────────────────────────────────────────────────

void ControllerNavigator::moveCursor(const QPointF &to)
{
    if (!m_window)
        return;
    const QPointF clamped(std::clamp(to.x(), 0.0, double(m_window->width() - 1)),
                          std::clamp(to.y(), 0.0, double(m_window->height() - 1)));
    setCursorVisible(true);
    if (clamped == m_cursor)
        return;
    m_cursor = clamped;
    Q_EMIT cursorChanged();
    injectMouse(QEvent::MouseMove, Qt::NoButton);
}

void ControllerNavigator::warpCursor(const QPointF &position)
{
    m_focusMode = false;
    moveCursor(position);
}

void ControllerNavigator::setCursorVisible(bool visible)
{
    if (m_cursorVisible == visible)
        return;
    m_cursorVisible = visible;
    // The controller's cursor replaces the system one while it shows.
    if (visible && !m_hidSystemCursor) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        m_hidSystemCursor = true;
    } else if (!visible && m_hidSystemCursor) {
        QGuiApplication::restoreOverrideCursor();
        m_hidSystemCursor = false;
    }
    Q_EMIT cursorVisibleChanged();
}

void ControllerNavigator::injectMouse(QEvent::Type type, Qt::MouseButton button)
{
    if (!m_window)
        return;
    if (type == QEvent::MouseButtonPress)
        m_buttons |= button;
    else if (type == QEvent::MouseButtonRelease)
        m_buttons &= ~button;
    QWindow *window = m_window.data();
    const InjectedInput injected;
    QWindowSystemInterface::handleMouseEvent(
        window, ulong(m_inputClock.elapsed()), QHighDpi::toNativeLocalPosition(m_cursor, window),
        QHighDpi::toNativeGlobalPosition(window->mapToGlobal(m_cursor), window), m_buttons, button, type);
}

void ControllerNavigator::releaseAll()
{
    if (m_pressed) {
        injectMouse(QEvent::MouseButtonRelease, Qt::LeftButton);
        m_pressed = false;
        Q_EMIT cursorPressedChanged();
    }
    m_repeating = -1;
    m_scroll = {};
}

bool ControllerNavigator::eventFilter(QObject *watched, QEvent *event)
{
    // A real mouse takes over from the controller's cursor (and leaves it
    // where the mouse is).
    if (!isInjecting() && m_cursorVisible
        && (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress)) {
        m_cursor = static_cast<QMouseEvent *>(event)->scenePosition();
        Q_EMIT cursorChanged();
        setCursorVisible(false);
    }
    return QObject::eventFilter(watched, event);
}

// ── Keys and text ───────────────────────────────────────────────────────────

bool ControllerNavigator::injectKey(int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    if (!m_window)
        return false;
    // Delivered synchronously, so the result says whether anything took it.
    return sendKeyStroke(m_window.data(), ulong(m_inputClock.elapsed()), key, modifiers, text);
}

void ControllerNavigator::sendKey(int key, int modifiers)
{
    QString text;
    if (key == Qt::Key_Return || key == Qt::Key_Enter)
        text = QStringLiteral("\r");
    else if (key == Qt::Key_Escape)
        text = QStringLiteral("\x1b");
    else if (key == Qt::Key_Backspace)
        text = QStringLiteral("\b");
    else if (key == Qt::Key_Space)
        text = QStringLiteral(" ");
    injectKey(key, Qt::KeyboardModifiers(modifiers), text);
}

void ControllerNavigator::typeText(const QString &text)
{
    for (const QChar ch : text) {
        int key = Qt::Key_unknown;
        Qt::KeyboardModifiers modifiers;
        if (ch.isLetter() && ch.unicode() < 128) {
            key = Qt::Key_A + (ch.toLower().unicode() - 'a');
            if (ch.isUpper())
                modifiers = Qt::ShiftModifier;
        } else if (ch.isDigit() && ch.unicode() < 128) {
            key = Qt::Key_0 + (ch.unicode() - '0');
        } else if (ch == u' ') {
            key = Qt::Key_Space;
        } else if (ch.unicode() < 128) {
            key = ch.unicode(); // Qt keys for ASCII punctuation are their code points
        }
        injectKey(key, modifiers, QString(ch));
    }
}

void ControllerNavigator::updateTextInput()
{
    bool text = false;
    if (QObject *focus = QGuiApplication::focusObject()) {
        QInputMethodQueryEvent query(Qt::ImEnabled | Qt::ImReadOnly);
        QCoreApplication::sendEvent(focus, &query);
        text = query.value(Qt::ImEnabled).toBool() && !query.value(Qt::ImReadOnly).toBool();
    }
    if (text == m_textInputFocused)
        return;
    m_textInputFocused = text;
    Q_EMIT textInputFocusedChanged();
}

void ControllerNavigator::feedback(qreal strength, int milliseconds)
{
    if (m_vibration && m_enabled)
        GamepadManager::instance()->rumble(float(strength), float(strength * 0.7), milliseconds);
}

} // namespace qor::input
