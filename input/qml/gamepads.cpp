// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "gamepads.h"

#include "gamepadmanager.h"

#include <QtCore/QCoreApplication>
#include <QtQml/QJSEngine>

using namespace Qt::StringLiterals;

namespace qtos::input {

Gamepads::Gamepads(QObject *parent) : QObject(parent)
{
    GamepadManager *manager = GamepadManager::instance();
    connect(manager, &GamepadManager::connected, this, [this](const GamepadInfo &info) {
        Q_EMIT gamepadsChanged();
        Q_EMIT connected(describe(info));
    });
    connect(manager, &GamepadManager::disconnected, this, [this](const GamepadInfo &info) {
        Q_EMIT gamepadsChanged();
        Q_EMIT disconnected(describe(info));
    });
    connect(manager, &GamepadManager::infoChanged, this, [this] {
        Q_EMIT gamepadsChanged();
        Q_EMIT activeChanged();
    });
    connect(manager, &GamepadManager::activeChanged, this, &Gamepads::activeChanged);
    connect(manager, &GamepadManager::mergedChanged, this, [this](const GamepadState &now, const GamepadState &before) {
        Q_EMIT stateChanged();
        const quint32 changed = now.buttons ^ before.buttons;
        for (int b = 0; b < int(qtos::input::Button::Count); ++b) {
            if (!(changed & (1u << b)))
                continue;
            if (now.buttons & (1u << b))
                Q_EMIT pressed(b);
            else
                Q_EMIT released(b);
        }
    });
}

Gamepads *Gamepads::instance()
{
    static Gamepads *gamepads = new Gamepads(qApp);
    return gamepads;
}

Gamepads *Gamepads::create(QQmlEngine *, QJSEngine *)
{
    Gamepads *gamepads = instance();
    QJSEngine::setObjectOwnership(gamepads, QJSEngine::CppOwnership);
    return gamepads;
}

QString Gamepads::familyName(Family family)
{
    switch (family) {
    case Family::Xbox: return u"xbox"_s;
    case Family::PlayStation: return u"playstation"_s;
    case Family::Nintendo: return u"nintendo"_s;
    case Family::Generic: break;
    }
    return u"generic"_s;
}

QVariantMap Gamepads::describe(const GamepadInfo &info)
{
    return {
        { u"id"_s, info.id },
        { u"name"_s, info.name },
        { u"family"_s, familyName(info.family) },
        { u"connection"_s, info.connection },
        { u"battery"_s, info.battery },
        { u"charging"_s, info.charging },
        { u"rumble"_s, info.rumble },
        { u"lightbar"_s, info.lightbar },
    };
}

int Gamepads::count() const { return int(GamepadManager::instance()->gamepads().size()); }

QVariantList Gamepads::gamepads() const
{
    QVariantList list;
    for (const GamepadInfo &info : GamepadManager::instance()->gamepads())
        list.append(describe(info));
    return list;
}

QVariantMap Gamepads::active() const
{
    const auto info = GamepadManager::instance()->gamepad(GamepadManager::instance()->activeId());
    return info ? describe(*info) : QVariantMap();
}

int Gamepads::buttons() const { return int(GamepadManager::instance()->merged().buttons); }

QPointF Gamepads::leftStick() const
{
    const GamepadState state = GamepadManager::instance()->merged();
    return { state.axis(Axis::LeftX), state.axis(Axis::LeftY) };
}

QPointF Gamepads::rightStick() const
{
    const GamepadState state = GamepadManager::instance()->merged();
    return { state.axis(Axis::RightX), state.axis(Axis::RightY) };
}

qreal Gamepads::leftTrigger() const { return GamepadManager::instance()->merged().axis(Axis::LeftTrigger); }

qreal Gamepads::rightTrigger() const { return GamepadManager::instance()->merged().axis(Axis::RightTrigger); }

QStringList Gamepads::backends() const { return GamepadManager::instance()->backendNames(); }

bool Gamepads::isPressed(int button) const
{
    return button >= 0 && button < int(qtos::input::Button::Count)
           && GamepadManager::instance()->merged().pressed(qtos::input::Button(button));
}

void Gamepads::rumble(qreal strong, qreal weak, int milliseconds)
{
    GamepadManager::instance()->rumble(float(strong), float(weak), milliseconds);
}

void Gamepads::setLightColor(const QColor &color) { GamepadManager::instance()->setLightColor(color); }

QString Gamepads::buttonName(int button, const QString &family) const
{
    const bool playstation = family == u"playstation"_s;
    const bool nintendo = family == u"nintendo"_s;
    switch (Button(button)) {
    case South: return playstation ? u"Cross"_s : nintendo ? u"B"_s : u"A"_s;
    case East: return playstation ? u"Circle"_s : nintendo ? u"A"_s : u"B"_s;
    case West: return playstation ? u"Square"_s : nintendo ? u"Y"_s : u"X"_s;
    case North: return playstation ? u"Triangle"_s : nintendo ? u"X"_s : u"Y"_s;
    case LeftShoulder: return playstation ? u"L1"_s : nintendo ? u"L"_s : u"LB"_s;
    case RightShoulder: return playstation ? u"R1"_s : nintendo ? u"R"_s : u"RB"_s;
    case LeftTrigger: return playstation ? u"L2"_s : nintendo ? u"ZL"_s : u"LT"_s;
    case RightTrigger: return playstation ? u"R2"_s : nintendo ? u"ZR"_s : u"RT"_s;
    case Back: return playstation ? u"Share"_s : nintendo ? u"−"_s : u"View"_s;
    case Start: return playstation ? u"Options"_s : nintendo ? u"+"_s : u"Menu"_s;
    case Guide: return playstation ? u"PS"_s : nintendo ? u"Home"_s : family == u"xbox"_s ? u"Xbox"_s : u"Home"_s;
    case LeftStick: return playstation ? u"L3"_s : u"LS"_s;
    case RightStick: return playstation ? u"R3"_s : u"RS"_s;
    case DPadUp: return u"Up"_s;
    case DPadDown: return u"Down"_s;
    case DPadLeft: return u"Left"_s;
    case DPadRight: return u"Right"_s;
    case Touchpad: return u"Touchpad"_s;
    case Misc: return playstation ? u"Mute"_s : u"Share"_s;
    }
    return {};
}

} // namespace qtos::input
