// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services

// The Rail: a floating glass bar. Left: the QTOS mark, the active app and
// the Space indicator. Centre: the Live Activity capsule. Right: system
// status and the clock, which open Control Center.
Glass {
    id: rail

    property string activeTitle
    property int currentSpace: 0
    property int spaceCount: 3
    property string surface

    signal surfaceRequested(string name)

    radius: 13
    elevation: 2
    highlight: 0.7

    property date now: new Date()
    Timer {
        // Wake once per minute, aligned to the minute boundary.
        id: clockTimer
        running: true
        interval: 60000 - (Date.now() % 60000) + 20
        onTriggered: {
            rail.now = new Date();
            interval = 60000 - (Date.now() % 60000) + 20;
            restart();
        }
    }

    // ── Left ────────────────────────────────────────────────────────────────
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Item {
            width: 22
            height: 22
            anchors.verticalCenter: parent.verticalCenter
            QtosMark {
                anchors.centerIn: parent
                size: 20
                scale: markHover.hovered ? 1.08 : 1
                Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
            }
            HoverHandler { id: markHover }
            TapHandler { onTapped: rail.surfaceRequested("beam") }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: rail.activeTitle.length > 0 ? rail.activeTitle : qsTr("Desktop")
            font: Theme.bodyStrong
            color: Theme.textPrimary
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            Repeater {
                model: rail.spaceCount
                delegate: Surface {
                    id: dot
                    required property int index
                    readonly property bool current: index === rail.currentSpace
                    anchors.verticalCenter: parent.verticalCenter
                    width: current ? 14 : 5
                    height: 5
                    radius: 2.5
                    exponent: 2
                    color: current ? Theme.textPrimary : Theme.textTertiary
                    Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                    TapHandler { onTapped: Intents.request("shell.space", { "index": dot.index }) }
                }
            }
        }
    }

    // ── Centre ──────────────────────────────────────────────────────────────
    LiveActivity {
        anchors.centerIn: parent
        expanded: rail.surface === "media"
        onToggled: rail.surfaceRequested("media")
    }

    // ── Right ───────────────────────────────────────────────────────────────
    Item {
        id: status
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        width: statusRow.implicitWidth + 16
        height: parent.height - 6

        readonly property var network: Services.network
        readonly property var power: Services.power
        readonly property var audio: Services.audio

        Surface {
            anchors.fill: parent
            radius: 10
            color: rail.surface === "control-center" ? Theme.fillControlPressed
                                                     : statusHover.hovered ? Theme.fillControlHover : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
        }

        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: 12

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: Services.notifications.doNotDisturb
                name: "moon"
                size: 15
                color: Theme.textPrimary
            }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: !Services.network.wifiEnabled ? "wifi-off"
                      : Services.network.connecting ? "wifi-low"
                      : ["wifi-zero", "wifi-low", "wifi-high", "wifi"][Math.max(0, Math.min(3, Services.network.signalBars))]
                size: 16
                color: Services.network.online ? Theme.textPrimary : Theme.textTertiary
                SequentialAnimation on opacity {
                    running: Services.network.connecting
                    loops: Animation.Infinite
                    alwaysRunToEnd: true
                    NumberAnimation { to: 0.35; duration: 450 }
                    NumberAnimation { to: 1; duration: 450 }
                }
            }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: Services.bluetooth.enabled
                name: Services.bluetooth.devices.some(d => d.connected) ? "bluetooth-connected" : "bluetooth"
                size: 15
                color: Theme.textPrimary
            }
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                readonly property real level: Services.audio.muted ? 0 : Services.audio.volume
                name: level <= 0.001 ? "volume-x" : level < 0.34 ? "volume" : level < 0.67 ? "volume-1" : "volume-2"
                size: 16
                color: Theme.textPrimary
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: Services.power.batteryLevel + "%"
                    font: Theme.footnote
                    color: Theme.textSecondary
                }
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    readonly property int level: Services.power.batteryLevel
                    name: Services.power.charging ? "battery-charging"
                          : level > 80 ? "battery-full" : level > 45 ? "battery-medium" : level > 15 ? "battery-low" : "battery"
                    size: 18
                    color: !Services.power.charging && level <= 15 ? Theme.critical : Theme.textPrimary
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: rail.now.toLocaleDateString(Qt.locale(), "ddd d MMM") + "   "
                      + rail.now.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                font: Theme.bodyStrong
                color: Theme.textPrimary
            }
        }

        HoverHandler { id: statusHover }
        TapHandler { onTapped: rail.surfaceRequested("control-center") }
    }
}
