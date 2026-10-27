// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "gamepad.h"
#include "gamepadbackend.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <memory>
#include <optional>
#include <vector>

namespace qor::input {

class VirtualBackend;

// Every game controller connected to the machine, from every backend, in the
// standard QOR OS layout: dead zones applied, triggers as buttons, and button
// edges reported. `merged` combines all controllers, so the same controller
// seen twice (for example a DualShock 4 that Steam Input also presents as an
// Xbox controller) never acts twice.
//
// Backends: XInput and raw HID (PlayStation, generic HID gamepads) on
// Windows; evdev on Linux; and virtual controllers for tests and scripts.
// QOR_GAMEPADS=virtual keeps only the virtual one (tests), =off none.
class GamepadManager : public QObject
{
    Q_OBJECT
public:
    static GamepadManager *instance();
    ~GamepadManager() override;

    QList<GamepadInfo> gamepads() const;
    std::optional<GamepadInfo> gamepad(const QString &id) const;
    GamepadState state(const QString &id) const;
    GamepadState merged() const { return m_merged; }
    // The controller used most recently.
    QString activeId() const { return m_active; }
    QStringList backendNames() const;

    // A short vibration on every controller that can (`strong` and `weak`
    // 0..1), or on one.
    void rumble(float strong, float weak, int milliseconds, const QString &id = {});
    // The light bar colour of every controller that has one.
    void setLightColor(const QColor &color);
    QColor lightColor() const { return m_light; }

    // Virtual controllers, for tests and the shell's script runner.
    QString addVirtual(Family family, const QString &name);
    void setVirtualState(const QString &id, const GamepadState &state);
    void removeVirtual(const QString &id);

Q_SIGNALS:
    void connected(const qor::input::GamepadInfo &info);
    void infoChanged(const qor::input::GamepadInfo &info);
    void disconnected(const qor::input::GamepadInfo &info);
    void stateChanged(const QString &id, const qor::input::GamepadState &state);
    void buttonPressed(const QString &id, qor::input::Button button);
    void buttonReleased(const QString &id, qor::input::Button button);
    // The combined state of every controller changed.
    void mergedChanged(const qor::input::GamepadState &now, const qor::input::GamepadState &before);
    void activeChanged();

private:
    explicit GamepadManager(QObject *parent = nullptr);
    void addBackend(std::unique_ptr<Backend> backend);
    void onAdded(Backend *backend, const GamepadInfo &info);
    void onRemoved(const QString &id);
    void onChanged(const QString &id, const GamepadState &raw);
    void updateMerged();

    struct Pad
    {
        GamepadInfo info;
        GamepadState state;
        Backend *backend = nullptr;
    };

    std::vector<std::unique_ptr<Backend>> m_backends;
    VirtualBackend *m_virtual = nullptr;
    QHash<QString, Pad> m_pads;
    QStringList m_order;
    GamepadState m_merged;
    QString m_active;
    QColor m_light;
    QTimer m_poll;
    QHash<QString, QTimer *> m_rumbleStops;
};

} // namespace qor::input
