// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QTOS.Kit
import QTOS.Services

// Settings: every control is a live binding to a QTOS service replica, so
// changes apply system-wide instantly (and persist via qtos-settingsd).
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    property string page: "appearance"

    onLaunchArgsChanged: if (launchArgs && launchArgs.page) page = launchArgs.page

    readonly property var pages: AppRegistry.settingsPages.filter(p => {
        const q = search.text.trim().toLowerCase();
        return q.length === 0 || p.title.toLowerCase().indexOf(q) >= 0 || p.keywords.indexOf(q) >= 0;
    })

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    FrostSidebar {
        id: sidebar
        width: 236
        height: parent.height
        topInset: app.topInset

        SearchField {
            id: search
            width: sidebar.width - 20
            placeholderText: qsTr("Search settings")
        }
        Item { width: 1; height: 8 }
        Repeater {
            model: app.pages
            delegate: SidebarItem {
                required property var modelData
                width: sidebar.width - 20
                icon: modelData.icon
                text: modelData.title
                selected: app.page === modelData.id
                onClicked: app.page = modelData.id
            }
        }
    }

    // ── Page host ───────────────────────────────────────────────────────────
    Flickable {
        id: scroller
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom
        contentHeight: pageLoader.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        Loader {
            id: pageLoader
            x: 32
            y: 12
            width: scroller.width - 64
            sourceComponent: {
                switch (app.page) {
                case "display": return displayPage;
                case "sound": return soundPage;
                case "network": return networkPage;
                case "bluetooth": return bluetoothPage;
                case "notifications": return notificationsPage;
                case "accessibility": return accessibilityPage;
                case "about": return aboutPage;
                default: return appearancePage;
                }
            }
            onLoaded: {
                enter.value = 0;
                enter.target = 1;
                scroller.contentY = 0;
            }
            opacity: enter.value
            transform: Translate { y: (1 - enter.value) * 10 }
            Spring { id: enter; target: 1; preset: Motion.smooth }
        }
    }

    // ── Pages ───────────────────────────────────────────────────────────────
    Component {
        id: appearancePage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Appearance") }

            Row {
                spacing: 16
                Repeater {
                    model: [{ dark: false, label: qsTr("Light") }, { dark: true, label: qsTr("Dark") }]
                    delegate: Column {
                        id: option
                        required property var modelData
                        readonly property bool chosen: Services.settings.darkMode === modelData.dark
                        spacing: 8
                        Surface {
                            width: 200
                            height: 124
                            radius: Tokens.radius.l
                            color: option.modelData.dark ? Oklch.color(0.2, 0.02, Theme.hue) : Oklch.color(0.96, 0.01, Theme.hue)
                            strokeColor: option.chosen ? Theme.accent : Theme.windowStroke
                            strokeWidth: option.chosen ? 2.5 : 1
                            elevation: option.chosen ? 3 : 1
                            Wallpaper {
                                anchors.fill: parent
                                anchors.margins: 6
                                darkness: option.modelData.dark ? 1 : 0
                                living: false
                                layer.enabled: true
                                layer.effect: RoundMask { radius: Tokens.radius.l - 4 }
                            }
                            Surface {
                                x: 18
                                y: 22
                                width: 110
                                height: 70
                                radius: 8
                                color: option.modelData.dark ? Oklch.color(0.24, 0.015, Theme.hue, 0.92) : Oklch.color(0.99, 0.003, Theme.hue, 0.92)
                                elevation: 2
                                Surface {
                                    x: 8
                                    y: 10
                                    width: 46
                                    height: 6
                                    radius: 3
                                    color: Theme.accent
                                }
                            }
                            TapHandler { onTapped: Services.settings.darkMode = option.modelData.dark }
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: option.modelData.label
                            font: option.chosen ? Theme.bodyStrong : Theme.body
                            color: Theme.textPrimary
                        }
                    }
                }
            }

            Group {
                SettingRow {
                    title: qsTr("Accent color")
                    subtitle: qsTr("Tints controls, selection and the Aurora wallpaper.")
                    Row {
                        spacing: 8
                        Repeater {
                            model: Tokens.accentPresets
                            delegate: Surface {
                                id: swatch
                                required property real modelData
                                readonly property bool chosen: Math.abs(Services.settings.accentHue - modelData) < 0.5
                                width: 22
                                height: 22
                                radius: 11
                                exponent: 2
                                color: Oklch.color(0.72, 0.16, modelData + 10)
                                gradientTo: Oklch.color(0.56, 0.18, modelData - 10)
                                strokeColor: chosen ? Theme.textPrimary : Qt.rgba(1, 1, 1, 0.25)
                                strokeWidth: chosen ? 2 : 1
                                TapHandler { onTapped: Services.settings.accentHue = swatch.modelData }
                            }
                        }
                    }
                }
                SettingRow {
                    title: qsTr("Custom hue")
                    subtitle: Math.round(Services.settings.accentHue) + "° OKLCH"
                    Slider {
                        width: 220
                        from: 0
                        to: 360
                        value: Services.settings.accentHue
                        onMoved: Services.settings.accentHue = value
                    }
                }
                SettingRow {
                    title: qsTr("Living wallpaper")
                    subtitle: qsTr("Aurora drifts slowly. Costs GPU time; off by default.")
                    Switch {
                        checked: Services.settings.livingWallpaper
                        onToggled: Services.settings.livingWallpaper = checked
                    }
                }
                SettingRow {
                    title: qsTr("Reduce transparency")
                    subtitle: qsTr("Replace glass and frost with solid surfaces.")
                    last: true
                    Switch {
                        checked: Services.settings.reduceTransparency
                        onToggled: Services.settings.reduceTransparency = checked
                    }
                }
            }
        }
    }

    Component {
        id: displayPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Display") }
            Group {
                SettingRow {
                    title: qsTr("Brightness")
                    subtitle: Math.round(Services.display.brightness * 100) + "%"
                    Slider {
                        width: 240
                        from: 0.2
                        to: 1
                        value: Services.display.brightness
                        onMoved: Services.display.brightness = value
                    }
                }
                SettingRow {
                    title: qsTr("Night Light")
                    subtitle: qsTr("Warmer colors after dark, applied by Prism's output pipeline.")
                    Switch {
                        checked: Services.display.nightLight
                        onToggled: Services.display.nightLight = checked
                    }
                }
                SettingRow {
                    title: qsTr("Color temperature")
                    subtitle: Services.display.nightLightTemperature + " K"
                    last: true
                    Slider {
                        width: 240
                        from: 2700
                        to: 5500
                        stepSize: 100
                        value: Services.display.nightLightTemperature
                        onMoved: Services.display.nightLightTemperature = value
                    }
                }
            }
        }
    }

    Component {
        id: soundPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Sound") }
            Group {
                SettingRow {
                    title: qsTr("Output volume")
                    subtitle: Services.audio.muted ? qsTr("Muted") : Math.round(Services.audio.volume * 100) + "%"
                    Slider {
                        width: 240
                        value: Services.audio.volume
                        onMoved: Services.audio.volume = value
                    }
                }
                SettingRow {
                    title: qsTr("Mute")
                    last: true
                    Switch {
                        checked: Services.audio.muted
                        onToggled: Services.audio.muted = checked
                    }
                }
            }
            SectionLabel { text: qsTr("Output device") }
            Group {
                Repeater {
                    model: Services.audio.outputDevices
                    delegate: SettingRow {
                        id: device
                        required property string modelData
                        required property int index
                        title: modelData
                        last: index === Services.audio.outputDevices.length - 1
                        RadioButton {
                            checked: Services.audio.outputDevice === device.modelData
                            onClicked: Services.audio.selectOutput(device.modelData)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: networkPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Wi-Fi") }
            Group {
                SettingRow {
                    title: qsTr("Wi-Fi")
                    subtitle: Services.network.connecting ? qsTr("Connecting…")
                              : Services.network.online ? qsTr("Connected to %1").arg(Services.network.activeSsid)
                              : qsTr("Not connected")
                    last: true
                    Switch {
                        checked: Services.network.wifiEnabled
                        onToggled: Services.network.wifiEnabled = checked
                    }
                }
            }
            SectionLabel { text: qsTr("Networks"); visible: Services.network.wifiEnabled }
            Group {
                visible: Services.network.wifiEnabled
                Repeater {
                    model: Services.network.accessPoints
                    delegate: SettingRow {
                        id: ap
                        required property var modelData
                        required property int index
                        readonly property bool active: modelData.ssid === Services.network.activeSsid
                        title: modelData.ssid
                        subtitle: (modelData.secure ? qsTr("Secured") : qsTr("Open")) + (active ? qsTr(" · Connected") : "")
                        icon: ["wifi-zero", "wifi-low", "wifi-high", "wifi"][Math.min(3, modelData.bars)]
                        last: index === Services.network.accessPoints.length - 1
                        Button {
                            text: ap.active ? qsTr("Disconnect") : qsTr("Connect")
                            highlighted: !ap.active
                            onClicked: ap.active ? Services.network.disconnectNetwork()
                                                 : Services.network.connectTo(ap.modelData.ssid)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: bluetoothPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Bluetooth") }
            Group {
                SettingRow {
                    title: qsTr("Bluetooth")
                    subtitle: Services.bluetooth.enabled ? qsTr("Discoverable as “%1”").arg(Services.settings.hostName) : qsTr("Off")
                    last: true
                    Switch {
                        checked: Services.bluetooth.enabled
                        onToggled: Services.bluetooth.enabled = checked
                    }
                }
            }
            SectionLabel { text: qsTr("My devices") }
            Group {
                Repeater {
                    model: Services.bluetooth.devices
                    delegate: SettingRow {
                        id: dev
                        required property var modelData
                        required property int index
                        title: modelData.name
                        subtitle: (modelData.connected ? qsTr("Connected") : qsTr("Not connected"))
                                  + (modelData.battery >= 0 ? " · " + modelData.battery + "%" : "")
                        icon: ({ headphones: "headphones", keyboard: "keyboard", mouse: "app-window", speaker: "speaker" })[modelData.kind] ?? "bluetooth"
                        last: index === Services.bluetooth.devices.length - 1
                        Button {
                            text: dev.modelData.connected ? qsTr("Disconnect") : qsTr("Connect")
                            enabled: Services.bluetooth.enabled
                            onClicked: Services.bluetooth.toggleDevice(dev.modelData.name)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: notificationsPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Notifications") }
            Group {
                SettingRow {
                    title: qsTr("Focus")
                    subtitle: qsTr("Silence banners. Notifications still arrive in history.")
                    Switch {
                        checked: Services.notifications.doNotDisturb
                        onToggled: Services.notifications.doNotDisturb = checked
                    }
                }
                SettingRow {
                    title: qsTr("Test notification")
                    subtitle: qsTr("Delivered by qtos-notifyd over Qt Remote Objects.")
                    last: true
                    Button {
                        text: qsTr("Send")
                        icon.name: "bell-ring"
                        onClicked: Services.notifications.notify("system", qsTr("It works"),
                                                                 qsTr("Sent from Settings at %1").arg(new Date().toLocaleTimeString(Qt.locale(), Locale.ShortFormat)),
                                                                 "bell-ring")
                    }
                }
            }
        }
    }

    Component {
        id: accessibilityPage
        Column {
            spacing: 18
            PageTitle { text: qsTr("Accessibility") }
            Group {
                SettingRow {
                    title: qsTr("Reduce motion")
                    subtitle: qsTr("Springs settle instantly; parallax and zoom are removed.")
                    Switch {
                        checked: Services.settings.reduceMotion
                        onToggled: Services.settings.reduceMotion = checked
                    }
                }
                SettingRow {
                    title: qsTr("Reduce transparency")
                    subtitle: qsTr("Glass and frost become solid for maximum legibility.")
                    last: true
                    Switch {
                        checked: Services.settings.reduceTransparency
                        onToggled: Services.settings.reduceTransparency = checked
                    }
                }
            }
        }
    }

    Component {
        id: aboutPage
        Column {
            spacing: 18
            Row {
                spacing: 18
                QtosMark { size: 72 }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text { text: "QTOS"; font: Theme.display; color: Theme.textPrimary }
                    Text { text: qsTr("Version %1 · Prism shell in simulation").arg(Kit.version); font: Theme.callout; color: Theme.textSecondary }
                }
            }
            Group {
                SettingRow { title: qsTr("Design language"); subtitle: "Lumen " + Tokens.version }
                SettingRow { title: qsTr("UI font"); subtitle: Kit.uiFontFamily + " · " + Kit.monoFontFamily }
                SettingRow {
                    title: qsTr("Graphics")
                    subtitle: {
                        switch (GraphicsInfo.api) {
                        case GraphicsInfo.Direct3D11: return "Direct3D 11 (QRhi)";
                        case GraphicsInfo.Direct3D12: return "Direct3D 12 (QRhi)";
                        case GraphicsInfo.Vulkan: return "Vulkan (QRhi)";
                        case GraphicsInfo.Metal: return "Metal (QRhi)";
                        case GraphicsInfo.OpenGL: return "OpenGL (QRhi)";
                        case GraphicsInfo.Software: return qsTr("Software");
                        }
                        return qsTr("Unknown");
                    }
                }
                SettingRow { title: qsTr("System services"); subtitle: Services.url + " · Qt Remote Objects"; last: true }
            }
            SectionLabel { text: qsTr("Built with Qt") }
            Flow {
                width: parent.width
                spacing: 8
                Repeater {
                    model: ["Qt Quick", "Qt Quick Controls", "Qt Quick Effects", "Qt Quick Shapes", "Qt Shader Tools",
                            "Qt Remote Objects", "Qt Graphs", "Qt Svg", "Qt Qml Core", "Qt Wayland Compositor",
                            "Qt Multimedia", "Qt Quick 3D", "Qt Spatial Audio", "Qt Virtual Keyboard", "Qt WebEngine",
                            "Qt Positioning", "Qt TextToSpeech", "Qt gRPC"]
                    delegate: Surface {
                        id: chip
                        required property string modelData
                        width: chipText.implicitWidth + 22
                        height: 26
                        radius: 13
                        exponent: 2
                        color: Theme.fillControl
                        strokeColor: Theme.windowStroke
                        Text {
                            id: chipText
                            anchors.centerIn: parent
                            text: chip.modelData
                            font: Theme.footnote
                            color: Theme.textSecondary
                        }
                    }
                }
            }
        }
    }

    // ── Building blocks ─────────────────────────────────────────────────────
    component PageTitle: Text {
        font: Theme.title1
        color: Theme.textPrimary
        topPadding: 10
    }

    component SectionLabel: Text {
        font: Theme.footnote
        color: Theme.textTertiary
        leftPadding: 4
    }

    component Group: Card {
        width: parent ? parent.width : 400
        padding: 0
        spacing: 0
    }

    component SettingRow: Item {
        id: row
        default property alias control: slot.data
        property string title
        property string subtitle
        property string icon
        property bool last: false
        width: parent ? parent.width : 400
        height: Math.max(54, labels.implicitHeight + 22)

        Icon {
            id: rowIcon
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            visible: row.icon.length > 0
            name: row.icon
            size: 18
            color: Theme.textSecondary
        }
        Column {
            id: labels
            anchors.left: row.icon.length > 0 ? rowIcon.right : parent.left
            anchors.leftMargin: row.icon.length > 0 ? 12 : 16
            anchors.right: slot.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                width: parent.width
                text: row.title
                font: Theme.body
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                visible: row.subtitle.length > 0
                text: row.subtitle
                font: Theme.footnote
                color: Theme.textTertiary
                wrapMode: Text.Wrap
            }
        }
        Item {
            id: slot
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            width: childrenRect.width
            height: childrenRect.height
        }
        Divider {
            visible: !row.last
            anchors.bottom: parent.bottom
            x: 16
            width: parent.width - 16
        }
    }
}
