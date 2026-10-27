// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include <array>
#include <cmath>

// The QOR OS controller model: every supported controller, whatever its driver,
// is presented as one standard layout (the Xbox / "south-east-west-north"
// layout that SDL, the Linux gamepad spec and the W3C Gamepad API share).
namespace qor::input {

enum class Button : int {
    South,         // A, Cross
    East,          // B, Circle
    West,          // X, Square
    North,         // Y, Triangle
    LeftShoulder,  // LB, L1
    RightShoulder, // RB, R1
    LeftTrigger,   // LT, L2 (pressed past half way)
    RightTrigger,  // RT, R2
    Back,          // View, Share, Create, Select, Minus
    Start,         // Menu, Options, Plus
    Guide,         // Xbox, PS, Home
    LeftStick,     // L3
    RightStick,    // R3
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Touchpad, // PlayStation touchpad click
    Misc,     // Share (Xbox Series), Mute (DualSense), Capture
    Count
};

enum class Axis : int {
    LeftX, // -1 left .. 1 right
    LeftY, // -1 up .. 1 down
    RightX,
    RightY,
    LeftTrigger,  // 0 .. 1
    RightTrigger, // 0 .. 1
    Count
};

// Decides button glyphs (A/B/X/Y or Cross/Circle/Square/Triangle).
enum class Family : int { Generic, Xbox, PlayStation, Nintendo };

constexpr quint32 bit(Button button) { return 1u << int(button); }

struct GamepadState
{
    quint32 buttons = 0;
    std::array<float, int(Axis::Count)> axes {};

    bool pressed(Button button) const { return buttons & bit(button); }
    void set(Button button, bool down)
    {
        if (down)
            buttons |= bit(button);
        else
            buttons &= ~bit(button);
    }
    float axis(Axis a) const { return axes[std::size_t(a)]; }
    float &axis(Axis a) { return axes[std::size_t(a)]; }

    // Triggers also count as buttons, with hysteresis against flutter.
    void deriveTriggerButtons()
    {
        set(Button::LeftTrigger, axis(Axis::LeftTrigger) > (pressed(Button::LeftTrigger) ? 0.35f : 0.5f));
        set(Button::RightTrigger, axis(Axis::RightTrigger) > (pressed(Button::RightTrigger) ? 0.35f : 0.5f));
    }

    friend bool operator==(const GamepadState &, const GamepadState &) = default;
};

struct GamepadInfo
{
    QString id;   // stable for the connection, e.g. "xinput:0", "hid:<path>"
    QString name; // "DualShock 4", "Xbox Wireless Controller"
    Family family = Family::Generic;
    QString connection; // "USB", "Bluetooth", "Wireless", "Virtual"
    quint16 vendor = 0;
    quint16 product = 0;
    int battery = -1; // percent; -1 unknown (wired controllers report none)
    bool charging = false;
    bool rumble = false;   // can vibrate
    bool lightbar = false; // has a colour light (PlayStation)

    friend bool operator==(const GamepadInfo &, const GamepadInfo &) = default;
};

// Radial dead zone with the live range rescaled to start at 0, so small stick
// drift reads exactly 0 and full deflection still reads 1.
inline void applyStickDeadZone(float &x, float &y, float deadZone = 0.16f)
{
    const float magnitude = std::sqrt(x * x + y * y);
    if (magnitude <= deadZone) {
        x = y = 0;
        return;
    }
    const float scaled = std::min(1.0f, (magnitude - deadZone) / (1.0f - deadZone));
    x = x / magnitude * scaled;
    y = y / magnitude * scaled;
}

inline float applyTriggerDeadZone(float value, float deadZone = 0.04f)
{
    return value <= deadZone ? 0.0f : std::min(1.0f, (value - deadZone) / (1.0f - deadZone));
}

// Converts a hat switch (0 = up, clockwise to 7, anything else = centred).
inline void setHat(GamepadState &state, int hat)
{
    state.set(Button::DPadUp, hat == 0 || hat == 1 || hat == 7);
    state.set(Button::DPadRight, hat >= 1 && hat <= 3);
    state.set(Button::DPadDown, hat >= 3 && hat <= 5);
    state.set(Button::DPadLeft, hat >= 5 && hat <= 7);
}

} // namespace qor::input

Q_DECLARE_METATYPE(qor::input::GamepadState)
Q_DECLARE_METATYPE(qor::input::GamepadInfo)
