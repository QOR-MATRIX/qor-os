// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "rep_media_source.h"
#include "rep_notifications_source.h"
#include "rep_settings_source.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QTimer>

namespace qtos::sim {

// Simulated now-playing session with a queue that advances in real time.
class SimMedia : public MediaSimpleSource
{
    Q_OBJECT
public:
    explicit SimMedia(QObject *parent = nullptr);
    void load(const QJsonObject &config);

    void togglePlayback() override;
    void next() override;
    void previous() override;
    void seek(double seconds) override;
    void playIndex(int index) override;

private:
    void applyTrack(int index);
    void tick();

    QTimer m_clock;
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

} // namespace qtos::sim
