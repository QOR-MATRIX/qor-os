// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "services.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QLoggingCategory>
#include <QtQml/QJSEngine>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcServices, "qor.services")

namespace qor {

Services::Services(QObject *parent) : QObject(parent) { }

Services *Services::instance()
{
    static Services *services = new Services(qApp);
    return services;
}

Services *Services::create(QQmlEngine *, QJSEngine *)
{
    Services *services = instance();
    QJSEngine::setObjectOwnership(services, QJSEngine::CppOwnership);
    return services;
}

QUrl Services::defaultUrl()
{
#if defined(Q_OS_LINUX)
    // Abstract sockets never leave stale files behind after a crash.
    return QUrl(u"localabstract:qor-services"_s);
#else
    return QUrl(u"local:qor-services"_s);
#endif
}

bool Services::connectTo(const QUrl &url, int timeoutMs)
{
    if (!m_node.connectToNode(url)) {
        qCWarning(lcServices) << "Cannot connect to services host" << url << m_node.lastError();
        return false;
    }
    m_url = url;

    m_network.reset(m_node.acquire<NetworkReplica>());
    m_bluetooth.reset(m_node.acquire<BluetoothReplica>());
    m_power.reset(m_node.acquire<PowerReplica>());
    m_audio.reset(m_node.acquire<AudioReplica>());
    m_display.reset(m_node.acquire<DisplayReplica>());
    m_media.reset(m_node.acquire<MediaReplica>());
    m_notifications.reset(m_node.acquire<NotificationsReplica>());
    m_settings.reset(m_node.acquire<SettingsReplica>());

    const QDeadlineTimer deadline(timeoutMs);
    const QList<QRemoteObjectReplica *> replicas { m_network.get(),       m_bluetooth.get(), m_power.get(),
                                                   m_audio.get(),         m_display.get(),   m_media.get(),
                                                   m_notifications.get(), m_settings.get() };
    bool allReady = true;
    for (QRemoteObjectReplica *replica : replicas) {
        const int remaining = int(qMax<qint64>(1, deadline.remainingTime()));
        if (!replica->isInitialized() && !replica->waitForSource(remaining)) {
            qCWarning(lcServices) << "Service did not initialise in time:" << replica->metaObject()->className();
            allReady = false;
        }
    }

    m_connected = allReady;
    Q_EMIT connectedChanged();
    qCInfo(lcServices) << "Connected to services at" << url << (allReady ? "(all ready)" : "(partial)");
    return allReady;
}

} // namespace qor
