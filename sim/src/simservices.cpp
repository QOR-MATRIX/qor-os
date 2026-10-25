// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "simservices.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QLoggingCategory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcSim, "qtos.sim")

namespace qtos::sim {

namespace {

QUrl platformDefaultUrl()
{
#if defined(Q_OS_LINUX)
    return QUrl(u"localabstract:qtos-services"_s);
#else
    return QUrl(u"local:qtos-services"_s);
#endif
}

} // namespace

// ── SimControl ───────────────────────────────────────────────────────────────

SimControlSource::SimControlSource(SimServices *services) : SimControlSimpleSource(services), m_services(services) { }

void SimControlSource::setBattery(int level, bool charging) { m_services->power()->setBattery(level, charging); }

void SimControlSource::setSignal(int bars) { m_services->network()->setSignal(bars); }

void SimControlSource::postNotification(const QString &appId, const QString &title, const QString &body)
{
    m_services->notifications()->notify(appId, title, body, {});
}

void SimControlSource::replayScenario() { m_services->replayScenario(); }

// ── SimServices ──────────────────────────────────────────────────────────────

SimServices::SimServices(QObject *parent) : QObject(parent) { }

SimServices::~SimServices() = default;

bool SimServices::loadScenario(const QString &path)
{
    const QString source = path.isEmpty() ? u":/qtos/sim/scenarios/default.json"_s : path;
    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcSim) << "Cannot open scenario" << source << file.errorString();
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(lcSim) << "Invalid scenario" << source << error.errorString();
        return false;
    }
    m_scenario = document.object();
    qCInfo(lcSim) << "Loaded scenario" << scenarioName() << "from" << source;
    return true;
}

QString SimServices::scenarioName() const { return m_scenario.value(u"name"_s).toString(u"default"_s); }

bool SimServices::start(const SimOptions &options)
{
    if (!loadScenario(options.scenarioPath) && !loadScenario({}))
        return false;

    m_engine =
        std::make_unique<audio::Engine>(options.silent ? audio::Engine::Output::Null : audio::Engine::Output::Auto);
    m_network = std::make_unique<SimNetwork>();
    m_bluetooth = std::make_unique<SimBluetooth>();
    m_power = std::make_unique<SimPower>();
    m_audio = std::make_unique<SimAudio>(m_engine.get(), options.persistent);
    m_display = std::make_unique<SimDisplay>();
    m_media = std::make_unique<SimMedia>(m_engine.get());
    m_notifications = std::make_unique<SimNotifications>();
    m_settings = std::make_unique<SimSettings>(options.persistent);
    m_control = std::make_unique<SimControlSource>(this);

    m_network->load(m_scenario.value(u"network"_s).toObject());
    m_bluetooth->load(m_scenario.value(u"bluetooth"_s).toObject());
    m_power->load(m_scenario.value(u"power"_s).toObject());
    m_audio->load(m_scenario.value(u"audio"_s).toObject());
    m_media->load(m_scenario.value(u"media"_s).toObject(),
                  options.persistent || qEnvironmentVariableIsSet("QTOS_MUSIC_DIR"));
    m_settings->load(m_scenario.value(u"user"_s).toObject());

    // The interface reacts to the frequencies chosen in Settings.
    const auto applyRange = [this] {
        m_engine->setReactiveRange(m_settings->reactiveLowHz(), m_settings->reactiveHighHz());
    };
    connect(m_settings.get(), &SettingsSimpleSource::reactiveLowHzChanged, this, applyRange);
    connect(m_settings.get(), &SettingsSimpleSource::reactiveHighHzChanged, this, applyRange);
    applyRange();
    m_control->setScenario(scenarioName());

    const QUrl requested = options.url.isEmpty() ? platformDefaultUrl() : options.url;
    m_host = std::make_unique<QRemoteObjectHost>();
    m_url = requested;
    if (!m_host->setHostUrl(m_url)) {
        // Another QTOS instance owns the default endpoint: fall back to a private one.
        m_url = QUrl(requested.scheme() + u":qtos-services-"_s + QString::number(QCoreApplication::applicationPid()));
        qCWarning(lcSim) << requested << "is in use; hosting services at" << m_url;
        if (!m_host->setHostUrl(m_url)) {
            qCCritical(lcSim) << "Cannot host services:" << m_host->lastError();
            return false;
        }
    }

    m_host->enableRemoting(m_network.get());
    m_host->enableRemoting(m_bluetooth.get());
    m_host->enableRemoting(m_power.get());
    m_host->enableRemoting(m_audio.get());
    m_host->enableRemoting(m_display.get());
    m_host->enableRemoting(m_media.get());
    m_host->enableRemoting(m_notifications.get());
    m_host->enableRemoting(m_settings.get());
    m_host->enableRemoting(m_control.get());

    m_notifications->schedule(m_scenario.value(u"notifications"_s).toArray());
    qCInfo(lcSim) << "Simulated services hosted at" << m_url;
    return true;
}

void SimServices::replayScenario() { m_notifications->schedule(m_scenario.value(u"notifications"_s).toArray()); }

} // namespace qtos::sim
