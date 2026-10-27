// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "rep_media_source.h"
#include "rep_notifications_source.h"
#include "rep_settings_source.h"

#include <QtCore/QFutureWatcher>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QTimer>

namespace qor::audio {
class Engine;
}

namespace qor::sim {

// Now playing, played for real by the audio engine: the scenario's QOR OS
// Sessions (music generated live) followed by the user's music files.
class SimMedia : public MediaSimpleSource
{
    Q_OBJECT
public:
    SimMedia(audio::Engine *engine, bool persistent, QObject *parent = nullptr);
    // `library`: also queue the music files in the user's Music folder and
    // $QOR_MUSIC_DIR (read in the background).
    void load(const QJsonObject &config, bool library);

    void togglePlayback() override;
    void next() override;
    void previous() override;
    void seek(double seconds) override;
    void playIndex(int index) override;

private:
    void applyTrack(int index);
    void updateQueuedDuration(double seconds);
    void skipUnplayable(const QString &message);
    void trackFinished();
    // The queue index `steps` away from the current track in play order.
    int stepFrom(int steps) const;
    void reshuffle();
    void save();

    audio::Engine *m_engine;
    bool m_persistent;
    QFutureWatcher<QVariantList> m_library;
    int m_failures = 0;
    // Play order while shuffling: a permutation of the queue, current first.
    QList<int> m_order;
};

// Simulated notification daemon.
class SimNotifications : public NotificationsSimpleSource
{
    Q_OBJECT
public:
    explicit SimNotifications(QObject *parent = nullptr);

    // Schedules the scenario's timed notifications relative to now.
    void schedule(const QJsonArray &timeline);

    void notify(const QString &appId, const QString &title, const QString &body, const QString &icon) override;
    void dismiss(int id) override;
    void markAllRead() override;

private:
    int m_nextId = 1;
    QList<QTimer *> m_pending;
};

// Simulated settings daemon. Preferences persist with QSettings unless the
// simulator runs ephemerally (screenshots, tests).
class SimSettings : public SettingsSimpleSource
{
    Q_OBJECT
public:
    explicit SimSettings(bool persistent, QObject *parent = nullptr);
    void load(const QJsonObject &config);

private:
    void save();
    bool m_persistent;
};

} // namespace qor::sim
