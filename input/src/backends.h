// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "gamepadbackend.h"

#include <QtCore/QHash>

#include <memory>
#include <vector>

namespace qtos::input {

// Controllers driven by code: tests, the shell's script runner, and demos.
class VirtualBackend : public Backend
{
    Q_OBJECT
public:
    using Backend::Backend;
    QString name() const override { return QStringLiteral("virtual"); }
    void start() override { }

    QString add(Family family, const QString &name);
    void set(const QString &id, const GamepadState &state);
    void remove(const QString &id);

private:
    int m_next = 1;
    QHash<QString, GamepadInfo> m_pads;
};

// The platform's hardware backends (XInput and HID on Windows, evdev on
// Linux).
std::vector<std::unique_ptr<Backend>> createHardwareBackends();

} // namespace qtos::input
