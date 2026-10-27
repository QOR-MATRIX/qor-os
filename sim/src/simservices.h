// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "rep_simcontrol_source.h"
#include "simdevices.h"
#include "simsession.h"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtRemoteObjects/QRemoteObjectHost>

#include <memory>

namespace qor::sim {

class SimServices;

// Simulation-only control surface (qorctl, tests).
class SimControlSource : public SimControlSimpleSource
{
    Q_OBJECT
public:
    explicit SimControlSource(SimServices *services);
    void setBattery(int level, bool charging) override;
    void setSignal(int bars) override;
    void postNotification(const QString &appId, const QString &title, const QString &body) override;
    void replayScenario() override;

private:
    SimServices *m_services;
};

struct SimOptions
{
    QUrl url;               // empty: platform default
    QString scenarioPath;   // empty: built-in default scenario
    bool persistent = true; // persist user preferences, read the user's music
    bool silent = false;    // render audio to the null output (automation)
};

// Hosts every simulated QOR OS service on a Qt Remote Objects node, so the
// shell, qorctl and tests talk to them exactly as they would to the real
// qor-*d daemons on a device.
class SimServices : public QObject
{
    Q_OBJECT
public:
    explicit SimServices(QObject *parent = nullptr);
    ~SimServices() override;

    // Loads the scenario and starts remoting. Returns false if no endpoint
    // could be opened.
    bool start(const SimOptions &options);
    QUrl url() const { return m_url; }
    QString scenarioName() const;

    SimNetwork *network() const { return m_network.get(); }
    SimBluetooth *bluetooth() const { return m_bluetooth.get(); }
    SimPower *power() const { return m_power.get(); }
    SimAudio *audio() const { return m_audio.get(); }
    SimDisplay *display() const { return m_display.get(); }
    SimMedia *media() const { return m_media.get(); }
    SimNotifications *notifications() const { return m_notifications.get(); }
    SimSettings *settings() const { return m_settings.get(); }
    audio::Engine *audioEngine() const { return m_engine.get(); }

    void replayScenario();

private:
    bool loadScenario(const QString &path);

    QJsonObject m_scenario;
    QUrl m_url;
    std::unique_ptr<QRemoteObjectHost> m_host;
    std::unique_ptr<audio::Engine> m_engine; // outlives the services that use it
    std::unique_ptr<SimNetwork> m_network;
    std::unique_ptr<SimBluetooth> m_bluetooth;
    std::unique_ptr<SimPower> m_power;
    std::unique_ptr<SimAudio> m_audio;
    std::unique_ptr<SimDisplay> m_display;
    std::unique_ptr<SimMedia> m_media;
    std::unique_ptr<SimNotifications> m_notifications;
    std::unique_ptr<SimSettings> m_settings;
    std::unique_ptr<SimControlSource> m_control;
};

} // namespace qor::sim
