// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services
import QTOS.Apps

// The Prism desktop for one output.
//
// Layering (bottom → top):
//   content   wallpaper, focus-pull blur, windows       ← captured by sceneBackdrop
//   chrome    Rail, Shelf, notifications, Control Center, Beam
//   lock      lock screen
//   display   brightness / Night Light simulation
//
// Glass in the chrome samples `sceneBackdrop` (blurred content); Frost inside
// windows samples `wallpaperBackdrop` (blurred wallpaper only).
FocusScope {
    id: desktop

    property var options: ({})

    // ── Session state ───────────────────────────────────────────────────────
    property bool locked: true
    property string surface: ""        // "", "beam", "control-center", "overview", "media"
    property int currentSpace: 0
    readonly property int spaceCount: 3
    readonly property bool overview: surface === "overview"
    property bool startupAppsOpened: false

    readonly property real railHeight: 34
    readonly property rect workArea: Qt.rect(10, railHeight + 18, width - 20, height - railHeight - 18 - 96)

    // ── Appearance follows qtos-settingsd ───────────────────────────────────
    Binding { target: Theme; property: "dark"; value: Services.settings.darkMode }
    Binding { target: Theme; property: "accentHue"; value: Services.settings.accentHue }
    Binding { target: Theme; property: "reduceMotion"; value: Services.settings.reduceMotion }
    Binding { target: Theme; property: "reduceTransparency"; value: Services.settings.reduceTransparency }
    Binding { target: Theme; property: "increaseContrast"; value: Services.settings.increaseContrast }
    Binding { target: Theme; property: "livingWallpaper"; value: Services.settings.livingWallpaper }

    // ── Interface sounds follow qtos-audiod ─────────────────────────────────
    Binding {
        target: Sounds
        property: "enabled"
        value: Services.connected && Services.audio.systemSounds && Services.audio.outputAvailable
    }
    Binding {
        target: Sounds
        property: "volume"
        // Relative to the output volume, which has a perceptual (cubic) taper.
        value: Services.audio.muted ? 0 : Math.pow(Services.audio.volume, 3) * Services.audio.systemVolume
    }
    Binding { target: Sounds; property: "device"; value: Services.audio.outputDevice }
    Connections {
        target: Services.notifications
        function onPosted() {
            if (!Services.notifications.doNotDisturb)
                Sounds.play("notification");
        }
    }
    Connections {
        target: Services.power
        function onChargingChanged() {
            if (Services.power.charging)
                Sounds.play("connect");
        }
    }

    // ── Shell API ───────────────────────────────────────────────────────────
    function setSurface(name: string) {
        if (locked && name !== "")
            return;
        surface = name;
        if (name === "")
            windows.focusActive();
    }

    function toggleSurface(name: string) {
        setSurface(surface === name ? "" : name);
    }

    function launch(appId: string, args: var) {
        if (locked)
            return;
        if (surface !== "overview")
            surface = "";
        const window = windows.open(appId, shelf.originFor(appId), args === undefined ? ({}) : args);
        if (window && window.space !== currentSpace)
            currentSpace = window.space;
        if (surface === "overview")
            surface = "";
    }

    function lock() {
        surface = "";
        if (!locked)
            Sounds.play("lock");
        locked = true;
    }

    function unlock() {
        if (locked)
            Sounds.play("unlock");
        locked = false;
        desktop.forceActiveFocus();
        if (!startupAppsOpened) {
            startupAppsOpened = true;
            // "settings/sound" opens an app at a page, "settings/sound/reactive"
            // at a section of it.
            const apps = options.apps ?? [];
            for (let i = 0; i < apps.length; ++i) {
                const parts = apps[i].split("/");
                const args = {};
                if (parts.length > 1)
                    args.page = parts[1];
                if (parts.length > 2)
                    args.section = parts[2];
                startupTimer.createObject(desktop, { appId: parts[0], args: args, interval: 260 + i * 140 });
            }
        }
    }

    // ── Compositor integration (Prism on device) ───────────────────────────
    readonly property alias windowLayer: windows

    function adoptSurface(shellSurface: var, toplevel: var): AppWindow {
        return windows.adopt(shellSurface, toplevel);
    }

    // Volume keys: sixteen steps, unmuting on the way up.
    function changeVolume(steps: int) {
        const audio = Services.audio;
        if (steps > 0 && audio.muted)
            audio.muted = false;
        audio.volume = Math.max(0, Math.min(1, Math.round(audio.volume * 16 + steps) / 16));
        Sounds.play("volume");
        volumeHud.show();
    }

    function toggleMute() {
        Services.audio.muted = !Services.audio.muted;
        volumeHud.show();
    }

    function switchSpace(index: int) {
        currentSpace = Math.max(0, Math.min(spaceCount - 1, index));
    }

    function runAction(id: string, args: var) {
        const settings = Services.settings;
        switch (id) {
        case "appearance.toggle": settings.darkMode = !settings.darkMode; break;
        case "session.lock": lock(); break;
        case "shell.overview": setSurface("overview"); break;
        case "shell.control-center": setSurface("control-center"); break;
        case "media.toggle": Services.media.togglePlayback(); break;
        case "media.next": Services.media.next(); break;
        case "wifi.toggle": Services.network.wifiEnabled = !Services.network.wifiEnabled; break;
        case "bluetooth.toggle": Services.bluetooth.enabled = !Services.bluetooth.enabled; break;
        case "dnd.toggle": Services.notifications.doNotDisturb = !Services.notifications.doNotDisturb; break;
        case "motion.toggle": settings.reduceMotion = !settings.reduceMotion; break;
        case "transparency.toggle": settings.reduceTransparency = !settings.reduceTransparency; break;
        case "contrast.toggle": settings.increaseContrast = !settings.increaseContrast; break;
        case "wallpaper.living": settings.livingWallpaper = !settings.livingWallpaper; break;
        case "reactive.toggle": settings.audioReactive = !settings.audioReactive; break;
        case "notify.test":
            Services.notifications.notify("system", qsTr("Hello from Beam"),
                                          qsTr("Notifications travel over Qt Remote Objects from qtos-notifyd."),
                                          "bell-ring");
            break;
        case "space.go": switchSpace(args.index); break;
        case "accent.set": settings.accentHue = args.hue; break;
        case "app.open": launch(args.id, args); break;
        default: console.warn("Unknown shell action", id);
        }
    }

    Connections {
        target: Intents
        function onRequested(action: string, args: var) {
            switch (action) {
            case "app.open": desktop.launch(args.id, args); break;
            case "session.lock": desktop.lock(); break;
            case "shell.overview": desktop.setSurface("overview"); break;
            case "shell.beam": desktop.setSurface("beam"); break;
            case "shell.space": desktop.switchSpace(args.index); break;
            default: console.warn("Unhandled intent", action);
            }
        }
    }

    Component {
        id: startupTimer
        Timer {
            property string appId
            property var args: ({})
            running: true
            onTriggered: {
                desktop.launch(appId, args);
                destroy();
            }
        }
    }

    Component.onCompleted: {
        Materials.desktop = wallpaperBackdrop;
        Materials.scene = sceneBackdrop;
        if (options.theme === "light")
            Services.settings.darkMode = false;
        else if (options.theme === "dark")
            Services.settings.darkMode = true;
        if (options.accent !== undefined && options.accent !== null)
            Services.settings.accentHue = options.accent;
        if (options.contrast === "high")
            Services.settings.increaseContrast = true;
        else if (options.contrast === "standard")
            Services.settings.increaseContrast = false;
        if (options.unlocked)
            unlock();
        if (options.surface && options.surface.length > 0)
            surfaceTimer.start();
        forceActiveFocus();
    }

    Timer {
        id: surfaceTimer
        interval: 900
        onTriggered: {
            desktop.setSurface(desktop.options.surface);
            if (desktop.options.surface === "beam" && desktop.options.query)
                beam.query = desktop.options.query;
        }
    }

    // ── Motion drivers ──────────────────────────────────────────────────────
    Spring { id: lockDepth; target: desktop.locked ? 1 : 0; preset: Motion.gentle }
    Spring { id: overviewDepth; target: desktop.overview ? 1 : 0; preset: Motion.smooth }
    Spring { id: spaceTravel; target: desktop.currentSpace; preset: Motion.smooth }

    // ── Content (captured by sceneBackdrop) ─────────────────────────────────
    Item {
        id: content
        anchors.fill: parent

        Wallpaper {
            id: wallpaper
            anchors.fill: parent
            parallax: Qt.vector2d(spaceTravel.value * 0.03, 0)
            // The audio-reactive interface: light that swells with the music
            // and a gentle push on every beat.
            energy: Reactive.energy
            pulse: Reactive.pulse
            zoom: 1 + 0.06 * lockDepth.value + 0.03 * overviewDepth.value + 0.012 * Reactive.pulse
        }

        // Focus pull: crossfade to the pre-blurred wallpaper.
        ShaderEffect {
            anchors.fill: parent
            property variant source: wallpaperBackdrop.texture
            opacity: Math.max(lockDepth.value, overviewDepth.value * 0.9)
            visible: opacity > 0.002
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.16 * lockDepth.value + 0.10 * overviewDepth.value
            visible: opacity > 0.002
        }

        WindowLayer {
            id: windows
            anchors.fill: parent
            workArea: desktop.workArea
            currentSpace: desktop.currentSpace
            spaceOffset: spaceTravel.value
            overview: desktop.overview
            opacity: 1 - lockDepth.value
            scale: 1 - 0.035 * lockDepth.value
            visible: opacity > 0.002
            onOverviewPicked: desktop.setSurface("")
            originProvider: appId => shelf.originFor(appId)
        }
    }

    Backdrop { id: wallpaperBackdrop; sourceItem: wallpaper; live: true } // also feeds the focus pull
    Backdrop { id: sceneBackdrop; sourceItem: content }

    // ── Chrome ──────────────────────────────────────────────────────────────
    Item {
        id: chrome
        anchors.fill: parent
        opacity: 1 - lockDepth.value
        visible: opacity > 0.002

        SpaceSwitcher {
            anchors.horizontalCenter: parent.horizontalCenter
            y: desktop.workArea.y + 6
            progress: overviewDepth.value
            current: desktop.currentSpace
            count: desktop.spaceCount
            backdrop: sceneBackdrop
            onSelected: index => desktop.switchSpace(index)
        }

        Rail {
            id: rail
            x: 8
            y: 6 - 52 * lockDepth.value
            width: parent.width - 16
            height: desktop.railHeight
            backdrop: sceneBackdrop
            activeTitle: windows.activeTitle
            currentSpace: desktop.currentSpace
            spaceCount: desktop.spaceCount
            surface: desktop.surface
            onSurfaceRequested: name => desktop.toggleSurface(name)
        }

        Shelf {
            id: shelf
            objectName: "shelf"
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - height - 12 + 110 * lockDepth.value
            backdrop: sceneBackdrop
            runningApps: windows.runningApps
            attentionApps: windows.attentionApps
            identityProvider: appId => windows.clientIdentity(appId)
            overview: desktop.overview
            onLaunchRequested: appId => desktop.launch(appId, {})
            onBeamRequested: desktop.toggleSurface("beam")
            onOverviewRequested: desktop.toggleSurface("overview")
        }

        NotificationStack {
            id: notifications
            anchors.right: parent.right
            anchors.rightMargin: 12
            y: desktop.railHeight + 18
            width: 356
            height: parent.height - y - 100
            backdrop: sceneBackdrop
            suppressed: desktop.locked || desktop.surface === "control-center"
            onActivated: appId => {
                if (AppRegistry.app(appId))
                    desktop.launch(appId, {});
            }
        }

        // Click-away dismissal for popovers.
        Item {
            anchors.fill: parent
            visible: desktop.surface === "media" || desktop.surface === "control-center"
            TapHandler { onTapped: desktop.setSurface("") }
        }

        MediaPopover {
            anchors.horizontalCenter: parent.horizontalCenter
            y: desktop.railHeight + 14
            open: desktop.surface === "media"
            backdrop: sceneBackdrop
            onDismissed: desktop.setSurface("")
        }

        ControlCenter {
            anchors.right: parent.right
            anchors.rightMargin: 12
            y: desktop.railHeight + 14
            open: desktop.surface === "control-center"
            backdrop: sceneBackdrop
            onDismissed: desktop.setSurface("")
            onActionRequested: (id, args) => desktop.runAction(id, args)
        }

        Beam {
            id: beam
            anchors.fill: parent
            open: desktop.surface === "beam"
            backdrop: sceneBackdrop
            onDismissed: desktop.setSurface("")
            onActionRequested: (id, args) => {
                desktop.setSurface("");
                desktop.runAction(id, args);
            }
        }
    }

    LockScreen {
        anchors.fill: parent
        progress: lockDepth.value
        locked: desktop.locked
        backdrop: sceneBackdrop
        history: notifications.history
        onUnlockRequested: desktop.unlock()
    }

    // Above the lock screen: the volume keys work there too.
    VolumeHud {
        id: volumeHud
        objectName: "volumeHud"
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - height - 112
        backdrop: sceneBackdrop
    }

    // ── Display simulation (applied by Prism's output pipeline on device) ───
    Rectangle {
        anchors.fill: parent
        color: "#ff8c3a"
        opacity: Services.display.nightLight ? 0.11 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 600; easing.type: Easing.InOutCubic } }
    }
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: (1 - Services.display.brightness) * 0.72
        visible: opacity > 0.002
    }

    // ── Keyboard ────────────────────────────────────────────────────────────
    // Media and volume keys work everywhere, the lock screen included.
    // On Linux a keyboard's play/pause key arrives as Media Play
    // (XF86AudioPlay), so both toggle, as media players there do.
    Shortcut { sequences: ["Toggle Media Play/Pause", "Media Play"]; onActivated: Services.media.togglePlayback() }
    Shortcut { sequences: ["Media Pause", "Media Stop"]; onActivated: Services.media.playing = false }
    Shortcut { sequence: "Media Next"; onActivated: Services.media.next() }
    Shortcut { sequence: "Media Previous"; onActivated: Services.media.previous() }
    Shortcut { sequence: "Volume Up"; onActivated: desktop.changeVolume(1) }
    Shortcut { sequence: "Volume Down"; onActivated: desktop.changeVolume(-1) }
    Shortcut { sequence: "Volume Mute"; onActivated: desktop.toggleMute() }
    Shortcut {
        sequences: ["Ctrl+Space", "Alt+Space"]
        enabled: !desktop.locked
        onActivated: desktop.toggleSurface("beam")
    }
    Shortcut {
        sequences: ["Ctrl+Up", "F3"]
        enabled: !desktop.locked
        onActivated: desktop.toggleSurface("overview")
    }
    Shortcut {
        sequence: "Ctrl+Alt+Left"
        enabled: !desktop.locked
        onActivated: desktop.switchSpace(desktop.currentSpace - 1)
    }
    Shortcut {
        sequence: "Ctrl+Alt+Right"
        enabled: !desktop.locked
        onActivated: desktop.switchSpace(desktop.currentSpace + 1)
    }
    Shortcut {
        sequence: "Ctrl+Alt+L"
        enabled: !desktop.locked
        onActivated: desktop.lock()
    }
    Shortcut {
        sequence: "Ctrl+,"
        enabled: !desktop.locked
        onActivated: desktop.launch("settings", {})
    }
    Shortcut {
        sequence: "Ctrl+W"
        enabled: !desktop.locked && desktop.surface === ""
        onActivated: windows.closeActive()
    }
    Shortcut {
        sequence: "Ctrl+M"
        enabled: !desktop.locked && desktop.surface === ""
        onActivated: windows.minimizeActive()
    }
    Shortcut {
        sequence: "Escape"
        enabled: desktop.surface !== ""
        onActivated: desktop.setSurface("")
    }
    Repeater {
        model: desktop.spaceCount
        delegate: Item {
            id: spaceShortcut
            required property int index
            Shortcut {
                sequence: "Alt+" + (spaceShortcut.index + 1)
                enabled: !desktop.locked
                onActivated: desktop.switchSpace(spaceShortcut.index)
            }
        }
    }
}
