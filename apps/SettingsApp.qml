// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QTOS.Kit
import QTOS.Services

// Settings: every control is a live binding to a QTOS service replica, so
// changes apply system-wide instantly (and persist via qtos-settingsd).
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    property string page: "appearance"
    // A section of the page to scroll to (the objectName of its label).
    property string section: ""

    onLaunchArgsChanged: {
        if (!launchArgs || !launchArgs.page)
            return;
        page = launchArgs.page;
        section = launchArgs.section ?? "";
        revealTimer.restart();
    }

    function findNamed(item: Item, name: string): Item {
        if (!item)
            return null;
        if (item.objectName === name)
            return item;
        for (let i = 0; i < item.children.length; ++i) {
            const found = findNamed(item.children[i], name);
            if (found)
                return found;
        }
        return null;
    }

    function revealSection() {
        const target = section.length > 0 ? findNamed(pageLoader.item, section) : null;
        if (!target)
            return;
        const y = target.mapToItem(pageLoader, 0, 0).y + pageLoader.y - 12;
        scroller.contentY = Math.max(0, Math.min(y, scroller.contentHeight - scroller.height));
        section = "";
    }

    // Positioners lay the page out after it loads.
    Timer {
        id: revealTimer
        interval: 80
        onTriggered: app.revealSection()
    }

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
                revealTimer.restart();
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
            id: sound
            spacing: 18

            // The reactive range slider is logarithmic, 20 Hz .. 20 kHz.
            function toPosition(hz: real): real { return Math.log(hz / 20) / Math.log(1000); }
            function toHz(position: real): real { return 20 * Math.pow(1000, position); }
            function formatHz(hz: real): string {
                return hz >= 1000 ? (Math.round(hz / 100) / 10) + " kHz" : Math.round(hz) + " Hz";
            }
            readonly property var rangePresets: [
                { label: qsTr("Bass"), low: 40, high: 160 },
                { label: qsTr("Vocals"), low: 300, high: 3000 },
                { label: qsTr("Highs"), low: 4000, high: 16000 },
                { label: qsTr("Full range"), low: 20, high: 20000 }
            ]

            PageTitle { text: qsTr("Sound") }

            Group {
                SettingRow {
                    title: qsTr("Output volume")
                    subtitle: Services.audio.muted ? qsTr("Muted") : Math.round(Services.audio.volume * 100) + "%"
                    Slider {
                        width: 240
                        value: Services.audio.volume
                        onMoved: Services.audio.volume = value
                        onPressedChanged: if (!pressed) Sounds.play("volume")
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
                        subtitle: Services.audio.outputDevice === modelData && !Services.audio.outputAvailable
                                  ? qsTr("Simulated · sound is rendered silently") : ""
                        last: index === Services.audio.outputDevices.length - 1
                        RadioButton {
                            checked: Services.audio.outputDevice === device.modelData
                            onClicked: Services.audio.selectOutput(device.modelData)
                        }
                    }
                }
            }

            // ── Equalizer ───────────────────────────────────────────────────
            SectionLabel { objectName: "equalizer"; text: qsTr("Equalizer") }
            Card {
                id: eqCard
                width: parent.width
                padding: 16
                spacing: 14

                Flow {
                    width: eqCard.width - 32
                    spacing: 6
                    Repeater {
                        model: Services.audio.equalizerPresets
                        delegate: Chip {
                            required property string modelData
                            required property int index
                            objectName: "eqPreset" + index
                            text: modelData
                            selected: Services.audio.equalizerPreset === modelData
                            onClicked: Services.audio.applyEqualizerPreset(modelData)
                        }
                    }
                    Chip {
                        visible: Services.audio.equalizerPreset === qsTr("Custom")
                        text: qsTr("Custom")
                        selected: true
                    }
                }

                Item {
                    id: eqBands
                    readonly property list<string> labels: ["60 Hz", "250 Hz", "1 kHz", "4 kHz", "12 kHz"]
                    readonly property real columnWidth: width / labels.length
                    readonly property real trackTop: 22
                    readonly property real trackHeight: 132
                    width: eqCard.width - 32
                    height: trackTop + trackHeight + 26

                    function gain(band: int): real { return Number(Services.audio.equalizer[band] ?? 0); }
                    function yFor(gainDb: real): real {
                        // Matches the vertical Slider's handle centre.
                        return trackTop + 4 + 9 + (1 - (gainDb + 12) / 24) * (trackHeight - 8 - 18);
                    }

                    // Live spectrum behind the bands.
                    Spectrum {
                        x: 0
                        y: eqBands.trackTop
                        width: parent.width
                        height: eqBands.trackHeight
                        bars: 48
                        values: Reactive.spectrum
                        minimum: 0
                        color: Oklch.alpha(Theme.accent, 0.16)
                        dimColor: color
                    }

                    // Zero line and the response curve through the handles.
                    Rectangle {
                        y: eqBands.yFor(0)
                        width: parent.width
                        height: 1
                        color: Theme.separator
                    }
                    // The response: a soft fill under a curve through the handles.
                    Shape {
                        anchors.fill: parent
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: "transparent"
                            fillGradient: LinearGradient {
                                y1: eqBands.trackTop
                                y2: eqBands.trackTop + eqBands.trackHeight
                                GradientStop { position: 0; color: Oklch.alpha(Theme.accent, 0.26) }
                                GradientStop { position: 1; color: Oklch.alpha(Theme.accent, 0.0) }
                            }
                            startX: 0
                            startY: eqBands.yFor(eqBands.gain(0))
                            PathCurve { x: eqBands.columnWidth * 0.5; y: eqBands.yFor(eqBands.gain(0)) }
                            PathCurve { x: eqBands.columnWidth * 1.5; y: eqBands.yFor(eqBands.gain(1)) }
                            PathCurve { x: eqBands.columnWidth * 2.5; y: eqBands.yFor(eqBands.gain(2)) }
                            PathCurve { x: eqBands.columnWidth * 3.5; y: eqBands.yFor(eqBands.gain(3)) }
                            PathCurve { x: eqBands.columnWidth * 4.5; y: eqBands.yFor(eqBands.gain(4)) }
                            PathCurve { x: eqBands.width; y: eqBands.yFor(eqBands.gain(4)) }
                            PathLine { x: eqBands.width; y: eqBands.trackTop + eqBands.trackHeight }
                            PathLine { x: 0; y: eqBands.trackTop + eqBands.trackHeight }
                            PathLine { x: 0; y: eqBands.yFor(eqBands.gain(0)) }
                        }
                        ShapePath {
                            strokeColor: Oklch.alpha(Theme.accent, 0.9)
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            startX: 0
                            startY: eqBands.yFor(eqBands.gain(0))
                            PathCurve { x: eqBands.columnWidth * 0.5; y: eqBands.yFor(eqBands.gain(0)) }
                            PathCurve { x: eqBands.columnWidth * 1.5; y: eqBands.yFor(eqBands.gain(1)) }
                            PathCurve { x: eqBands.columnWidth * 2.5; y: eqBands.yFor(eqBands.gain(2)) }
                            PathCurve { x: eqBands.columnWidth * 3.5; y: eqBands.yFor(eqBands.gain(3)) }
                            PathCurve { x: eqBands.columnWidth * 4.5; y: eqBands.yFor(eqBands.gain(4)) }
                            PathCurve { x: eqBands.width; y: eqBands.yFor(eqBands.gain(4)) }
                        }
                    }

                    Repeater {
                        model: eqBands.labels
                        delegate: Item {
                            id: band
                            required property string modelData
                            required property int index
                            x: index * eqBands.columnWidth
                            width: eqBands.columnWidth
                            height: eqBands.height

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: {
                                    const g = eqBands.gain(band.index);
                                    return (g > 0 ? "+" : "") + g.toFixed(1) + " dB";
                                }
                                font: Theme.caption
                                color: Math.abs(eqBands.gain(band.index)) > 0.01 ? Theme.textSecondary : Theme.textTertiary
                            }
                            Slider {
                                id: bandSlider
                                objectName: "eqBand" + band.index
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: eqBands.trackTop
                                height: eqBands.trackHeight
                                orientation: Qt.Vertical
                                from: -12
                                to: 12
                                stepSize: 0.5
                                value: eqBands.gain(band.index)
                                onMoved: Services.audio.setEqualizerBand(band.index, value)
                                // Boost and cut are both departures from 0 dB:
                                // a plain track, the curve shows the gain.
                                background: Rectangle {
                                    x: (bandSlider.width - width) / 2
                                    y: bandSlider.topPadding
                                    implicitWidth: 2
                                    implicitHeight: 120
                                    width: 2
                                    height: bandSlider.availableHeight
                                    radius: 1
                                    color: Theme.fillControlPressed
                                }
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: eqBands.trackTop + eqBands.trackHeight + 8
                                text: band.modelData
                                font: Theme.caption
                                color: Theme.textTertiary
                            }
                        }
                    }
                }
            }

            // ── Interface sounds ────────────────────────────────────────────
            SectionLabel { objectName: "sounds"; text: qsTr("Interface sounds") }
            Group {
                SettingRow {
                    title: qsTr("Play interface sounds")
                    subtitle: qsTr("Notifications, locking and unlocking, volume changes.")
                    Switch {
                        checked: Services.audio.systemSounds
                        onToggled: Services.audio.systemSounds = checked
                    }
                }
                SettingRow {
                    title: qsTr("Sound volume")
                    subtitle: qsTr("Relative to the output volume.")
                    last: true
                    Slider {
                        width: 240
                        enabled: Services.audio.systemSounds
                        value: Services.audio.systemVolume
                        onMoved: Services.audio.systemVolume = value
                        onPressedChanged: if (!pressed) Sounds.play("notification")
                    }
                }
            }

            // ── Audio-reactive interface ────────────────────────────────────
            SectionLabel { objectName: "reactive"; text: qsTr("Audio-reactive interface") }
            Group {
                SettingRow {
                    title: qsTr("React to sound")
                    subtitle: Services.settings.reduceMotion
                              ? qsTr("Paused while Reduce Motion is on.")
                              : qsTr("The wallpaper, Now Playing and the Rail move with what's playing.")
                    Switch {
                        objectName: "audioReactiveSwitch"
                        checked: Services.settings.audioReactive
                        onToggled: Services.settings.audioReactive = checked
                    }
                }
                SettingRow {
                    title: qsTr("Intensity")
                    subtitle: {
                        const v = Services.settings.reactiveIntensity;
                        const name = v < 0.34 ? qsTr("Subtle") : v < 0.67 ? qsTr("Balanced") : qsTr("Vivid");
                        return name + " · " + Math.round(v * 100) + "%";
                    }
                    Slider {
                        objectName: "reactiveIntensitySlider"
                        width: 240
                        enabled: Services.settings.audioReactive
                        value: Services.settings.reactiveIntensity
                        onMoved: Services.settings.reactiveIntensity = value
                    }
                }
                SettingRow {
                    title: qsTr("Reacts to")
                    subtitle: sound.formatHz(Services.settings.reactiveLowHz) + " – "
                              + sound.formatHz(Services.settings.reactiveHighHz)
                    Row {
                        spacing: 6
                        Repeater {
                            model: sound.rangePresets
                            delegate: Chip {
                                required property var modelData
                                required property int index
                                objectName: "reactivePreset" + index
                                text: modelData.label
                                enabled: Services.settings.audioReactive
                                selected: Math.abs(Services.settings.reactiveLowHz - modelData.low) < 1
                                          && Math.abs(Services.settings.reactiveHighHz - modelData.high) < 1
                                onClicked: {
                                    Services.settings.reactiveLowHz = modelData.low;
                                    Services.settings.reactiveHighHz = modelData.high;
                                }
                            }
                        }
                    }
                }

                // The live spectrum with the chosen range lit, and the range
                // slider directly beneath it on the same logarithmic axis.
                Item {
                    id: rangePanel
                    width: parent.width
                    height: 150
                    opacity: Services.settings.audioReactive ? 1 : 0.45
                    Behavior on opacity { NumberAnimation { duration: 160 } }

                    readonly property real trackX: range.x + range.leftPadding + range.handleSize / 2
                    readonly property real trackWidth: range.availableWidth - range.handleSize
                    // Spectrum bands cover 30 Hz .. 16 kHz.
                    function bandFraction(hz: real): real { return Math.log(hz / 30) / Math.log(16000 / 30); }

                    Surface {
                        x: 16
                        y: 4
                        width: parent.width - 32
                        height: 88
                        radius: Tokens.radius.m
                        color: Oklch.alpha(Theme.textPrimary, 0.035)
                    }
                    Spectrum {
                        objectName: "reactiveSpectrum"
                        x: rangePanel.trackX + sound.toPosition(30) * rangePanel.trackWidth
                        y: 12
                        width: (sound.toPosition(16000) - sound.toPosition(30)) * rangePanel.trackWidth
                        height: 72
                        bars: 48
                        spacing: 2
                        values: Reactive.spectrum
                        color: Theme.accent
                        dimColor: Oklch.alpha(Theme.textPrimary, 0.16)
                        highlightFrom: rangePanel.bandFraction(Services.settings.reactiveLowHz)
                        highlightTo: rangePanel.bandFraction(Services.settings.reactiveHighHz)
                    }
                    Text {
                        x: 28
                        y: 14
                        visible: !Reactive.playing
                        text: qsTr("Play something to see the spectrum")
                        font: Theme.caption
                        color: Theme.textTertiary
                    }

                    RangeSlider {
                        id: range
                        objectName: "reactiveRange"
                        x: 16
                        y: 96
                        width: parent.width - 32
                        from: 0
                        to: 1
                        enabled: Services.settings.audioReactive
                        first.value: sound.toPosition(Services.settings.reactiveLowHz)
                        second.value: sound.toPosition(Services.settings.reactiveHighHz)
                        first.onMoved: Services.settings.reactiveLowHz = Math.round(sound.toHz(first.value))
                        second.onMoved: Services.settings.reactiveHighHz = Math.round(sound.toHz(second.value))
                    }
                    Repeater {
                        model: [20, 100, 1000, 10000, 20000]
                        delegate: Text {
                            required property real modelData
                            x: rangePanel.trackX + sound.toPosition(modelData) * rangePanel.trackWidth - width / 2
                            y: 124
                            text: modelData >= 1000 ? (modelData / 1000) + "k" : String(modelData)
                            font: Theme.caption
                            color: Theme.textTertiary
                        }
                    }
                }

                // How strongly the interface is reacting right now.
                SettingRow {
                    title: qsTr("Response")
                    subtitle: Reactive.enabled ? qsTr("Live energy in the chosen range; the dot flashes on beats.")
                                               : qsTr("Off")
                    last: true
                    Row {
                        spacing: 10
                        Surface {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 160
                            height: 6
                            radius: 3
                            exponent: 2
                            color: Theme.fillControlPressed
                            Surface {
                                objectName: "reactiveMeter"
                                width: Math.max(6, parent.width * Reactive.energy)
                                height: parent.height
                                radius: 3
                                exponent: 2
                                color: Theme.accent
                                opacity: Reactive.energy > 0.001 ? 1 : 0
                            }
                        }
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 12
                            height: 12
                            radius: 6
                            color: Theme.accent
                            opacity: 0.2 + 0.8 * Reactive.pulse
                            scale: 0.8 + 0.5 * Reactive.pulse
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
                        objectName: "reduceMotionSwitch"
                        checked: Services.settings.reduceMotion
                        onToggled: Services.settings.reduceMotion = checked
                    }
                }
                SettingRow {
                    title: qsTr("Reduce transparency")
                    subtitle: qsTr("Glass and frost become solid for maximum legibility.")
                    Switch {
                        checked: Services.settings.reduceTransparency
                        onToggled: Services.settings.reduceTransparency = checked
                    }
                }
                SettingRow {
                    title: qsTr("Increase contrast")
                    subtitle: qsTr("Stronger text, outlines and separators; glass becomes solid.")
                    last: true
                    Switch {
                        checked: Services.settings.increaseContrast
                        onToggled: Services.settings.increaseContrast = checked
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

    component Chip: Surface {
        id: chip
        property string text
        property bool selected: false
        signal clicked()

        width: chipLabel.implicitWidth + 24
        height: 28
        radius: 14
        exponent: 2
        color: selected ? Theme.accent : chipHover.hovered ? Theme.fillControlHover : Theme.fillControl
        strokeColor: selected ? "transparent" : Theme.windowStroke
        opacity: enabled ? 1 : 0.5
        Behavior on color { ColorAnimation { duration: 140 } }
        Accessible.role: Accessible.Button
        Accessible.name: text

        Text {
            id: chipLabel
            anchors.centerIn: parent
            text: chip.text
            font: Theme.footnote
            color: chip.selected ? Theme.onAccent : Theme.textPrimary
        }
        HoverHandler { id: chipHover; enabled: chip.enabled }
        TapHandler { enabled: chip.enabled; onTapped: chip.clicked() }
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
