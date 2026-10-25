// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "gamepad.h"

#include <QtCore/QByteArray>
#include <QtGui/QColor>

#include <optional>

// Sony DualShock 4 and DualSense, spoken to directly over HID (USB and
// Bluetooth): input reports, rumble and the light bar. Layouts follow the
// Linux kernel's hid-playstation driver. Platform independent, so the parsing
// is unit tested without a controller.
namespace qtos::input::playstation {

enum class Model { DualShock4, DualSense };

struct Device
{
    Model model;
    const char *name;
};

// Known Sony controllers by USB vendor/product id.
std::optional<Device> identify(quint16 vendor, quint16 product);

struct Battery
{
    int percent = -1;
    bool charging = false;
};

// Parses one input report (report id first). Handles USB reports, the basic
// Bluetooth report (0x01) and the full Bluetooth reports (DualShock 4 0x11,
// DualSense 0x31). Returns nothing for reports that carry no controls.
std::optional<GamepadState> parseInput(Model model, const quint8 *data, int size, Battery *battery = nullptr);

// Builds an output report that sets rumble (0..255 per motor) and the light
// bar. Bluetooth reports carry a CRC-32. `sequence` (0..15) is the DualSense
// Bluetooth sequence number.
QByteArray outputReport(Model model, bool bluetooth, quint8 strongMotor, quint8 weakMotor, const QColor &light,
                        quint8 sequence = 0);

// The feature report to read once over Bluetooth to switch the controller
// from its basic report to full reports (id and length).
struct FeatureRequest
{
    quint8 id;
    int length;
};
FeatureRequest fullReportRequest(Model model);

// CRC-32 (IEEE, as zlib) over `seed` followed by `data`, as the controllers
// expect on Bluetooth (seed 0xA2 for output reports).
quint32 crc32(quint8 seed, const quint8 *data, int size);

} // namespace qtos::input::playstation
