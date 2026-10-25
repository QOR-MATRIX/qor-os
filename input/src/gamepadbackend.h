// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "gamepad.h"

#include <QtCore/QObject>
#include <QtGui/QColor>

namespace qtos::input {

// One source of controllers: XInput, raw HID, Linux evdev, or virtual
// (tests). Lives on the manager's thread; poll() is called about 125 times
// a second and reports through the signals, raw (no dead zones).
class Backend : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    virtual QString name() const = 0;
    virtual void start() = 0;
    virtual void poll() { }

    // Motor levels 0..1 until changed (the manager times pulses).
    virtual void setRumble(const QString &id, float strong, float weak)
    {
        Q_UNUSED(id);
        Q_UNUSED(strong);
        Q_UNUSED(weak);
    }
    virtual void setLight(const QString &id, const QColor &color)
    {
        Q_UNUSED(id);
        Q_UNUSED(color);
    }

Q_SIGNALS:
    void added(const qtos::input::GamepadInfo &info);
    void infoChanged(const qtos::input::GamepadInfo &info);
    void removed(const QString &id);
    void changed(const QString &id, const qtos::input::GamepadState &state);
};

} // namespace qtos::input
