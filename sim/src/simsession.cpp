// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "simsession.h"

#include "engine.h"
#include "library.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRandomGenerator>
#include <QtCore/QSettings>

#include <algorithm>
#include <utility>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcSimMedia, "qtos.sim.media")

namespace qtos::sim {

// ── Media ────────────────────────────────────────────────────────────────────

SimMedia::SimMedia(audio::Engine *engine, bool persistent, QObject *parent)
    : MediaSimpleSource(parent), m_engine(engine), m_persistent(persistent)
{
    connect(this, &MediaSimpleSource::playingChanged, this, [this] { m_engine->setPlaying(playing()); });
    connect(this, &MediaSimpleSource::shuffleChanged, this, [this] {
        reshuffle();
        save();
    });
    connect(this, &MediaSimpleSource::repeatChanged, this, [this] {
        if (repeat() < 0 || repeat() > 2)
            setRepeat(1);
        save();
    });
    connect(m_engine, &audio::Engine::positionChanged, this, [this](double seconds) {
        setPosition(duration() > 0 ? std::min(seconds, duration()) : seconds);
        m_failures = 0;
    });
    connect(m_engine, &audio::Engine::durationChanged, this, &SimMedia::updateQueuedDuration);
    connect(m_engine, &audio::Engine::bufferingChanged, this, &SimMedia::setBuffering);
    connect(m_engine, &audio::Engine::finished, this, &SimMedia::trackFinished);
    connect(m_engine, &audio::Engine::loadFailed, this, &SimMedia::skipUnplayable);
    connect(&m_library, &QFutureWatcher<QVariantList>::finished, this, [this] {
        const QVariantList files = m_library.result();
        if (files.isEmpty())
            return;
        qCInfo(lcSimMedia) << "Queued" << files.size() << "music files";
        setQueue(queue() + files);
        setAvailable(true);
        reshuffle();
    });
}

void SimMedia::load(const QJsonObject &config, bool library)
{
    setPlayer(config.value(u"player"_s).toString(u"Music"_s));
    setQueue(config.value(u"queue"_s).toArray().toVariantList());
    setAvailable(!queue().isEmpty());
    applyTrack(config.value(u"index"_s).toInt(0));
    if (const double position = config.value(u"position"_s).toDouble(0); position > 0)
        seek(position);
    if (m_persistent) {
        QSettings store;
        store.beginGroup(u"media"_s);
        const bool persistent = std::exchange(m_persistent, false); // no saving while restoring
        setShuffle(store.value(u"shuffle"_s, shuffle()).toBool());
        setRepeat(store.value(u"repeat"_s, repeat()).toInt());
        m_persistent = persistent;
        store.endGroup();
    }
    setPlaying(config.value(u"playing"_s).toBool(false));
    if (library)
        m_library.setFuture(QtConcurrent::run([roots = audio::libraryRoots()] { return audio::scanLibrary(roots); }));
}

void SimMedia::applyTrack(int index)
{
    const QVariantList tracks = queue();
    if (tracks.isEmpty())
        return;
    index = ((index % tracks.size()) + tracks.size()) % tracks.size();
    const QVariantMap track = tracks.at(index).toMap();
    setQueueIndex(index);
    setTitle(track.value(u"title"_s).toString());
    setArtist(track.value(u"artist"_s).toString());
    setAlbum(track.value(u"album"_s).toString());
    setArtworkHue(track.value(u"hue"_s, 280.0).toDouble());
    setDuration(track.value(u"duration"_s, 0.0).toDouble());
    setPosition(0);
    m_engine->load(track);
}

// Files learn their length once decoding starts; the queue remembers it.
void SimMedia::updateQueuedDuration(double seconds)
{
    if (seconds <= 0)
        return;
    setDuration(seconds);
    QVariantList tracks = queue();
    const int index = queueIndex();
    if (index < 0 || index >= tracks.size())
        return;
    QVariantMap track = tracks.at(index).toMap();
    if (qFuzzyCompare(track.value(u"duration"_s).toDouble(), seconds))
        return;
    track.insert(u"duration"_s, seconds);
    tracks[index] = track;
    setQueue(tracks);
}

void SimMedia::skipUnplayable(const QString &message)
{
    qCWarning(lcSimMedia) << "Skipping" << title() << "-" << message;
    // Stop rather than spin if nothing in the queue plays.
    if (++m_failures >= queue().size()) {
        setPlaying(false);
        m_failures = 0;
        return;
    }
    next();
}

void SimMedia::togglePlayback() { setPlaying(!playing()); }

int SimMedia::stepFrom(int steps) const
{
    const int count = int(queue().size());
    if (count == 0)
        return 0;
    if (!shuffle() || m_order.size() != count)
        return ((queueIndex() + steps) % count + count) % count;
    const int at = std::max(0, int(m_order.indexOf(queueIndex())));
    return m_order.at(((at + steps) % count + count) % count);
}

// A new random order that starts with the current track, so turning shuffle
// on never interrupts what is playing.
void SimMedia::reshuffle()
{
    m_order.clear();
    if (!shuffle())
        return;
    for (int i = 0; i < queue().size(); ++i)
        if (i != queueIndex())
            m_order.append(i);
    std::shuffle(m_order.begin(), m_order.end(), *QRandomGenerator::global());
    m_order.prepend(queueIndex());
}

void SimMedia::next()
{
    const int index = stepFrom(1);
    // A full shuffled pass is over: deal a new order for the next one.
    if (shuffle() && !m_order.isEmpty() && index == m_order.first()) {
        applyTrack(index);
        reshuffle();
        return;
    }
    applyTrack(index);
}

void SimMedia::previous()
{
    if (position() > 3.0)
        seek(0);
    else
        applyTrack(stepFrom(-1));
}

void SimMedia::trackFinished()
{
    switch (repeat()) {
    case 2: // the track again
        seek(0);
        break;
    case 0: // stop after the last track, ready at the first
        if (stepFrom(1) == (shuffle() && !m_order.isEmpty() ? m_order.first() : 0)) {
            setPlaying(false);
            applyTrack(stepFrom(1));
            reshuffle();
            break;
        }
        next();
        break;
    default: next(); break;
    }
}

void SimMedia::save()
{
    if (!m_persistent)
        return;
    QSettings store;
    store.beginGroup(u"media"_s);
    store.setValue(u"shuffle"_s, shuffle());
    store.setValue(u"repeat"_s, repeat());
    store.endGroup();
}

void SimMedia::seek(double seconds)
{
    seconds = std::max(0.0, duration() > 0 ? std::min(seconds, duration()) : seconds);
    setPosition(seconds);
    m_engine->seek(seconds);
}

void SimMedia::playIndex(int index)
{
    applyTrack(index);
    // A chosen track starts the shuffled order afresh from itself.
    reshuffle();
    setPlaying(true);
}

// ── Notifications ────────────────────────────────────────────────────────────

SimNotifications::SimNotifications(QObject *parent) : NotificationsSimpleSource(parent) { }

void SimNotifications::schedule(const QJsonArray &timeline)
{
    qDeleteAll(m_pending);
    m_pending.clear();
    for (const QJsonValue &value : timeline) {
        const QJsonObject entry = value.toObject();
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(entry.value(u"atMs"_s).toInt(0));
        connect(timer, &QTimer::timeout, this, [this, entry] {
            notify(entry.value(u"app"_s).toString(u"system"_s), entry.value(u"title"_s).toString(),
                   entry.value(u"body"_s).toString(), entry.value(u"icon"_s).toString());
        });
        timer->start();
        m_pending.append(timer);
    }
}

void SimNotifications::notify(const QString &appId, const QString &title, const QString &body, const QString &icon)
{
    const int id = m_nextId++;
    setUnreadCount(unreadCount() + 1);
    Q_EMIT posted(id, appId, title, body, icon);
}

void SimNotifications::dismiss(int id)
{
    setUnreadCount(std::max(0, unreadCount() - 1));
    Q_EMIT dismissed(id);
}

void SimNotifications::markAllRead() { setUnreadCount(0); }

// ── Settings ─────────────────────────────────────────────────────────────────

SimSettings::SimSettings(bool persistent, QObject *parent) : SettingsSimpleSource(parent), m_persistent(persistent) { }

void SimSettings::load(const QJsonObject &config)
{
    setUserName(config.value(u"name"_s).toString(u"QTOS User"_s));
    setHostName(config.value(u"host"_s).toString(u"qtos-sim"_s));

    if (m_persistent) {
        QSettings store;
        store.beginGroup(u"appearance"_s);
        setDarkMode(store.value(u"darkMode"_s, darkMode()).toBool());
        setAccentHue(store.value(u"accentHue"_s, accentHue()).toDouble());
        setReduceMotion(store.value(u"reduceMotion"_s, reduceMotion()).toBool());
        setReduceTransparency(store.value(u"reduceTransparency"_s, reduceTransparency()).toBool());
        setIncreaseContrast(store.value(u"increaseContrast"_s, increaseContrast()).toBool());
        setLivingWallpaper(store.value(u"livingWallpaper"_s, livingWallpaper()).toBool());
        store.endGroup();
        store.beginGroup(u"reactive"_s);
        setAudioReactive(store.value(u"enabled"_s, audioReactive()).toBool());
        setReactiveIntensity(store.value(u"intensity"_s, reactiveIntensity()).toDouble());
        setReactiveLowHz(store.value(u"lowHz"_s, reactiveLowHz()).toDouble());
        setReactiveHighHz(store.value(u"highHz"_s, reactiveHighHz()).toDouble());
        store.endGroup();
        store.beginGroup(u"controller"_s);
        setControllerAutoSwitch(store.value(u"autoSwitch"_s, controllerAutoSwitch()).toBool());
        setControllerCursorSpeed(store.value(u"cursorSpeed"_s, controllerCursorSpeed()).toDouble());
        setControllerVibration(store.value(u"vibration"_s, controllerVibration()).toBool());
        setControllerLightbar(store.value(u"lightbar"_s, controllerLightbar()).toBool());
        store.endGroup();
    }

    for (auto signal :
         { &SettingsSimpleSource::darkModeChanged, &SettingsSimpleSource::reduceMotionChanged,
           &SettingsSimpleSource::reduceTransparencyChanged, &SettingsSimpleSource::increaseContrastChanged,
           &SettingsSimpleSource::livingWallpaperChanged, &SettingsSimpleSource::audioReactiveChanged,
           &SettingsSimpleSource::controllerAutoSwitchChanged, &SettingsSimpleSource::controllerVibrationChanged,
           &SettingsSimpleSource::controllerLightbarChanged })
        connect(this, signal, this, &SimSettings::save);
    for (auto signal : { &SettingsSimpleSource::accentHueChanged, &SettingsSimpleSource::reactiveIntensityChanged,
                         &SettingsSimpleSource::reactiveLowHzChanged, &SettingsSimpleSource::reactiveHighHzChanged,
                         &SettingsSimpleSource::controllerCursorSpeedChanged })
        connect(this, signal, this, &SimSettings::save);
}

void SimSettings::save()
{
    if (!m_persistent)
        return;
    QSettings store;
    store.beginGroup(u"appearance"_s);
    store.setValue(u"darkMode"_s, darkMode());
    store.setValue(u"accentHue"_s, accentHue());
    store.setValue(u"reduceMotion"_s, reduceMotion());
    store.setValue(u"reduceTransparency"_s, reduceTransparency());
    store.setValue(u"increaseContrast"_s, increaseContrast());
    store.setValue(u"livingWallpaper"_s, livingWallpaper());
    store.endGroup();
    store.beginGroup(u"reactive"_s);
    store.setValue(u"enabled"_s, audioReactive());
    store.setValue(u"intensity"_s, reactiveIntensity());
    store.setValue(u"lowHz"_s, reactiveLowHz());
    store.setValue(u"highHz"_s, reactiveHighHz());
    store.endGroup();
    store.beginGroup(u"controller"_s);
    store.setValue(u"autoSwitch"_s, controllerAutoSwitch());
    store.setValue(u"cursorSpeed"_s, controllerCursorSpeed());
    store.setValue(u"vibration"_s, controllerVibration());
    store.setValue(u"lightbar"_s, controllerLightbar());
    store.endGroup();
}

} // namespace qtos::sim
