// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "simdevices.h"

#include "dsp.h"

#include <QtCore/QJsonArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>

#include <algorithm>
#include <array>
#include <utility>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcSimDevices, "qor.sim.devices")

namespace qor::sim {

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

namespace {

struct EqualizerPreset
{
    const char16_t *name;
    std::array<double, 5> gains; // 60 Hz, 250 Hz, 1 kHz, 4 kHz, 12 kHz
};

constexpr EqualizerPreset kEqualizerPresets[] {
    { u"Flat", { 0, 0, 0, 0, 0 } },
    { u"Bass Boost", { 6, 3.5, 0, 0, 0 } },
    { u"Warm", { 3, 1.5, 0, -1.5, -3 } },
    { u"Vocal", { -2, -1, 3, 2.5, 0 } },
    { u"Bright", { -1, 0, 0, 2.5, 5 } },
    { u"Loudness", { 5, 1.5, -1, 1.5, 4 } },
    { u"Late Night", { -5, -2, 1, 1.5, -2 } },
};

QVariantList presetGains(const EqualizerPreset &preset)
{
    QVariantList gains;
    for (double gain : preset.gains)
        gains.append(gain);
    return gains;
}

} // namespace

SimAudio::SimAudio(audio::Engine *engine, bool persistent, QObject *parent)
    : AudioSimpleSource(parent), m_engine(engine), m_persistent(persistent)
{
    QStringList presets;
    for (const EqualizerPreset &preset : kEqualizerPresets)
        presets.append(QString::fromUtf16(preset.name));
    setEqualizerPresets(presets);
    applyEqualizer(presetGains(kEqualizerPresets[0]), presets.first());

    connect(this, &AudioSimpleSource::volumeChanged, this, [this] {
        m_engine->setVolume(volume());
        save();
    });
    connect(this, &AudioSimpleSource::mutedChanged, this, [this] {
        m_engine->setMuted(muted());
        save();
    });
    connect(this, &AudioSimpleSource::systemSoundsChanged, this, &SimAudio::save);
    connect(this, &AudioSimpleSource::systemVolumeChanged, this, &SimAudio::save);
    connect(m_engine, &audio::Engine::outputsChanged, this, &SimAudio::applyOutputs);
    connect(m_engine, &audio::Engine::analysisReady, this, &SimAudio::forward);
}

void SimAudio::load(const QJsonObject &config)
{
    if (m_engine->isNullOutput()) {
        // Nothing to play to: present the scenario's simulated outputs.
        QStringList devices;
        const QJsonArray array = config.value(u"devices"_s).toArray();
        for (const QJsonValue &value : array)
            devices.append(value.toString());
        setOutputDevices(devices);
        setOutputDevice(config.value(u"output"_s).toString(devices.value(0)));
        setOutputAvailable(false);
    }
    setVolume(config.value(u"volume"_s).toDouble(0.6));

    if (m_persistent) {
        const bool persistent = std::exchange(m_persistent, false); // no saving while restoring
        QSettings store;
        store.beginGroup(u"audio"_s);
        setVolume(store.value(u"volume"_s, volume()).toDouble());
        setMuted(store.value(u"muted"_s, muted()).toBool());
        setSystemSounds(store.value(u"systemSounds"_s, systemSounds()).toBool());
        setSystemVolume(store.value(u"systemVolume"_s, systemVolume()).toDouble());
        applyEqualizer(store.value(u"equalizer"_s, equalizer()).toList(),
                       store.value(u"equalizerPreset"_s, equalizerPreset()).toString());
        m_preferredOutput = store.value(u"output"_s).toString();
        store.endGroup();
        m_persistent = persistent;
    }
    m_engine->setVolume(volume());
    m_engine->setMuted(muted());
}

void SimAudio::applyOutputs(const QList<audio::OutputDevice> &outputs, const QString &currentId)
{
    if (m_engine->isNullOutput())
        return;
    m_outputs = outputs;
    QStringList names;
    QString current;
    for (const audio::OutputDevice &output : outputs) {
        names.append(output.name);
        if (output.id == currentId)
            current = output.name;
    }
    setOutputDevices(names);
    setOutputDevice(current.isEmpty() ? tr("No output") : current);
    setOutputAvailable(!currentId.isEmpty());

    // Once the outputs are known, return to the one chosen last time.
    if (!m_restoredOutput && !outputs.isEmpty()) {
        m_restoredOutput = true;
        if (!m_preferredOutput.isEmpty() && m_preferredOutput != current && names.contains(m_preferredOutput))
            selectOutput(m_preferredOutput);
    }
}

void SimAudio::selectOutput(const QString &device)
{
    if (m_engine->isNullOutput()) {
        if (outputDevices().contains(device))
            setOutputDevice(device);
        return;
    }
    for (const audio::OutputDevice &output : std::as_const(m_outputs)) {
        if (output.name == device) {
            m_preferredOutput = device;
            m_engine->selectOutput(output.id);
            save();
            return;
        }
    }
}

void SimAudio::applyEqualizer(const QVariantList &gains, const QString &preset)
{
    QVariantList clamped;
    QList<double> engineGains;
    for (int band = 0; band < audio::Equalizer::kBands; ++band) {
        const double gain =
            std::clamp(gains.value(band).toDouble(), -audio::Equalizer::kMaxGainDb, audio::Equalizer::kMaxGainDb);
        clamped.append(gain);
        engineGains.append(gain);
    }
    setEqualizer(clamped);
    setEqualizerPreset(equalizerPresets().contains(preset) ? preset : tr("Custom"));
    m_engine->setEqualizer(engineGains);
}

void SimAudio::setEqualizerBand(int band, double gainDb)
{
    if (band < 0 || band >= audio::Equalizer::kBands)
        return;
    QVariantList gains = equalizer();
    gains[band] = std::round(gainDb * 2.0) / 2.0; // half-decibel steps
    QString preset = tr("Custom");
    for (const EqualizerPreset &candidate : kEqualizerPresets)
        if (presetGains(candidate) == gains)
            preset = QString::fromUtf16(candidate.name);
    applyEqualizer(gains, preset);
    save();
}

void SimAudio::applyEqualizerPreset(const QString &name)
{
    for (const EqualizerPreset &preset : kEqualizerPresets) {
        if (QString::fromUtf16(preset.name) == name) {
            applyEqualizer(presetGains(preset), name);
            save();
            return;
        }
    }
}

void SimAudio::forward(const audio::AnalysisFrame &frame)
{
    QVariantList bands;
    bands.reserve(audio::AnalysisFrame::kBands);
    for (float band : frame.bands)
        bands.append(double(band));
    setActive(frame.active);
    Q_EMIT analysis(bands, frame.level, frame.reactive, frame.beat, frame.beatStrength);
}

void SimAudio::save()
{
    if (!m_persistent)
        return;
    QSettings store;
    store.beginGroup(u"audio"_s);
    store.setValue(u"volume"_s, volume());
    store.setValue(u"muted"_s, muted());
    store.setValue(u"systemSounds"_s, systemSounds());
    store.setValue(u"systemVolume"_s, systemVolume());
    store.setValue(u"equalizer"_s, equalizer());
    store.setValue(u"equalizerPreset"_s, equalizerPreset());
    store.setValue(u"output"_s, m_preferredOutput);
    store.endGroup();
}

} // namespace qor::sim
