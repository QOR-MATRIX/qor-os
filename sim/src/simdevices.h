// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "engine.h"
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

// Audio backed by the QTOS audio engine: the machine's real outputs (or the
// scenario's, when the engine renders silently), perceptual volume, the
// equaliser, interface-sound preferences and live analysis for the
// audio-reactive interface.
class SimAudio : public AudioSimpleSource
{
    Q_OBJECT
public:
    SimAudio(audio::Engine *engine, bool persistent, QObject *parent = nullptr);
    void load(const QJsonObject &config);

    void selectOutput(const QString &device) override;
    void setEqualizerBand(int band, double gainDb) override;
    void applyEqualizerPreset(const QString &name) override;

private:
    void applyOutputs(const QList<audio::OutputDevice> &outputs, const QString &currentId);
    void applyEqualizer(const QVariantList &gains, const QString &preset);
    void forward(const audio::AnalysisFrame &frame);
    void save();

    audio::Engine *m_engine;
    bool m_persistent;
    QList<audio::OutputDevice> m_outputs;
    QString m_preferredOutput; // chosen by name; empty: the system default
    bool m_restoredOutput = false;
};

class SimDisplay : public DisplaySimpleSource
{
    Q_OBJECT
public:
    using DisplaySimpleSource::DisplaySimpleSource;
};

} // namespace qtos::sim
