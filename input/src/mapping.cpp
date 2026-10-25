// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "mapping.h"

#include <algorithm>

namespace qtos::input {

// ── XInput ──────────────────────────────────────────────────────────────────

GamepadState mapXInput(const XInputPad &pad)
{
    GamepadState state;
    const auto has = [&pad](quint16 mask) { return (pad.buttons & mask) != 0; };
    state.set(Button::DPadUp, has(0x0001));
    state.set(Button::DPadDown, has(0x0002));
    state.set(Button::DPadLeft, has(0x0004));
    state.set(Button::DPadRight, has(0x0008));
    state.set(Button::Start, has(0x0010));
    state.set(Button::Back, has(0x0020));
    state.set(Button::LeftStick, has(0x0040));
    state.set(Button::RightStick, has(0x0080));
    state.set(Button::LeftShoulder, has(0x0100));
    state.set(Button::RightShoulder, has(0x0200));
    state.set(Button::Guide, has(0x0400));
    state.set(Button::South, has(0x1000));
    state.set(Button::East, has(0x2000));
    state.set(Button::West, has(0x4000));
    state.set(Button::North, has(0x8000));

    // XInput's Y axes point up; the QTOS layout's point down, like the screen.
    const auto thumb = [](qint16 v) { return std::max(-1.0f, float(v) / 32767.0f); };
    state.axis(Axis::LeftX) = thumb(pad.thumbLX);
    state.axis(Axis::LeftY) = -thumb(pad.thumbLY);
    state.axis(Axis::RightX) = thumb(pad.thumbRX);
    state.axis(Axis::RightY) = -thumb(pad.thumbRY);
    state.axis(Axis::LeftTrigger) = float(pad.leftTrigger) / 255.0f;
    state.axis(Axis::RightTrigger) = float(pad.rightTrigger) / 255.0f;
    state.deriveTriggerButtons();
    return state;
}

// ── evdev ───────────────────────────────────────────────────────────────────

EvdevMapper::EvdevMapper(const std::array<AbsRange, evdev::AbsCount> &ranges, bool swapNorthWest)
    : m_ranges(ranges), m_swapNorthWest(swapNorthWest)
      // Gamepads with RX/RY (xpad, hid-playstation) use them for the right
      // stick and Z/RZ for the triggers; others put the right stick on Z/RZ.
      ,
      m_rightStickIsRxRy(ranges[evdev::AbsRx].valid() && ranges[evdev::AbsRy].valid())
{ }

float EvdevMapper::normalised(int code, int value, bool centred) const
{
    const AbsRange &range = m_ranges[std::size_t(code)];
    if (!range.valid())
        return 0.0f;
    const float t = float(value - range.minimum) / float(range.maximum - range.minimum);
    return std::clamp(centred ? t * 2.0f - 1.0f : t, centred ? -1.0f : 0.0f, 1.0f);
}

bool EvdevMapper::handle(int type, int code, int value)
{
    using namespace evdev;
    if (type == EvSyn) {
        if (code == SynDropped) {
            // The kernel's buffer overran: ignore until the next report.
            m_dropping = true;
            return false;
        }
        if (code != SynReport)
            return false;
        if (m_dropping) {
            m_dropping = false;
            return false;
        }
        m_pending.deriveTriggerButtons();
        if (m_pending == m_state)
            return false;
        m_state = m_pending;
        return true;
    }
    if (type == EvKey) {
        const bool down = value != 0;
        switch (code) {
        case BtnSouth: m_pending.set(Button::South, down); break;
        case BtnEast: m_pending.set(Button::East, down); break;
        case BtnNorth: m_pending.set(m_swapNorthWest ? Button::West : Button::North, down); break;
        case BtnWest: m_pending.set(m_swapNorthWest ? Button::North : Button::West, down); break;
        case BtnTL: m_pending.set(Button::LeftShoulder, down); break;
        case BtnTR: m_pending.set(Button::RightShoulder, down); break;
        case BtnTL2:
            if (!m_ranges[std::size_t(m_rightStickIsRxRy ? AbsZ : AbsBrake)].valid())
                m_pending.axis(Axis::LeftTrigger) = down ? 1.0f : 0.0f;
            break;
        case BtnTR2:
            if (!m_ranges[std::size_t(m_rightStickIsRxRy ? AbsRz : AbsGas)].valid())
                m_pending.axis(Axis::RightTrigger) = down ? 1.0f : 0.0f;
            break;
        case BtnSelect: m_pending.set(Button::Back, down); break;
        case BtnStart: m_pending.set(Button::Start, down); break;
        case BtnMode: m_pending.set(Button::Guide, down); break;
        case BtnThumbL: m_pending.set(Button::LeftStick, down); break;
        case BtnThumbR: m_pending.set(Button::RightStick, down); break;
        case BtnDpadUp: m_pending.set(Button::DPadUp, down); break;
        case BtnDpadDown: m_pending.set(Button::DPadDown, down); break;
        case BtnDpadLeft: m_pending.set(Button::DPadLeft, down); break;
        case BtnDpadRight: m_pending.set(Button::DPadRight, down); break;
        default: break;
        }
        return false;
    }
    if (type == EvAbs && code >= 0 && code < AbsCount) {
        switch (code) {
        case AbsX: m_pending.axis(Axis::LeftX) = normalised(code, value, true); break;
        case AbsY: m_pending.axis(Axis::LeftY) = normalised(code, value, true); break;
        case AbsRx:
            if (m_rightStickIsRxRy)
                m_pending.axis(Axis::RightX) = normalised(code, value, true);
            break;
        case AbsRy:
            if (m_rightStickIsRxRy)
                m_pending.axis(Axis::RightY) = normalised(code, value, true);
            break;
        case AbsZ:
            if (m_rightStickIsRxRy)
                m_pending.axis(Axis::LeftTrigger) = normalised(code, value, false);
            else
                m_pending.axis(Axis::RightX) = normalised(code, value, true);
            break;
        case AbsRz:
            if (m_rightStickIsRxRy)
                m_pending.axis(Axis::RightTrigger) = normalised(code, value, false);
            else
                m_pending.axis(Axis::RightY) = normalised(code, value, true);
            break;
        case AbsBrake: m_pending.axis(Axis::LeftTrigger) = normalised(code, value, false); break;
        case AbsGas: m_pending.axis(Axis::RightTrigger) = normalised(code, value, false); break;
        case AbsHat0X:
            m_pending.set(Button::DPadLeft, value < 0);
            m_pending.set(Button::DPadRight, value > 0);
            break;
        case AbsHat0Y:
            m_pending.set(Button::DPadUp, value < 0);
            m_pending.set(Button::DPadDown, value > 0);
            break;
        default: break;
        }
    }
    return false;
}

// ── Generic HID ─────────────────────────────────────────────────────────────

GamepadState mapGenericHid(const GenericHidReport &report)
{
    GamepadState state;
    const bool android = report.buttonCount >= 15;
    for (int usage : report.pressed) {
        std::optional<Button> button;
        if (android) {
            switch (usage) {
            case 1: button = Button::South; break;
            case 2: button = Button::East; break;
            case 4: button = Button::West; break;
            case 5: button = Button::North; break;
            case 7: button = Button::LeftShoulder; break;
            case 8: button = Button::RightShoulder; break;
            case 9: state.axis(Axis::LeftTrigger) = 1.0f; break;
            case 10: state.axis(Axis::RightTrigger) = 1.0f; break;
            case 11: button = Button::Back; break;
            case 12: button = Button::Start; break;
            case 13: button = Button::Guide; break;
            case 14: button = Button::LeftStick; break;
            case 15: button = Button::RightStick; break;
            default: break;
            }
        } else {
            switch (usage) {
            case 1: button = Button::South; break;
            case 2: button = Button::East; break;
            case 3: button = Button::West; break;
            case 4: button = Button::North; break;
            case 5: button = Button::LeftShoulder; break;
            case 6: button = Button::RightShoulder; break;
            case 7: state.axis(Axis::LeftTrigger) = 1.0f; break;
            case 8: state.axis(Axis::RightTrigger) = 1.0f; break;
            case 9: button = Button::Back; break;
            case 10: button = Button::Start; break;
            case 11: button = Button::LeftStick; break;
            case 12: button = Button::RightStick; break;
            case 13: button = Button::Guide; break;
            default: break;
            }
        }
        if (button)
            state.set(*button, true);
    }

    state.axis(Axis::LeftX) = report.x.value_or(0.0f);
    state.axis(Axis::LeftY) = report.y.value_or(0.0f);
    if (report.z && report.rz) {
        state.axis(Axis::RightX) = *report.z;
        state.axis(Axis::RightY) = *report.rz;
        if (report.rx && report.ry && !report.brake) {
            state.axis(Axis::LeftTrigger) = std::max(state.axis(Axis::LeftTrigger), (*report.rx + 1.0f) / 2.0f);
            state.axis(Axis::RightTrigger) = std::max(state.axis(Axis::RightTrigger), (*report.ry + 1.0f) / 2.0f);
        }
    } else if (report.rx && report.ry) {
        state.axis(Axis::RightX) = *report.rx;
        state.axis(Axis::RightY) = *report.ry;
    }
    if (report.brake)
        state.axis(Axis::LeftTrigger) = std::max(state.axis(Axis::LeftTrigger), *report.brake);
    if (report.accelerator)
        state.axis(Axis::RightTrigger) = std::max(state.axis(Axis::RightTrigger), *report.accelerator);
    if (report.hat >= 0)
        setHat(state, report.hat);
    state.deriveTriggerButtons();
    return state;
}

} // namespace qtos::input
