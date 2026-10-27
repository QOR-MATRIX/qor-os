// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "playstation.h"

#include <algorithm>

namespace qor::input::playstation {

namespace {

constexpr quint16 kSony = 0x054c;

float stick(quint8 value) { return (float(value) - 127.5f) / 127.5f; }

// The control block shared by both controllers' basic layout:
// LX LY RX RY, then three button bytes. `triggersFirst` is the DualSense full
// layout, where L2 and R2 come before the buttons.
GamepadState parseControls(const quint8 *c, bool triggersFirst)
{
    GamepadState state;
    state.axis(Axis::LeftX) = stick(c[0]);
    state.axis(Axis::LeftY) = stick(c[1]);
    state.axis(Axis::RightX) = stick(c[2]);
    state.axis(Axis::RightY) = stick(c[3]);
    const quint8 *buttons = triggersFirst ? c + 7 : c + 4;
    const quint8 l2 = triggersFirst ? c[4] : c[7];
    const quint8 r2 = triggersFirst ? c[5] : c[8];

    setHat(state, buttons[0] & 0x0f);
    state.set(Button::West, buttons[0] & 0x10);  // Square
    state.set(Button::South, buttons[0] & 0x20); // Cross
    state.set(Button::East, buttons[0] & 0x40);  // Circle
    state.set(Button::North, buttons[0] & 0x80); // Triangle
    state.set(Button::LeftShoulder, buttons[1] & 0x01);
    state.set(Button::RightShoulder, buttons[1] & 0x02);
    state.set(Button::Back, buttons[1] & 0x10);  // Share / Create
    state.set(Button::Start, buttons[1] & 0x20); // Options
    state.set(Button::LeftStick, buttons[1] & 0x40);
    state.set(Button::RightStick, buttons[1] & 0x80);
    state.set(Button::Guide, buttons[2] & 0x01); // PS
    state.set(Button::Touchpad, buttons[2] & 0x02);
    if (triggersFirst)
        state.set(Button::Misc, buttons[2] & 0x04); // DualSense mute

    state.axis(Axis::LeftTrigger) = float(l2) / 255.0f;
    state.axis(Axis::RightTrigger) = float(r2) / 255.0f;
    state.deriveTriggerButtons();
    return state;
}

} // namespace

std::optional<Device> identify(quint16 vendor, quint16 product)
{
    if (vendor != kSony)
        return std::nullopt;
    switch (product) {
    case 0x05c4: return Device { Model::DualShock4, "DualShock 4" };
    case 0x09cc: return Device { Model::DualShock4, "DualShock 4" };
    case 0x0ba0: return Device { Model::DualShock4, "DualShock 4 (wireless adapter)" };
    case 0x0ce6: return Device { Model::DualSense, "DualSense" };
    case 0x0df2: return Device { Model::DualSense, "DualSense Edge" };
    default: return std::nullopt;
    }
}

std::optional<GamepadState> parseInput(Model model, const quint8 *data, int size, Battery *battery)
{
    if (size < 10)
        return std::nullopt;
    const quint8 id = data[0];

    if (model == Model::DualShock4) {
        // USB 0x01 (64 bytes) and basic Bluetooth 0x01 share a layout; the
        // full Bluetooth report 0x11 has two more header bytes.
        const int offset = id == 0x01 ? 1 : id == 0x11 ? 3 : -1;
        if (offset < 0 || size < offset + 9)
            return std::nullopt;
        GamepadState state = parseControls(data + offset, false);
        // Battery: status byte 29 of the input report (USB and full Bluetooth).
        if (battery && size >= offset + 31 && (id == 0x11 || size >= 64)) {
            const quint8 status = data[offset + 29];
            const bool cable = status & 0x10;
            const int level = status & 0x0f;
            battery->charging = cable && level < 10;
            battery->percent = level < 10 ? level * 10 + 5 : 100;
        }
        return state;
    }

    // DualSense: USB 0x01 (64 bytes) and full Bluetooth 0x31 share the full
    // layout (Bluetooth one byte later); the basic Bluetooth 0x01 report is
    // short and laid out like the DualShock 4's.
    if (id == 0x01 && size < 32)
        return parseControls(data + 1, false);
    const int offset = id == 0x01 ? 1 : id == 0x31 ? 2 : -1;
    if (offset < 0 || size < offset + 11)
        return std::nullopt;
    GamepadState state = parseControls(data + offset, true);
    if (battery && size >= offset + 54) {
        const quint8 status = data[offset + 52];
        const int level = status & 0x0f;
        const int charge = (status & 0xf0) >> 4;
        battery->percent = std::min(100, level * 10 + 5);
        battery->charging = charge == 1;
        if (charge == 2)
            battery->percent = 100;
    }
    return state;
}

FeatureRequest fullReportRequest(Model model)
{
    return model == Model::DualShock4 ? FeatureRequest { 0x02, 37 } : FeatureRequest { 0x05, 41 };
}

quint32 crc32(quint8 seed, const quint8 *data, int size)
{
    static const auto table = [] {
        std::array<quint32, 256> t {};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    quint32 crc = 0xffffffffu;
    crc = table[(crc ^ seed) & 0xff] ^ (crc >> 8);
    for (int i = 0; i < size; ++i)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return ~crc;
}

QByteArray outputReport(Model model, bool bluetooth, quint8 strongMotor, quint8 weakMotor, const QColor &light,
                        quint8 sequence)
{
    const auto r = quint8(light.red());
    const auto g = quint8(light.green());
    const auto b = quint8(light.blue());

    if (model == Model::DualShock4) {
        // Common block: valid flags, reserved, motors, light bar, flash.
        QByteArray report(bluetooth ? 78 : 32, '\0');
        auto *p = reinterpret_cast<quint8 *>(report.data());
        int common = 1;
        if (bluetooth) {
            p[0] = 0x11;
            p[1] = 0xc0; // HID report, with CRC
            p[2] = 0x00;
            common = 3;
        } else {
            p[0] = 0x05;
        }
        p[common + 0] = 0x01 | 0x02; // motors | light bar
        p[common + 3] = weakMotor;   // right: fast, light
        p[common + 4] = strongMotor; // left: slow, heavy
        p[common + 5] = r;
        p[common + 6] = g;
        p[common + 7] = b;
        if (bluetooth) {
            const quint32 crc = crc32(0xa2, p, 74);
            for (int i = 0; i < 4; ++i)
                p[74 + i] = quint8(crc >> (8 * i));
        }
        return report;
    }

    // DualSense common block (47 bytes) after the report id (USB) or after
    // id and sequence tag (Bluetooth).
    QByteArray report(bluetooth ? 78 : 63, '\0');
    auto *p = reinterpret_cast<quint8 *>(report.data());
    int common = 1;
    if (bluetooth) {
        p[0] = 0x31;
        p[1] = quint8((sequence & 0x0f) << 4) | 0x00;
        p[2] = 0x10; // output tag
        common = 3;
    } else {
        p[0] = 0x02;
    }
    p[common + 0] = 0x01 | 0x02; // compatible vibration | haptics select
    p[common + 1] = 0x04;        // light bar control enable
    p[common + 2] = weakMotor;   // right
    p[common + 3] = strongMotor; // left
    p[common + 38] = 0x02;       // valid flag 2: light bar setup control
    p[common + 41] = 0x02;       // light bar setup: light out (take control)
    p[common + 44] = r;
    p[common + 45] = g;
    p[common + 46] = b;
    if (bluetooth) {
        const quint32 crc = crc32(0xa2, p, 74);
        for (int i = 0; i < 4; ++i)
            p[74 + i] = quint8(crc >> (8 * i));
    }
    return report;
}

} // namespace qor::input::playstation
