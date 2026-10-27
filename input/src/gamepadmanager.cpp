// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "gamepadmanager.h"

#include "backends.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>

#include <algorithm>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcGamepads, "qor.input.gamepads")

namespace qor::input {

namespace {
constexpr int kPollMs = 8;
}

// ── Virtual backend ─────────────────────────────────────────────────────────

QString VirtualBackend::add(Family family, const QString &name)
{
    GamepadInfo info;
    info.id = u"virtual:"_s + QString::number(m_next++);
    info.name = name;
    info.family = family;
    info.connection = u"Virtual"_s;
    info.rumble = true;
    info.lightbar = family == Family::PlayStation;
    m_pads.insert(info.id, info);
    Q_EMIT added(info);
    return info.id;
}

void VirtualBackend::set(const QString &id, const GamepadState &state)
{
    if (m_pads.contains(id))
        Q_EMIT changed(id, state);
}

void VirtualBackend::remove(const QString &id)
{
    if (m_pads.remove(id))
        Q_EMIT removed(id);
}

// ── Manager ─────────────────────────────────────────────────────────────────

GamepadManager *GamepadManager::instance()
{
    static GamepadManager *manager = new GamepadManager(qApp);
    return manager;
}

GamepadManager::GamepadManager(QObject *parent) : QObject(parent), m_light(QColor::fromRgb(0x6b, 0x5b, 0xff))
{
    qRegisterMetaType<qor::input::GamepadInfo>();
    qRegisterMetaType<qor::input::GamepadState>();

    auto virtualBackend = std::make_unique<VirtualBackend>();
    m_virtual = virtualBackend.get();
    addBackend(std::move(virtualBackend));

    const QString mode = qEnvironmentVariable("QOR_GAMEPADS");
    if (mode != u"virtual"_s && mode != u"off"_s) {
        for (auto &backend : createHardwareBackends())
            addBackend(std::move(backend));
    }
    qCInfo(lcGamepads) << "Controller backends:" << backendNames();

    m_poll.setTimerType(Qt::PreciseTimer);
    m_poll.setInterval(kPollMs);
    connect(&m_poll, &QTimer::timeout, this, [this] {
        for (auto &backend : m_backends)
            backend->poll();
    });
    for (auto &backend : m_backends)
        backend->start();
    m_poll.start();
}

GamepadManager::~GamepadManager() = default;

void GamepadManager::addBackend(std::unique_ptr<Backend> backend)
{
    Backend *raw = backend.get();
    connect(raw, &Backend::added, this, [this, raw](const GamepadInfo &info) { onAdded(raw, info); });
    connect(raw, &Backend::infoChanged, this, [this](const GamepadInfo &info) {
        auto it = m_pads.find(info.id);
        if (it == m_pads.end() || it->info == info)
            return;
        it->info = info;
        Q_EMIT infoChanged(info);
    });
    connect(raw, &Backend::removed, this, &GamepadManager::onRemoved);
    connect(raw, &Backend::changed, this, &GamepadManager::onChanged);
    m_backends.push_back(std::move(backend));
}

QStringList GamepadManager::backendNames() const
{
    QStringList names;
    for (const auto &backend : m_backends)
        names.append(backend->name());
    return names;
}

QList<GamepadInfo> GamepadManager::gamepads() const
{
    QList<GamepadInfo> list;
    for (const QString &id : m_order)
        list.append(m_pads.value(id).info);
    return list;
}

std::optional<GamepadInfo> GamepadManager::gamepad(const QString &id) const
{
    const auto it = m_pads.constFind(id);
    if (it == m_pads.constEnd())
        return std::nullopt;
    return it->info;
}

GamepadState GamepadManager::state(const QString &id) const { return m_pads.value(id).state; }

void GamepadManager::onAdded(Backend *backend, const GamepadInfo &info)
{
    if (m_pads.contains(info.id))
        return;
    m_pads.insert(info.id, Pad { info, {}, backend });
    m_order.append(info.id);
    qCInfo(lcGamepads).nospace() << "Connected: " << info.name << " (" << info.connection << ", " << backend->name()
                                 << ")";
    if (info.lightbar)
        backend->setLight(info.id, m_light);
    if (m_active.isEmpty()) {
        m_active = info.id;
        Q_EMIT activeChanged();
    }
    Q_EMIT connected(info);
}

void GamepadManager::onRemoved(const QString &id)
{
    const auto it = m_pads.find(id);
    if (it == m_pads.end())
        return;
    const GamepadInfo info = it->info;
    // Release whatever it was holding.
    const GamepadState held = it->state;
    for (int b = 0; b < int(Button::Count); ++b)
        if (held.pressed(Button(b)))
            Q_EMIT buttonReleased(id, Button(b));
    m_pads.erase(it);
    m_order.removeAll(id);
    if (QTimer *timer = m_rumbleStops.take(id))
        timer->deleteLater();
    qCInfo(lcGamepads) << "Disconnected:" << info.name;
    if (m_active == id) {
        m_active = m_order.isEmpty() ? QString() : m_order.constLast();
        Q_EMIT activeChanged();
    }
    updateMerged();
    Q_EMIT disconnected(info);
}

void GamepadManager::onChanged(const QString &id, const GamepadState &raw)
{
    auto it = m_pads.find(id);
    if (it == m_pads.end())
        return;
    GamepadState state = raw;
    applyStickDeadZone(state.axis(Axis::LeftX), state.axis(Axis::LeftY));
    applyStickDeadZone(state.axis(Axis::RightX), state.axis(Axis::RightY));
    state.axis(Axis::LeftTrigger) = applyTriggerDeadZone(state.axis(Axis::LeftTrigger));
    state.axis(Axis::RightTrigger) = applyTriggerDeadZone(state.axis(Axis::RightTrigger));
    // Keep the hysteresis of the previous state for the trigger buttons.
    state.set(Button::LeftTrigger, it->state.pressed(Button::LeftTrigger));
    state.set(Button::RightTrigger, it->state.pressed(Button::RightTrigger));
    state.deriveTriggerButtons();
    if (state == it->state)
        return;

    const GamepadState before = it->state;
    it->state = state;
    if (state.buttons != before.buttons
        || std::abs(state.axis(Axis::LeftX)) + std::abs(state.axis(Axis::LeftY)) > 0.3f) {
        if (m_active != id) {
            m_active = id;
            Q_EMIT activeChanged();
        }
    }
    const quint32 changedButtons = state.buttons ^ before.buttons;
    for (int b = 0; b < int(Button::Count); ++b) {
        if (!(changedButtons & bit(Button(b))))
            continue;
        if (state.pressed(Button(b)))
            Q_EMIT buttonPressed(id, Button(b));
        else
            Q_EMIT buttonReleased(id, Button(b));
    }
    Q_EMIT stateChanged(id, state);
    updateMerged();
}

void GamepadManager::updateMerged()
{
    GamepadState merged;
    for (const Pad &pad : std::as_const(m_pads)) {
        merged.buttons |= pad.state.buttons;
        for (int a = 0; a < int(Axis::Count); ++a) {
            const float value = pad.state.axes[std::size_t(a)];
            if (std::abs(value) > std::abs(merged.axes[std::size_t(a)]))
                merged.axes[std::size_t(a)] = value;
        }
    }
    if (merged == m_merged)
        return;
    const GamepadState before = m_merged;
    m_merged = merged;
    Q_EMIT mergedChanged(merged, before);
}

void GamepadManager::rumble(float strong, float weak, int milliseconds, const QString &id)
{
    for (auto it = m_pads.cbegin(); it != m_pads.cend(); ++it) {
        if (!it->info.rumble || (!id.isEmpty() && it.key() != id))
            continue;
        const QString padId = it.key();
        Backend *backend = it->backend;
        backend->setRumble(padId, std::clamp(strong, 0.0f, 1.0f), std::clamp(weak, 0.0f, 1.0f));
        QTimer *&stop = m_rumbleStops[padId];
        if (!stop) {
            stop = new QTimer(this);
            stop->setSingleShot(true);
            connect(stop, &QTimer::timeout, this, [this, padId] {
                const auto pad = m_pads.constFind(padId);
                if (pad != m_pads.constEnd())
                    pad->backend->setRumble(padId, 0, 0);
            });
        }
        stop->start(milliseconds);
    }
}

void GamepadManager::setLightColor(const QColor &color)
{
    if (color == m_light)
        return;
    m_light = color;
    for (auto it = m_pads.cbegin(); it != m_pads.cend(); ++it)
        if (it->info.lightbar)
            it->backend->setLight(it.key(), color);
}

QString GamepadManager::addVirtual(Family family, const QString &name) { return m_virtual->add(family, name); }

void GamepadManager::setVirtualState(const QString &id, const GamepadState &state) { m_virtual->set(id, state); }

void GamepadManager::removeVirtual(const QString &id) { m_virtual->remove(id); }

} // namespace qor::input
