// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services
import QTOS.Apps
import QTOS.Input

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

    // ── Controller interface ────────────────────────────────────────────────
    // A game controller drives everything while this is on (see
    // docs/CONTROLLERS.md). Keyboard and mouse always keep working.
    readonly property bool controllerActive: Services.settings.controllerInterface && Gamepads.count > 0
    // For the shell's script runner and tests.
    readonly property var controllerState: ({
        gamepads: Gamepads.count,
        prompt: controllerPrompt.open,
        keyboard: controllerKeyboard.open,
        cursorVisible: controller.cursorVisible,
        cursor: [Math.round(controller.cursor.x), Math.round(controller.cursor.y)]
    })

    function controllerConnected(gamepad: var) {
        if (Services.settings.controllerInterface) {
            toast.show("gamepad-2", qsTr("%1 connected").arg(gamepad.name));
        } else if (Services.settings.controllerAutoSwitch) {
            Services.settings.controllerInterface = true;
            toast.show("gamepad-2", qsTr("Controller interface on"));
        } else {
            controllerPrompt.ask(gamepad);
        }
    }

    function controllerAction(name: string) {
        switch (name) {
        case "search": toggleSurface("beam"); break;
        case "controlCenter": toggleSurface("control-center"); break;
        case "overview": toggleSurface("overview"); break;
        case "home":
            if (locked)
                unlock();
            else
                setSurface("");
            break;
        case "back": setSurface(""); break;
        case "previousSpace": switchSpace(currentSpace - 1); break;
        case "nextSpace": switchSpace(currentSpace + 1); break;
        case "volumeDown": changeVolume(-1); break;
        case "volumeUp": changeVolume(1); break;
        case "keyboard": controllerKeyboard.open = !controllerKeyboard.open; break;
        case "playPause": Services.media.togglePlayback(); break;
        }
        if (name !== "volumeDown" && name !== "volumeUp")
            controller.feedback(0.3, 35);
    }

    readonly property var controllerHints: {
        if (controllerKeyboard.open)
            return [{ button: Gamepads.South, label: qsTr("Type") }, { button: Gamepads.West, label: qsTr("Delete") },
                    { button: Gamepads.North, label: qsTr("Space") }, { button: Gamepads.Start, label: qsTr("Enter") },
                    { button: Gamepads.East, label: qsTr("Close") }];
        if (locked)
            return [{ button: Gamepads.South, label: qsTr("Unlock") }];
        if (surface === "beam")
            return [{ button: Gamepads.DPadUp, text: qsTr("D-pad"), label: qsTr("Choose") },
                    { button: Gamepads.South, label: qsTr("Open") },
                    { button: Gamepads.LeftStick, label: qsTr("Keyboard") },
                    { button: Gamepads.East, label: qsTr("Close") }];
        if (surface !== "")
            return [{ button: Gamepads.South, label: qsTr("Select") }, { button: Gamepads.East, label: qsTr("Close") }];
        return [{ button: Gamepads.South, label: qsTr("Select") }, { button: Gamepads.East, label: qsTr("Back") },
                { button: Gamepads.North, label: qsTr("Search") },
                { button: Gamepads.Start, label: qsTr("Control Center") },
                { button: Gamepads.Back, label: qsTr("Overview") }];
    }

    ControllerNavigator {
        id: controller
        window: desktop.Window.window
        enabled: desktop.controllerActive
        cursorSpeed: Services.settings.controllerCursorSpeed
        vibration: Services.settings.controllerVibration
        keyboardOpen: controllerKeyboard.open
        onAction: name => desktop.controllerAction(name)
        onKeyboardMove: (dx, dy) => controllerKeyboard.move(dx, dy)
        onKeyboardButton: button => controllerKeyboard.button(button)
        // The keyboard follows text fields.
        onTextInputFocusedChanged: controllerKeyboard.open = desktop.controllerActive && textInputFocused
    }

    onControllerActiveChanged: {
        if (!controllerActive)
            controllerKeyboard.open = false;
        else if (controller.textInputFocused)
            controllerKeyboard.open = true;
    }

    // PlayStation light bars glow in the accent colour.
    readonly property color controllerLight: Services.settings.controllerLightbar
                                             ? Oklch.color(0.68, 0.17, Services.settings.accentHue)
                                             : Qt.rgba(0.08, 0.08, 0.1, 1)
    onControllerLightChanged: Gamepads.setLightColor(controllerLight)

    Connections {
        target: Gamepads
        function onConnected(gamepad: var) {
            desktop.controllerConnected(gamepad);
        }
        function onDisconnected(gamepad: var) {
            if (Gamepads.count > 0)
                return;
            controllerPrompt.close();
            if (Services.settings.controllerInterface) {
                Services.settings.controllerInterface = false;
                toast.show("keyboard", qsTr("Controller disconnected · keyboard and mouse"));
            }
        }
        function onPressed(button: int) {
            // The prompt answers to the controller before the interface is on.
            if (controllerPrompt.open && !desktop.controllerActive)
                controllerPrompt.controllerButton(button);
            else if (desktop.controllerActive && desktop.locked && button === Gamepads.South)
                desktop.unlock();
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

    // ── Gestures ────────────────────────────────────────────────────────────
    // Three fingers, on a touchpad or a touchscreen: sideways moves between
    // Spaces, up opens Overview and down closes it. The view follows the
    // fingers exactly; on release the springs carry on with their velocity.
    readonly property real overviewGestureDistance: 300
    readonly property real overviewProgress: overviewDepth.value
    // Spaces the strip is dragged by, and how far Overview is pulled open.
    property real spaceDrag: 0
    property bool spaceDragging: false
    property real overviewDrag: 0
    property real overviewDragStart: 0
    property bool overviewDragging: false
    // The springs stand still while fingers drive them. Cleared before the
    // targets move on, so they animate from where the fingers left them.
    property bool gestureTracking: false

    SystemGestures {
        id: gestures
        objectName: "systemGestures"
        window: desktop.Window.window
        enabled: !desktop.locked
        onStarted: gestures.axis === Qt.Horizontal ? desktop.beginSpaceGesture() : desktop.beginOverviewGesture()
        onTranslationChanged: {
            if (desktop.spaceDragging)
                desktop.trackSpaceGesture(gestures.translation.x);
            else if (desktop.overviewDragging)
                desktop.trackOverviewGesture(gestures.translation.y);
        }
        onFinished: desktop.endGesture(gestures.velocity, true)
        onCanceled: desktop.endGesture(Qt.point(0, 0), false)
    }

    function beginSpaceGesture() {
        spaceDrag = 0;
        gestureTracking = true;
        spaceDragging = true;
    }

    function trackSpaceGesture(dx: real) {
        // The desktop follows the fingers: moving left brings the next Space
        // in. Past the first and last Space it resists.
        const last = spaceCount - 1;
        let position = currentSpace - dx / Math.max(1, width);
        if (position < 0)
            position *= 0.3;
        else if (position > last)
            position = last + (position - last) * 0.3;
        spaceDrag = position - currentSpace;
    }

    function beginOverviewGesture() {
        overviewDragStart = overview ? 1 : 0;
        overviewDrag = overviewDragStart;
        gestureTracking = true;
        overviewDragging = true;
        windows.overviewGesture = overviewDrag;
        windows.overviewTracking = true;
        windows.relayout();
    }

    function trackOverviewGesture(dy: real) {
        overviewDrag = Math.max(0, Math.min(1, overviewDragStart - dy / overviewGestureDistance));
        windows.overviewGesture = overviewDrag;
    }

    // `released` is false for a cancelled gesture, which goes back.
    function endGesture(velocity: point, released: bool) {
        if (spaceDragging) {
            // Spaces per second; a flick carries on to the next Space.
            const speed = released ? -velocity.x / Math.max(1, width) : 0;
            const position = currentSpace + spaceDrag;
            const target = released
                ? Math.round(Math.max(currentSpace - 1, Math.min(currentSpace + 1, position + speed * 0.2)))
                : currentSpace;
            gestureTracking = false;
            spaceTravel.velocity = speed;
            spaceDragging = false;
            spaceDrag = 0;
            switchSpace(target);
        } else if (overviewDragging) {
            const speed = released ? -velocity.y / overviewGestureDistance : 0;
            const open = released ? overviewDrag + speed * 0.2 > 0.5 : overviewDragStart > 0.5;
            // While tracking, nothing moves yet.
            if (open !== overview)
                setSurface(open ? "overview" : "");
            gestureTracking = false;
            windows.overviewTracking = false;
            overviewDepth.velocity = speed;
            overviewDragging = false;
            windows.overviewGesture = -1;
            windows.relayout();
        }
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
        case "window.space": windows.moveActiveTo(args.index); break;
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
        Gamepads.setLightColor(controllerLight);
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
    Spring {
        id: overviewDepth
        target: desktop.overviewDragging ? desktop.overviewDrag : desktop.overview ? 1 : 0
        enabled: !desktop.gestureTracking
        preset: Motion.smooth
    }
    Spring {
        id: spaceTravel
        target: desktop.currentSpace + desktop.spaceDrag
        enabled: !desktop.gestureTracking
        preset: Motion.smooth
    }

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
            spaceAt: scenePoint => spaceSwitcher.indexAt(scenePoint)
            onSpaceRequested: index => desktop.switchSpace(index)
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
            id: spaceSwitcher
            objectName: "spaceSwitcher"
            dropHint: windows.spaceDropHint
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

    // ── Controller interface surfaces (above the lock screen too) ──────────
    ControllerHints {
        objectName: "controllerHints"
        x: 16
        y: parent.height - height - 18
        backdrop: sceneBackdrop
        active: desktop.controllerActive
        entries: desktop.controllerHints
    }

    ControllerKeyboard {
        id: controllerKeyboard
        objectName: "controllerKeyboard"
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - height - 104
        backdrop: sceneBackdrop
        navigator: controller
    }

    ControllerPrompt {
        id: controllerPrompt
        objectName: "controllerPrompt"
        anchors.horizontalCenter: parent.horizontalCenter
        y: desktop.railHeight + 18
        backdrop: sceneBackdrop
        onAccepted: always => {
            if (always)
                Services.settings.controllerAutoSwitch = true;
            Services.settings.controllerInterface = true;
            toast.show("gamepad-2", qsTr("Controller interface on"));
        }
    }

    ShellToast {
        id: toast
        objectName: "toast"
        anchors.horizontalCenter: parent.horizontalCenter
        y: desktop.railHeight + 18
        backdrop: sceneBackdrop
    }

    // Above the lock screen: the volume keys work there too.
    VolumeHud {
        id: volumeHud
        objectName: "volumeHud"
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - height - 112
        backdrop: sceneBackdrop
    }

    ControllerCursor {
        objectName: "controllerCursor"
        z: 1000
        navigator: controller
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
    // Move the active window to another Space, and go with it.
    Shortcut {
        sequence: "Ctrl+Alt+Shift+Left"
        enabled: !desktop.locked
        onActivated: windows.moveActive(-1)
    }
    Shortcut {
        sequence: "Ctrl+Alt+Shift+Right"
        enabled: !desktop.locked
        onActivated: windows.moveActive(1)
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
            Shortcut {
                sequence: "Alt+Shift+" + (spaceShortcut.index + 1)
                enabled: !desktop.locked
                onActivated: windows.moveActiveTo(spaceShortcut.index)
            }
        }
    }
}
