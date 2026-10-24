// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "rep_audio_replica.h"
#include "rep_bluetooth_replica.h"
#include "rep_display_replica.h"
#include "rep_media_replica.h"
#include "rep_network_replica.h"
#include "rep_notifications_replica.h"
#include "rep_power_replica.h"
#include "rep_settings_replica.h"

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtQml/qqmlregistration.h>
#include <QtRemoteObjects/QRemoteObjectNode>

#include <memory>

class QQmlEngine;
class QJSEngine;

namespace qtos {

// The shell's view of the QTOS system services, exposed to QML as the
// `Services` singleton.
//
// Every property is a Qt Remote Objects *replica*: the shell never talks to a
// service implementation directly. In simulation the sources live in qtos-sim
// (in-process or in qtos-simd); on device they are the qtos-*d daemons. The
// QML code is identical in both cases.
class Services : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(NetworkReplica *network READ network NOTIFY connectedChanged FINAL)
    Q_PROPERTY(BluetoothReplica *bluetooth READ bluetooth NOTIFY connectedChanged FINAL)
    Q_PROPERTY(PowerReplica *power READ power NOTIFY connectedChanged FINAL)
    Q_PROPERTY(AudioReplica *audio READ audio NOTIFY connectedChanged FINAL)
    Q_PROPERTY(DisplayReplica *display READ display NOTIFY connectedChanged FINAL)
    Q_PROPERTY(MediaReplica *media READ media NOTIFY connectedChanged FINAL)
    Q_PROPERTY(NotificationsReplica *notifications READ notifications NOTIFY connectedChanged FINAL)
    Q_PROPERTY(SettingsReplica *settings READ settings NOTIFY connectedChanged FINAL)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged FINAL)
    Q_PROPERTY(QUrl url READ url NOTIFY connectedChanged FINAL)

public:
    static Services *instance();
    static Services *create(QQmlEngine *, QJSEngine *);

    // The default endpoint of the services host on this platform.
    static QUrl defaultUrl();

    // Connects to the services host and waits (up to timeoutMs) until every
    // replica has received its initial state from its source.
    bool connectTo(const QUrl &url, int timeoutMs = 3000);

    NetworkReplica *network() const { return m_network.get(); }
    BluetoothReplica *bluetooth() const { return m_bluetooth.get(); }
    PowerReplica *power() const { return m_power.get(); }
    AudioReplica *audio() const { return m_audio.get(); }
    DisplayReplica *display() const { return m_display.get(); }
    MediaReplica *media() const { return m_media.get(); }
    NotificationsReplica *notifications() const { return m_notifications.get(); }
    SettingsReplica *settings() const { return m_settings.get(); }

    bool isConnected() const { return m_connected; }
    QUrl url() const { return m_url; }

Q_SIGNALS:
    void connectedChanged();

private:
    explicit Services(QObject *parent = nullptr);

    QRemoteObjectNode m_node;
    QUrl m_url;
    bool m_connected = false;
    std::unique_ptr<NetworkReplica> m_network;
    std::unique_ptr<BluetoothReplica> m_bluetooth;
    std::unique_ptr<PowerReplica> m_power;
    std::unique_ptr<AudioReplica> m_audio;
    std::unique_ptr<DisplayReplica> m_display;
    std::unique_ptr<MediaReplica> m_media;
    std::unique_ptr<NotificationsReplica> m_notifications;
    std::unique_ptr<SettingsReplica> m_settings;
};

} // namespace qtos

// QML type information for the repc-generated replica classes, so qmllint and
// the QML compilers type-check `Services.network.wifiEnabled` and friends.
// Declared in the global namespace, next to the generated classes, so
// QML_FOREIGN resolves them by their unqualified names.

struct NetworkServiceForeign
{
    Q_GADGET
    QML_FOREIGN(NetworkReplica)
    QML_NAMED_ELEMENT(NetworkService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct BluetoothServiceForeign
{
    Q_GADGET
    QML_FOREIGN(BluetoothReplica)
    QML_NAMED_ELEMENT(BluetoothService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct PowerServiceForeign
{
    Q_GADGET
    QML_FOREIGN(PowerReplica)
    QML_NAMED_ELEMENT(PowerService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct AudioServiceForeign
{
    Q_GADGET
    QML_FOREIGN(AudioReplica)
    QML_NAMED_ELEMENT(AudioService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct DisplayServiceForeign
{
    Q_GADGET
    QML_FOREIGN(DisplayReplica)
    QML_NAMED_ELEMENT(DisplayService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct MediaServiceForeign
{
    Q_GADGET
    QML_FOREIGN(MediaReplica)
    QML_NAMED_ELEMENT(MediaService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct NotificationsServiceForeign
{
    Q_GADGET
    QML_FOREIGN(NotificationsReplica)
    QML_NAMED_ELEMENT(NotificationsService)
    QML_UNCREATABLE("Provided by the Services singleton")
};

struct SettingsServiceForeign
{
    Q_GADGET
    QML_FOREIGN(SettingsReplica)
    QML_NAMED_ELEMENT(SettingsService)
    QML_UNCREATABLE("Provided by the Services singleton")
};
