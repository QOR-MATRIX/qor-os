// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "rep_audio_source.h"
#include "rep_bluetooth_source.h"
#include "rep_display_source.h"
#include "rep_network_source.h"
#include "rep_power_source.h"

#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

namespace qtos::sim {

// Simulated Wi-Fi: access points come from the scenario; connecting takes a
// realistic moment and toggling the radio drops and restores the link.
class SimNetwork : public NetworkSimpleSource
{
    Q_OBJECT
public:
    explicit SimNetwork(QObject *parent = nullptr);
    void load(const QJsonObject &config);
    void setSignal(int bars);

    void connectTo(const QString &ssid) override;
    void disconnectNetwork() override;

private:
    void onWifiEnabledChanged();
    void finishConnect();
    int barsFor(const QString &ssid) const;

    QString m_preferredSsid;
    QString m_pendingSsid;
    QTimer m_connectTimer;
};

// Simulated Bluetooth adapter with paired devices.
class SimBluetooth : public BluetoothSimpleSource
{
    Q_OBJECT
public:
    explicit SimBluetooth(QObject *parent = nullptr);
    void load(const QJsonObject &config);
    void toggleDevice(const QString &name) override;

private:
    QVariantList m_knownDevices;
};

// Simulated battery that drains (or charges) in real time.
class SimPower : public PowerSimpleSource
{
    Q_OBJECT
public:
    explicit SimPower(QObject *parent = nullptr);
    void load(const QJsonObject &config);
    void setBattery(int level, bool charging);
    void suspend() override;

private:
    void tick();
    void updateEstimate();

    QTimer m_timer;
    int m_secondsPerPercent = 90;
    bool m_warned20 = false;
    bool m_warned10 = false;
};

class SimAudio : public AudioSimpleSource
{
    Q_OBJECT
public:
    explicit SimAudio(QObject *parent = nullptr);
    void load(const QJsonObject &config);
    void selectOutput(const QString &device) override;
};

class SimDisplay : public DisplaySimpleSource
{
    Q_OBJECT
public:
    using DisplaySimpleSource::DisplaySimpleSource;
};

} // namespace qtos::sim
