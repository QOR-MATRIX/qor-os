// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "gamepad.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/QQuickWindow>

namespace qor::input {

// The controller interface: drives a window with a game controller as a
// person would with a mouse and keyboard, so every surface, app and Wayland
// client works without changes.
//
//   Left stick    a cursor (accelerating; `cursorSpeed`)
//   South (A/✕)   click, or activate the focused control after D-pad moves
//   West (X/□)    right click
//   East (B/○)    back (Escape)
//   D-pad         arrow keys (Tab and Shift+Tab when nothing takes them)
//   Right stick   scroll
//   North (Y/△), Start, Back, Guide, shoulders, triggers, stick clicks:
//                 `action()` for the shell ("search", "controlCenter",
//                 "overview", "home", "back", "previousSpace", "nextSpace",
//                 "volumeDown", "volumeUp", "keyboard", "playPause")
//
// While `keyboardOpen`, the D-pad, left stick and face buttons go to the
// on-screen keyboard through keyboardMove() and keyboardButton() instead.
// Pointer and key events enter through QWindowSystemInterface, like a
// device's, so they behave exactly like real input.
class ControllerNavigator : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(qreal cursorSpeed READ cursorSpeed WRITE setCursorSpeed NOTIFY cursorSpeedChanged FINAL)
    Q_PROPERTY(bool vibration READ vibration WRITE setVibration NOTIFY vibrationChanged FINAL)
    Q_PROPERTY(bool keyboardOpen READ keyboardOpen WRITE setKeyboardOpen NOTIFY keyboardOpenChanged FINAL)
    Q_PROPERTY(QPointF cursor READ cursor NOTIFY cursorChanged FINAL)
    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged FINAL)
    Q_PROPERTY(bool cursorPressed READ cursorPressed NOTIFY cursorPressedChanged FINAL)
    Q_PROPERTY(bool textInputFocused READ textInputFocused NOTIFY textInputFocusedChanged FINAL)

public:
    explicit ControllerNavigator(QObject *parent = nullptr);
    ~ControllerNavigator() override;

    QQuickWindow *window() const { return m_window; }
    void setWindow(QQuickWindow *window);
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    qreal cursorSpeed() const { return m_cursorSpeed; }
    void setCursorSpeed(qreal speed);
    bool vibration() const { return m_vibration; }
    void setVibration(bool vibration);
    bool keyboardOpen() const { return m_keyboardOpen; }
    void setKeyboardOpen(bool open);
    QPointF cursor() const { return m_cursor; }
    bool cursorVisible() const { return m_cursorVisible; }
    bool cursorPressed() const { return m_pressed; }
    bool textInputFocused() const { return m_textInputFocused; }

    // Types text into the focused item, as a keyboard would.
    Q_INVOKABLE void typeText(const QString &text);
    // Presses and releases a key (Qt::Key) with modifiers.
    Q_INVOKABLE void sendKey(int key, int modifiers = 0);
    Q_INVOKABLE void warpCursor(const QPointF &position);
    // A short vibration, when vibration is on.
    Q_INVOKABLE void feedback(qreal strength = 0.35, int milliseconds = 35);

Q_SIGNALS:
    void windowChanged();
    void enabledChanged();
    void cursorSpeedChanged();
    void vibrationChanged();
    void keyboardOpenChanged();
    void cursorChanged();
    void cursorVisibleChanged();
    void cursorPressedChanged();
    void textInputFocusedChanged();
    void action(const QString &name);
    void keyboardMove(int dx, int dy);
    void keyboardButton(int button);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void onMerged(const GamepadState &now, const GamepadState &before);
    void press(Button button);
    void release(Button button);
    void fire(Button button, bool repeated);
    void tick();
    void updateTicking();
    void moveCursor(const QPointF &to);
    void setCursorVisible(bool visible);
    void injectMouse(QEvent::Type type, Qt::MouseButton button);
    bool injectKey(int key, Qt::KeyboardModifiers modifiers, const QString &text = {});
    void activateFocus();
    void updateTextInput();
    void releaseAll();

    QPointer<QQuickWindow> m_window;
    bool m_enabled = false;
    qreal m_cursorSpeed = 0.5;
    bool m_vibration = true;
    bool m_keyboardOpen = false;
    QPointF m_cursor;
    bool m_cursorVisible = false;
    bool m_pressed = false;
    bool m_textInputFocused = false;
    bool m_focusMode = false; // the D-pad moved focus since the stick last moved
    bool m_hidSystemCursor = false;
    Qt::MouseButtons m_buttons;

    QTimer m_tick;
    QElapsedTimer m_clock;
    QElapsedTimer m_inputClock;
    QPointF m_scroll;
    // Held buttons that repeat (D-pad, triggers, keyboard moves).
    int m_repeating = -1;
    qint64 m_repeatAt = 0;
    // The direction the left stick last flicked the keyboard selection.
    QPoint m_flick;
    qint64 m_flickAt = 0;
};

} // namespace qor::input
