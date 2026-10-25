// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "gamepad.h"

#include <QtCore/QList>

#include <array>
#include <optional>

// Translations from each platform's controller vocabulary to the standard
// QTOS layout. Free of platform headers so every mapping is unit tested on
// every platform.
namespace qtos::input {

// ── XInput (Windows) ─────────────────────────────────────────────────────────

// The fields of XINPUT_GAMEPAD; `buttons` may include the Guide bit (0x0400)
// that XInputGetStateEx reports.
struct XInputPad
{
    quint16 buttons = 0;
    quint8 leftTrigger = 0;
    quint8 rightTrigger = 0;
    qint16 thumbLX = 0;
    qint16 thumbLY = 0;
    qint16 thumbRX = 0;
    qint16 thumbRY = 0;
};

GamepadState mapXInput(const XInputPad &pad);

// ── Linux evdev ─────────────────────────────────────────────────────────────

// Event codes from <linux/input-event-codes.h>.
namespace evdev {
constexpr int EvSyn = 0x00, EvKey = 0x01, EvAbs = 0x03;
constexpr int SynReport = 0, SynDropped = 3;
constexpr int BtnSouth = 0x130, BtnEast = 0x131, BtnC = 0x132, BtnNorth = 0x133, BtnWest = 0x134, BtnZ = 0x135;
constexpr int BtnTL = 0x136, BtnTR = 0x137, BtnTL2 = 0x138, BtnTR2 = 0x139;
constexpr int BtnSelect = 0x13a, BtnStart = 0x13b, BtnMode = 0x13c, BtnThumbL = 0x13d, BtnThumbR = 0x13e;
constexpr int BtnDpadUp = 0x220, BtnDpadDown = 0x221, BtnDpadLeft = 0x222, BtnDpadRight = 0x223;
constexpr int AbsX = 0x00, AbsY = 0x01, AbsZ = 0x02, AbsRx = 0x03, AbsRy = 0x04, AbsRz = 0x05;
constexpr int AbsGas = 0x09, AbsBrake = 0x0a, AbsHat0X = 0x10, AbsHat0Y = 0x11;
constexpr int AbsCount = 0x40;
} // namespace evdev

struct AbsRange
{
    int minimum = 0;
    int maximum = 0;
    bool valid() const { return maximum > minimum; }
};

// Accumulates evdev events into a state, committed on SYN_REPORT.
class EvdevMapper
{
public:
    // `ranges` by ABS code (missing axes have an invalid range). xpad
    // (Xbox controllers) reports its X and Y buttons as BTN_X / BTN_Y,
    // i.e. swapped against the gamepad spec's north and west.
    EvdevMapper(const std::array<AbsRange, evdev::AbsCount> &ranges, bool swapNorthWest);

    // Returns true when a SYN_REPORT completed a new state.
    bool handle(int type, int code, int value);
    const GamepadState &state() const { return m_state; }

private:
    float normalised(int code, int value, bool centred) const;

    std::array<AbsRange, evdev::AbsCount> m_ranges;
    bool m_swapNorthWest;
    bool m_rightStickIsRxRy;
    bool m_dropping = false;
    GamepadState m_pending;
    GamepadState m_state;
};

// ── Generic HID gamepads ────────────────────────────────────────────────────

// One report of a gamepad the system knows only as a HID game controller:
// pressed buttons (HID usages, 1-based) and the usages it reports values for,
// already normalised (sticks -1..1, triggers 0..1, hat 0..7 or -1).
struct GenericHidReport
{
    QList<int> pressed;
    int buttonCount = 0;
    std::optional<float> x, y, z, rx, ry, rz, brake, accelerator;
    int hat = -1;
};

// Best effort, as SDL does without a mapping database: Android-style pads
// (15 or more buttons: A B C X Y Z L1 R1 L2 R2 Select Start Mode L3 R3) and
// DirectInput-style pads (South East West North, shoulders, triggers, Back,
// Start, sticks).
GamepadState mapGenericHid(const GenericHidReport &report);

} // namespace qtos::input
