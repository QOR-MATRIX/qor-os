// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "simsession.h"

#include <QtCore/QSettings>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace qtos::sim {

namespace {
constexpr int kMediaTickMs = 250;
}

// ── Media ────────────────────────────────────────────────────────────────────

SimMedia::SimMedia(QObject *parent) : MediaSimpleSource(parent)
{
    m_clock.setInterval(kMediaTickMs);
    connect(&m_clock, &QTimer::timeout, this, &SimMedia::tick);
    connect(this, &MediaSimpleSource::playingChanged, this, [this] {
        if (playing())
            m_clock.start();
        else
            m_clock.stop();
    });
}

void SimMedia::load(const QJsonObject &config)
{
    setPlayer(config.value(u"player"_s).toString(u"Music"_s));
    setQueue(config.value(u"queue"_s).toArray().toVariantList());
    setAvailable(!queue().isEmpty());
    applyTrack(config.value(u"index"_s).toInt(0));
    seek(config.value(u"position"_s).toDouble(0));
    setPlaying(config.value(u"playing"_s).toBool(false));
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
    setDuration(track.value(u"duration"_s, 200.0).toDouble());
    setPosition(0);
}

void SimMedia::tick()
{
    const double next = position() + kMediaTickMs / 1000.0;
    if (next >= duration())
        this->next();
    else
        setPosition(next);
}

void SimMedia::togglePlayback() { setPlaying(!playing()); }

void SimMedia::next() { applyTrack(queueIndex() + 1); }

void SimMedia::previous()
{
    if (position() > 3.0)
        setPosition(0);
    else
        applyTrack(queueIndex() - 1);
}

void SimMedia::seek(double seconds) { setPosition(std::clamp(seconds, 0.0, duration())); }

void SimMedia::playIndex(int index)
{
    applyTrack(index);
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
    }

    for (auto signal :
         { &SettingsSimpleSource::darkModeChanged, &SettingsSimpleSource::reduceMotionChanged,
           &SettingsSimpleSource::reduceTransparencyChanged, &SettingsSimpleSource::increaseContrastChanged,
           &SettingsSimpleSource::livingWallpaperChanged })
        connect(this, signal, this, &SimSettings::save);
    connect(this, &SettingsSimpleSource::accentHueChanged, this, &SimSettings::save);
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
}

} // namespace qtos::sim
