// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QTOS.Kit
import QTOS.Services

// Control Center: connectivity, appearance, display, sound, media and power.
// Every control is a live binding to a system-service replica.
Item {
    id: center

    property bool open: false
    property Backdrop backdrop
    signal dismissed()
    signal actionRequested(string id, var args)

    property string expanded: ""   // "", "wifi", "bluetooth"

    width: 368
    height: column.implicitHeight + 28
    visible: presence.value > 0.002
    opacity: presence.value
    transformOrigin: Item.TopRight
    scale: 0.94 + 0.06 * presence.value

    Spring { id: presence; target: center.open ? 1 : 0; preset: Motion.snappy }
    onOpenChanged: if (!open) expanded = ""

    Glass {
        anchors.fill: parent
        backdrop: center.backdrop
        radius: 24
        elevation: 5
    }

    Column {
        id: column
        x: 14
        y: 14
        width: parent.width - 28
        spacing: 12

        // ── Connectivity ────────────────────────────────────────────────────
        Row {
            width: parent.width
            spacing: 10
            ConnectivityTile {
                width: (parent.width - 10) / 2
                icon: Services.network.wifiEnabled ? "wifi" : "wifi-off"
                title: qsTr("Wi-Fi")
                subtitle: !Services.network.wifiEnabled ? qsTr("Off")
                          : Services.network.connecting ? qsTr("Connecting…")
                          : Services.network.online ? Services.network.activeSsid : qsTr("Not connected")
                checked: Services.network.wifiEnabled
                expandedHere: center.expanded === "wifi"
                onToggled: Services.network.wifiEnabled = !Services.network.wifiEnabled
                onExpandToggled: center.expanded = center.expanded === "wifi" ? "" : "wifi"
            }
            ConnectivityTile {
                width: (parent.width - 10) / 2
                icon: "bluetooth"
                title: qsTr("Bluetooth")
                subtitle: {
                    if (!Services.bluetooth.enabled)
                        return qsTr("Off");
                    const n = Services.bluetooth.devices.filter(d => d.connected).length;
                    return n > 0 ? qsTr("%n device(s)", "", n) : qsTr("On");
                }
                checked: Services.bluetooth.enabled
                expandedHere: center.expanded === "bluetooth"
                onToggled: Services.bluetooth.enabled = !Services.bluetooth.enabled
                onExpandToggled: center.expanded = center.expanded === "bluetooth" ? "" : "bluetooth"
            }
        }

        // Expanded network / device list.
        Surface {
            width: parent.width
            height: center.expanded === "" ? 0 : listColumn.implicitHeight + 12
            visible: height > 0
            radius: Tokens.radius.l
            color: Theme.fillControl
            clip: true
            Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

            Column {
                id: listColumn
                x: 6
                y: 6
                width: parent.width - 12
                Repeater {
                    model: center.expanded === "wifi" ? Services.network.accessPoints
                           : center.expanded === "bluetooth" ? Services.bluetooth.devices : []
                    delegate: ItemDelegate {
                        id: entry
                        required property var modelData
                        readonly property bool isWifi: center.expanded === "wifi"
                        readonly property bool active: isWifi ? modelData.ssid === Services.network.activeSsid
                                                              : modelData.connected
                        width: listColumn.width
                        height: 32
                        text: isWifi ? modelData.ssid : modelData.name
                        icon.name: isWifi ? ["wifi-zero", "wifi-low", "wifi-high", "wifi"][Math.min(3, modelData.bars)]
                                          : ({ headphones: "headphones", keyboard: "keyboard", mouse: "app-window",
                                               speaker: "speaker" })[modelData.kind] ?? "bluetooth"
                        highlighted: active
                        onClicked: {
                            if (isWifi)
                                Services.network.connectTo(modelData.ssid);
                            else
                                Services.bluetooth.toggleDevice(modelData.name);
                        }
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: !entry.isWifi && entry.modelData.battery >= 0
                                text: entry.isWifi ? "" : entry.modelData.battery + "%"
                                font: Theme.caption
                                color: Theme.textTertiary
                            }
                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: entry.isWifi ? entry.modelData.secure : false
                                name: "lock"
                                size: 12
                                color: Theme.textTertiary
                            }
                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: entry.active
                                name: "check"
                                size: 14
                                color: Theme.accentText
                            }
                        }
                    }
                }
            }
        }

        // ── Quick toggles ───────────────────────────────────────────────────
        Row {
            width: parent.width
            spacing: 8
            QuickToggle {
                icon: "moon-star"
                label: qsTr("Focus")
                checked: Services.notifications.doNotDisturb
                onToggled: Services.notifications.doNotDisturb = !Services.notifications.doNotDisturb
            }
            QuickToggle {
                icon: Services.settings.darkMode ? "moon" : "sun"
                label: Services.settings.darkMode ? qsTr("Dark") : qsTr("Light")
                checked: Services.settings.darkMode
                onToggled: Services.settings.darkMode = !Services.settings.darkMode
            }
            QuickToggle {
                icon: "sun"
                label: qsTr("Night Light")
                checked: Services.display.nightLight
                onToggled: Services.display.nightLight = !Services.display.nightLight
            }
            QuickToggle {
                icon: "sparkles"
                label: qsTr("Living")
                checked: Services.settings.livingWallpaper
                onToggled: Services.settings.livingWallpaper = !Services.settings.livingWallpaper
            }
        }

        // ── Sliders ─────────────────────────────────────────────────────────
        Surface {
            width: parent.width
            height: 112
            radius: Tokens.radius.l
            color: Theme.fillControl

            Column {
                x: 14
                y: 12
                width: parent.width - 28
                spacing: 6
                Text { text: qsTr("Display"); font: Theme.footnote; color: Theme.textSecondary }
                Row {
                    width: parent.width
                    spacing: 10
                    Icon { anchors.verticalCenter: parent.verticalCenter; name: "sun"; size: 15; color: Theme.textSecondary }
                    Slider {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 25
                        from: 0.2
                        to: 1
                        value: Services.display.brightness
                        onMoved: Services.display.brightness = value
                    }
                }
                Text {
                    text: qsTr("Sound · %1").arg(Services.audio.outputDevice)
                    font: Theme.footnote
                    color: Theme.textSecondary
                }
                Row {
                    width: parent.width
                    spacing: 10
                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: Services.audio.muted ? "volume-x" : "volume-2"
                        size: 15
                        color: Theme.textSecondary
                        TapHandler { onTapped: Services.audio.muted = !Services.audio.muted }
                    }
                    Slider {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 25
                        from: 0
                        to: 1
                        value: Services.audio.volume
                        onMoved: Services.audio.volume = value
                        onPressedChanged: if (!pressed) Sounds.play("volume")
                    }
                }
            }
        }

        // ── Now playing ─────────────────────────────────────────────────────
        Surface {
            width: parent.width
            height: 72
            visible: Services.media.available
            radius: Tokens.radius.l
            color: Theme.fillControl

            Artwork {
                x: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 52
                height: 52
                hue: Services.media.artworkHue
            }
            Column {
                x: 74
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 74 - 118
                Text {
                    width: parent.width
                    text: Services.media.title
                    font: Theme.bodyStrong
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: Services.media.artist
                    font: Theme.footnote
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                }
                Spectrum {
                    objectName: "controlCenterSpectrum"
                    width: Math.min(parent.width, 96)
                    height: 10
                    visible: Reactive.playing
                    bars: 16
                    spacing: 2
                    minimum: 0.15
                    values: Reactive.spectrum
                    color: Oklch.color(Theme.darkness > 0.5 ? 0.78 : 0.6, 0.15, Services.media.artworkHue)
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                IconButton { icon: "skip-back"; width: 34; height: 34; onClicked: Services.media.previous() }
                IconButton {
                    icon: Services.media.playing ? "pause" : "play"
                    iconSize: 18
                    width: 38
                    height: 34
                    color: Theme.textPrimary
                    onClicked: Services.media.togglePlayback()
                }
                IconButton { icon: "skip-forward"; width: 34; height: 34; onClicked: Services.media.next() }
            }
        }

        // ── Accent ──────────────────────────────────────────────────────────
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            Repeater {
                model: Tokens.accentPresets
                delegate: Surface {
                    id: swatch
                    required property real modelData
                    readonly property bool selected: Math.abs(Services.settings.accentHue - modelData) < 0.5
                    width: 24
                    height: 24
                    radius: 12
                    exponent: 2
                    color: Oklch.color(0.72, 0.16, modelData + 10)
                    gradientTo: Oklch.color(0.56, 0.18, modelData - 10)
                    strokeColor: selected ? Theme.textPrimary : Qt.rgba(1, 1, 1, 0.25)
                    strokeWidth: selected ? 2 : 1
                    scale: swatchHover.hovered ? 1.12 : 1
                    Behavior on scale { NumberAnimation { duration: 120 } }
                    HoverHandler { id: swatchHover }
                    TapHandler { onTapped: Services.settings.accentHue = swatch.modelData }
                }
            }
        }

        // ── Footer ──────────────────────────────────────────────────────────
        Item {
            width: parent.width
            height: 32
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: Services.power.charging ? "battery-charging" : "battery-medium"
                    size: 18
                    color: Theme.textSecondary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        const p = Services.power;
                        const mins = p.minutesRemaining;
                        const left = mins < 0 ? "" : " · " + (Math.floor(mins / 60) + "h " + (mins % 60) + "m")
                                                       + (p.charging ? qsTr(" to full") : qsTr(" left"));
                        return p.batteryLevel + "%" + left;
                    }
                    font: Theme.footnote
                    color: Theme.textSecondary
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                IconButton {
                    icon: "settings"
                    onClicked: Intents.openApp("settings", {})
                }
                IconButton {
                    icon: "lock"
                    onClicked: center.actionRequested("session.lock", {})
                }
            }
        }
    }

    // ── Components ──────────────────────────────────────────────────────────
    component ConnectivityTile: Surface {
        id: tile
        property string icon
        property string title
        property string subtitle
        property bool checked: false
        property bool expandedHere: false
        signal toggled()
        signal expandToggled()

        height: 60
        radius: Tokens.radius.l
        color: tileHover.hovered ? Theme.fillControlHover : Theme.fillControl
        Behavior on color { ColorAnimation { duration: 120 } }

        Surface {
            id: badge
            x: 10
            anchors.verticalCenter: parent.verticalCenter
            width: 34
            height: 34
            radius: 17
            exponent: 2
            color: tile.checked ? Theme.accent : Theme.fillControlPressed
            highlight: tile.checked ? 0.8 : 0
            Behavior on color { ColorAnimation { duration: 180 } }
            Icon {
                anchors.centerIn: parent
                name: tile.icon
                size: 17
                color: tile.checked ? Theme.onAccent : Theme.textPrimary
            }
            TapHandler { onTapped: tile.toggled() }
        }
        Column {
            anchors.left: badge.right
            anchors.leftMargin: 10
            anchors.right: chevron.left
            anchors.verticalCenter: parent.verticalCenter
            Text { text: tile.title; font: Theme.bodyStrong; color: Theme.textPrimary }
            Text {
                width: parent.width
                text: tile.subtitle
                font: Theme.caption
                color: Theme.textSecondary
                elide: Text.ElideRight
            }
        }
        IconButton {
            id: chevron
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            height: 40
            icon: tile.expandedHere ? "chevron-up" : "chevron-down"
            iconSize: 14
            onClicked: tile.expandToggled()
        }
        HoverHandler { id: tileHover }
    }

    component QuickToggle: Surface {
        id: toggle
        property string icon
        property string label
        property bool checked: false
        signal toggled()

        width: (column.width - 24) / 4
        height: 74
        radius: Tokens.radius.l
        color: checked ? Oklch.alpha(Theme.accent, 0.22) : toggleHover.hovered ? Theme.fillControlHover : Theme.fillControl
        strokeColor: checked ? Oklch.alpha(Theme.accent, 0.5) : "transparent"
        scale: press.value
        Behavior on color { ColorAnimation { duration: 160 } }

        Spring { id: press; target: toggleTap.pressed ? 0.94 : 1; preset: Motion.snappy }

        Column {
            anchors.centerIn: parent
            spacing: 6
            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: toggle.icon
                size: 20
                color: toggle.checked ? Theme.accentText : Theme.textPrimary
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: toggle.label
                font: Theme.caption
                color: Theme.textSecondary
            }
        }
        HoverHandler { id: toggleHover }
        TapHandler { id: toggleTap; onTapped: toggle.toggled() }
    }
}
