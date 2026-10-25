// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "gamepad.h"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVariantList>
#include <QtGui/QColor>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace qtos::input {

// The connected game controllers, for QML: the `Gamepads` singleton.
//
//   Gamepads.count, Gamepads.gamepads, Gamepads.active
//   Gamepads.buttons / leftStick / rightStick / leftTrigger / rightTrigger
//       (every controller combined)
//   onConnected(gamepad), onDisconnected(gamepad), onPressed(button),
//   onReleased(button)
//
// A gamepad is { id, name, family: "xbox" | "playstation" | "nintendo" |
// "generic", connection, battery (percent or -1), charging, rumble,
// lightbar }.
class Gamepads : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int count READ count NOTIFY gamepadsChanged FINAL)
    Q_PROPERTY(QVariantList gamepads READ gamepads NOTIFY gamepadsChanged FINAL)
    Q_PROPERTY(QVariantMap active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(int buttons READ buttons NOTIFY stateChanged FINAL)
    Q_PROPERTY(QPointF leftStick READ leftStick NOTIFY stateChanged FINAL)
    Q_PROPERTY(QPointF rightStick READ rightStick NOTIFY stateChanged FINAL)
    Q_PROPERTY(qreal leftTrigger READ leftTrigger NOTIFY stateChanged FINAL)
    Q_PROPERTY(qreal rightTrigger READ rightTrigger NOTIFY stateChanged FINAL)
    Q_PROPERTY(QStringList backends READ backends CONSTANT FINAL)

public:
    enum Button {
        South = int(qtos::input::Button::South),
        East,
        West,
        North,
        LeftShoulder,
        RightShoulder,
        LeftTrigger,
        RightTrigger,
        Back,
        Start,
        Guide,
        LeftStick,
        RightStick,
        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,
        Touchpad,
        Misc
    };
    Q_ENUM(Button)

    static Gamepads *create(QQmlEngine *, QJSEngine *);
    static Gamepads *instance();

    int count() const;
    QVariantList gamepads() const;
    QVariantMap active() const;
    int buttons() const;
    QPointF leftStick() const;
    QPointF rightStick() const;
    qreal leftTrigger() const;
    qreal rightTrigger() const;
    QStringList backends() const;

    Q_INVOKABLE bool isPressed(int button) const;
    Q_INVOKABLE void rumble(qreal strong, qreal weak, int milliseconds);
    Q_INVOKABLE void setLightColor(const QColor &color);
    // The name printed on a button for a controller family ("A", "Cross",
    // "LB", "L1", "Menu", "Options", ...).
    Q_INVOKABLE QString buttonName(int button, const QString &family) const;

    static QVariantMap describe(const GamepadInfo &info);
    static QString familyName(Family family);

Q_SIGNALS:
    void gamepadsChanged();
    void activeChanged();
    void stateChanged();
    void connected(const QVariantMap &gamepad);
    void disconnected(const QVariantMap &gamepad);
    void pressed(int button);
    void released(int button);

private:
    explicit Gamepads(QObject *parent = nullptr);
};

} // namespace qtos::input
