// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "simdevices.h"

#include <QtCore/QJsonArray>
#include <QtCore/QLoggingCategory>

#include <algorithm>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcSimDevices, "qtos.sim.devices")

namespace qtos::sim {

// ── Network ──────────────────────────────────────────────────────────────────

SimNetwork::SimNetwork(QObject *parent) : NetworkSimpleSource(parent)
{
    m_connectTimer.setSingleShot(true);
    m_connectTimer.setInterval(1100);
    connect(&m_connectTimer, &QTimer::timeout, this, &SimNetwork::finishConnect);
    connect(this, &NetworkSimpleSource::wifiEnabledChanged, this, &SimNetwork::onWifiEnabledChanged);
}

void SimNetwork::load(const QJsonObject &config)
{
    setAccessPoints(config.value(u"accessPoints"_s).toArray().toVariantList());
    m_preferredSsid = config.value(u"ssid"_s).toString();
    if (wifiEnabled() && !m_preferredSsid.isEmpty()) {
        setActiveSsid(m_preferredSsid);
        setSignalBars(barsFor(m_preferredSsid));
        setOnline(true);
    }
}

int SimNetwork::barsFor(const QString &ssid) const
{
    const QVariantList points = accessPoints();
    for (const QVariant &point : points) {
        const QVariantMap map = point.toMap();
        if (map.value(u"ssid"_s).toString() == ssid)
            return map.value(u"bars"_s).toInt();
    }
    return -1;
}

void SimNetwork::setSignal(int bars) { setSignalBars(std::clamp(bars, 0, 3)); }

void SimNetwork::connectTo(const QString &ssid)
{
    if (!wifiEnabled()) {
        Q_EMIT connectionFailed(ssid, u"Wi-Fi is turned off"_s);
        return;
    }
    if (barsFor(ssid) < 0) {
        Q_EMIT connectionFailed(ssid, u"Network not in range"_s);
        return;
    }
    m_pendingSsid = ssid;
    setOnline(false);
    setConnecting(true);
    m_connectTimer.start();
}

void SimNetwork::disconnectNetwork()
{
    m_connectTimer.stop();
    setConnecting(false);
    setOnline(false);
    setActiveSsid({});
    setSignalBars(0);
}

void SimNetwork::finishConnect()
{
    setConnecting(false);
    if (!wifiEnabled())
        return;
    m_preferredSsid = m_pendingSsid;
    setActiveSsid(m_pendingSsid);
    setSignalBars(barsFor(m_pendingSsid));
    setOnline(true);
    qCDebug(lcSimDevices) << "Connected to" << m_pendingSsid;
}

void SimNetwork::onWifiEnabledChanged()
{
    if (wifiEnabled()) {
        if (!m_preferredSsid.isEmpty())
            connectTo(m_preferredSsid);
    } else {
        disconnectNetwork();
    }
}

// ── Bluetooth ────────────────────────────────────────────────────────────────

SimBluetooth::SimBluetooth(QObject *parent) : BluetoothSimpleSource(parent)
{
    connect(this, &BluetoothSimpleSource::enabledChanged, this, [this] {
        if (enabled()) {
            setDevices(m_knownDevices);
            return;
        }
        QVariantList off;
        for (const QVariant &device : std::as_const(m_knownDevices)) {
            QVariantMap map = device.toMap();
            map.insert(u"connected"_s, false);
            off.append(map);
        }
        setDevices(off);
    });
}

void SimBluetooth::load(const QJsonObject &config)
{
    m_knownDevices = config.value(u"devices"_s).toArray().toVariantList();
    setDevices(m_knownDevices);
}

void SimBluetooth::toggleDevice(const QString &name)
{
    if (!enabled())
        return;
    for (QVariant &device : m_knownDevices) {
        QVariantMap map = device.toMap();
        if (map.value(u"name"_s).toString() == name) {
            map.insert(u"connected"_s, !map.value(u"connected"_s).toBool());
            device = map;
        }
    }
    setDevices(m_knownDevices);
}

// ── Power ────────────────────────────────────────────────────────────────────

SimPower::SimPower(QObject *parent) : PowerSimpleSource(parent)
{
    setProfile(u"balanced"_s);
    connect(&m_timer, &QTimer::timeout, this, &SimPower::tick);
}

void SimPower::load(const QJsonObject &config)
{
    m_secondsPerPercent = std::max(1, config.value(u"secondsPerPercent"_s).toInt(90));
    setBattery(config.value(u"batteryLevel"_s).toInt(82), config.value(u"charging"_s).toBool(false));
    m_timer.start(m_secondsPerPercent * 1000);
}

void SimPower::setBattery(int level, bool charging)
{
    setBatteryLevel(std::clamp(level, 0, 100));
    setCharging(charging);
    setOnBattery(!charging);
    if (level > 20)
        m_warned20 = false;
    if (level > 10)
        m_warned10 = false;
    updateEstimate();
}

void SimPower::tick()
{
    int level = batteryLevel();
    if (charging())
        level = std::min(100, level + 2);
    else
        level = std::max(0, level - 1);
    setBatteryLevel(level);
    updateEstimate();

    if (!charging() && level <= 20 && !m_warned20) {
        m_warned20 = true;
        Q_EMIT batteryLow(level);
    }
    if (!charging() && level <= 10 && !m_warned10) {
        m_warned10 = true;
        Q_EMIT batteryLow(level);
    }
}

void SimPower::updateEstimate()
{
    const int level = batteryLevel();
    const int minutes =
        charging() ? ((100 - level) * m_secondsPerPercent / 2) / 60 : (level * m_secondsPerPercent) / 60;
    setMinutesRemaining(minutes);
}

void SimPower::suspend() { qCInfo(lcSimDevices) << "Suspend requested (simulated)"; }

// ── Audio ────────────────────────────────────────────────────────────────────

SimAudio::SimAudio(QObject *parent) : AudioSimpleSource(parent) { }

void SimAudio::load(const QJsonObject &config)
{
    QStringList devices;
    const QJsonArray array = config.value(u"devices"_s).toArray();
    for (const QJsonValue &value : array)
        devices.append(value.toString());
    setOutputDevices(devices);
    setOutputDevice(config.value(u"output"_s).toString(devices.value(0)));
    setVolume(config.value(u"volume"_s).toDouble(0.6));
}

void SimAudio::selectOutput(const QString &device)
{
    if (outputDevices().contains(device))
        setOutputDevice(device);
}

} // namespace qtos::sim
