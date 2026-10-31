// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Services
import QOR.Apps
import QOR.Input
import QOR.Store

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
    property string surface: ""        // "", "home", "beam", "control-center", "overview", "media"
    property int currentSpace: 0
    readonly property int spaceCount: 3
    readonly property bool overview: surface === "overview"
    property bool startupAppsOpened: false

    readonly property real railHeight: 34
    readonly property rect workArea: Qt.rect(10, railHeight + 18, width - 20, height - railHeight - 18 - 96)

    // ── Appearance follows qor-settingsd ───────────────────────────────────
    Binding { target: Theme; property: "dark"; value: Services.settings.darkMode }
    Binding { target: Theme; property: "accentHue"; value: Services.settings.accentHue }
    Binding { target: Theme; property: "reduceMotion"; value: Services.settings.reduceMotion }
    Binding { target: Theme; property: "reduceTransparency"; value: Services.settings.reduceTransparency }
    Binding { target: Theme; property: "increaseContrast"; value: Services.settings.increaseContrast }
    Binding { target: Theme; property: "livingWallpaper"; value: Services.settings.livingWallpaper }
    // The theme's tint and wallpaper (QOR.Store Themes).
    Binding { target: Theme; property: "neutralHue"; value: Themes.active.neutralHue }
    Binding { target: Theme; property: "neutralTint"; value: Themes.active.neutralTint }
    Binding { target: Theme; property: "wallpaperSeed"; value: Themes.active.wallpaperSeed }
    Binding { target: Theme; property: "wallpaperIntensity"; value: Themes.active.wallpaperIntensity }

    // ── Interface sounds follow qor-audiod ─────────────────────────────────
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
    // A game in front plays with the controller: a product whose record says
    // so (the QQ Player), with its window focused and nothing of the shell
    // open. The game reads the controller itself, through QOR.Input, and the
    // controller interface steps aside so that a press is the game's alone.
    // Guide brings the shell back over it.
    readonly property bool controllerForGame: !locked && surface === "" && focusedLayer.active !== null
                                              && !focusedLayer.active.minimized
                                              && ProductHost.product(focusedLayer.active.appId).takesController === true
    // For the shell's script runner and tests.
    readonly property var controllerState: ({
        gamepads: Gamepads.count,
        prompt: controllerPrompt.open,
        keyboard: controllerKeyboard.open,
        cursorVisible: controller.cursorVisible,
        cursor: [Math.round(controller.cursor.x), Math.round(controller.cursor.y)],
        acting: controller.enabled,
        game: controllerForGame
    })

    function controllerConnected(gamepad: var) {
        if (Services.settings.controllerInterface) {
            toast.show("gamepad-2", qsTr("%1 connected").arg(gamepad.name));
        } else if (Services.settings.controllerAutoSwitch) {
            Services.settings.controllerInterface = true;
            toast.show("gamepad-2", qsTr("Controller interface on"));
        } else if (controllerForGame) {
            // Picked up to play: the game has it, and the question waits.
            toast.show("gamepad-2", qsTr("%1 connected").arg(gamepad.name));
        } else if (locked) {
            // Nothing a device says appears over the lock screen: asked once it is open.
            controllerAwaiting = gamepad;
        } else {
            controllerPrompt.ask(gamepad);
        }
    }
    property var controllerAwaiting: null

    function controllerAction(name: string) {
        switch (name) {
        case "search": toggleSurface("beam"); break;
        case "controlCenter": toggleSurface("control-center"); break;
        case "overview": toggleSurface("overview"); break;
        case "home":
            if (!locked)
                toggleSurface("home");
            break;
        case "back": setSurface(""); break;
        // On Home, the shoulders go between Home and the Library.
        case "previousSpace": surface === "home" ? home.showPage("home") : switchSpace(currentSpace - 1); break;
        case "nextSpace": surface === "home" ? home.showPage("library") : switchSpace(currentSpace + 1); break;
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
            return [{ button: Gamepads.South, label: qsTr("Type the password") }];
        if (surface === "home")
            return [{ button: Gamepads.DPadUp, text: qsTr("D-pad"), label: qsTr("Choose") },
                    { button: Gamepads.South, label: qsTr("Play") },
                    { button: Gamepads.RightShoulder, label: home.page === "home" ? qsTr("Library") : qsTr("Home") },
                    { button: Gamepads.East, label: qsTr("Close") }];
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
        enabled: desktop.controllerActive && !desktop.controllerForGame
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
    onControllerForGameChanged: if (controllerForGame) controllerKeyboard.open = false

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
            // On the lock screen a controller types the password like anyone
            // else; it never unlocks by itself.
            else if (desktop.controllerActive && desktop.locked && button === Gamepads.South)
                controllerKeyboard.open = true;
            // Over a game, Guide brings the shell back: Overview, to pick
            // another window or return to the game.
            else if (desktop.controllerActive && desktop.controllerForGame && button === Gamepads.Guide)
                desktop.toggleSurface("overview");
        }
    }

    // ── Shell API ───────────────────────────────────────────────────────────
    function setSurface(name: string) {
        if (locked && name !== "")
            return;
        surface = name;
        if (name === "")
            focusedLayer.focusActive();
    }

    function toggleSurface(name: string) {
        setSurface(surface === name ? "" : name);
    }

    function launch(appId: string, args: var) {
        if (locked)
            return;
        if (surface !== "overview")
            surface = "";
        // An app already open on another display comes back there.
        const owner = displays.find(layer => layer.find(appId) !== null) ?? windows;
        // A product Prism hosts, with no window yet, is asked to show one.
        if (owner.find(appId) === null && ProductHost.product(appId).launchable === true) {
            showProduct(appId, null);
            if (surface === "overview")
                surface = "";
            return;
        }
        const window = owner.open(appId, shelf.originFor(appId), args === undefined ? ({}) : args);
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

    // Every way off the lock screen comes through here. A simulated session has
    // nothing to check a password against, so any unlocks it, and the screen says
    // so. Any other stays locked until there is something to check it with
    // (QOR ID's sign-in, Phase 2).
    function tryUnlock(secret: string): bool {
        if (options.simulated !== true)
            return false;
        unlock();
        return true;
    }

    function unlock() {
        if (locked)
            Sounds.play("unlock");
        locked = false;
        desktop.forceActiveFocus();
        focusedLayer.focusActive();
        if (controllerAwaiting !== null) {
            const waiting = controllerAwaiting;
            controllerAwaiting = null;
            if (Gamepads.count > 0)
                controllerConnected(waiting);
        }
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
    // Set by Prism: a trusted xdg-activation token for a window it opens.
    property var activationToken: () => ""

    // ── Products (ARQADE, QOR Media, QQ Studio, the QQ Player) ─────────────
    // Prism hosts them as Wayland clients (ProductHost, QOR-0010). ARQADE and
    // QOR Media it places: it plans a product's window before asking the
    // product to show it and tells the product that content rectangle in
    // `place`; when the window arrives it is laid out by the same plan, made
    // again then (adoptSurface). QQ Studio and the QQ Player it only starts:
    // their windows open at the size they ask for.
    Binding { target: ProductHost; property: "reducedMotion"; value: Theme.reduceMotion }
    // They wear Lumen (QOR-0016): Demiurge's look on its roles, with the person's accent, tint and contrast.
    Binding { target: ProductHost; property: "theme"; value: Theme.productColors }

    // Shows a product, starting it if need be: `file` ({ path, kind }) is
    // what it opens: QOR Media a file, the QQ Player a game's scene. False if
    // it cannot be shown here.
    // `file` may name what to show: { path, kind } a file to open, with
    // `position` where it resumes; { game } one of ARQADE's games to play.
    function showProduct(id: string, file: var): bool {
        const product = ProductHost.product(id);
        if (product.id === undefined)
            return false;
        if (!product.placed) {
            // One run at a time: its window comes forward, on its file or not.
            const shown = ProductHost.launch(id, file ? file.path : "", activationToken());
            const owner = displays.find(layer => layer.find(id) !== null);
            if (owner)
                owner.raise(owner.find(id));
            if (!shown)
                toast.show(product.glyph, product.running ? qsTr("%1 is already open").arg(product.name)
                                                          : qsTr("%1 cannot be opened here").arg(product.name));
            return shown;
        }
        const frame = windows.planClient(product.width, product.height);
        // The content below the title bar, in the output's physical pixels
        // (1 while the output's screen is not known).
        const ratio = Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1;
        const content = Qt.rect(Math.round(frame.x * ratio), Math.round((frame.y + 40) * ratio),
                                Math.round(frame.width * ratio), Math.round((frame.height - 40) * ratio));
        const shown = !file ? ProductHost.place(id, content, activationToken())
                    : file.game ? ProductHost.play(id, file.game, content, activationToken())
                                : ProductHost.open(id, file.path, file.kind, content, activationToken(),
                                                   file.position ?? -1);
        if (!shown)
            toast.show(product.glyph, qsTr("%1 cannot be opened here").arg(product.name));
        return shown;
    }

    // Opens a file in QOR Media: a video, audio, picture or document.
    function openInMedia(path: string, kind: string): bool {
        if (locked)
            return false;
        surface = "";
        return showProduct("qor-media", { path: path, kind: kind });
    }

    // Plays a QQ game, its scene file, in the QQ Player: one game at a time.
    function playQQGame(scene: string): bool {
        if (locked)
            return false;
        surface = "";
        return showProduct("qq-player", { path: scene });
    }

    // Plays one of ARQADE's games, by its kind ("orbital").
    function playInArqade(game: string): bool {
        if (locked)
            return false;
        surface = "";
        return showProduct("arqade", { game: game });
    }

    // ── Home and the Library ────────────────────────────────────────────────
    // Opens Home at a page: "home" or "library".
    function openHome(page: string) {
        if (locked)
            return;
        home.page = page;
        setSurface("home");
    }

    // Plays a game or opens a file of the Library's (library.h), by its item,
    // a film or a song where it stopped; and remembers it was.
    function playItem(item: var): bool {
        if (!item || !item.id || locked)
            return false;
        let shown = false;
        if (item.source === "arqade") {
            shown = playInArqade(item.game);
        } else if (item.kind === "game") {
            shown = playQQGame(item.path);
        } else {
            surface = "";
            shown = showProduct("qor-media", { path: item.path, kind: item.mediaKind,
                                                position: item.position > 0 ? item.position : -1 });
        }
        if (shown)
            Library.used(item.id);
        return shown;
    }

    Binding {
        target: Library
        property: "arqade"
        value: ProductHost.products.some(product => product.id === "arqade" && product.available)
    }

    Connections {
        target: ProductHost
        // Where a film or a song stopped, and how long it is: the Library's.
        function onReported(id: string, report: var) {
            if (id === "qor-media")
                Library.reported(report);
        }
        function onExited(id: string, failed: bool) {
            const product = ProductHost.product(id);
            if (failed)
                toast.show(product.glyph, qsTr("%1 stopped").arg(product.name));
        }
    }

    // ── Displays ────────────────────────────────────────────────────────────
    // Every display's window layer, left to right: this one first, then those
    // of further outputs under Prism (DisplayDesktop). Windows move between
    // them; Spaces and Overview span them all.
    property var displays: [windows]
    // The display whose window is active: keyboard shortcuts act there.
    property WindowLayer focusedLayer: windows
    readonly property real spaceOffset: spaceTravel.value
    readonly property real lockProgress: lockDepth.value
    readonly property var runningApps: displays.reduce((ids, layer) => ids.concat(layer.runningApps.filter(id => ids.indexOf(id) < 0)), [])
    readonly property var attentionApps: displays.reduce((ids, layer) => ids.concat(layer.attentionApps.filter(id => ids.indexOf(id) < 0)), [])

    function registerDisplay(layer: WindowLayer) {
        displays = displays.concat([layer]);
        layer.windowActivated.connect(() => desktop.focusDisplay(layer));
    }

    function unregisterDisplay(layer: WindowLayer) {
        for (const window of layer.windows.slice())
            layer.moveToDisplay(window, 0, null, null);
        displays = displays.filter(l => l !== layer);
        if (focusedLayer === layer)
            focusedLayer = windows;
    }

    // One active window across all displays.
    function focusDisplay(layer: WindowLayer) {
        for (const other of displays)
            if (other !== layer && other.active)
                other.active.focused = false;
        focusedLayer = layer;
    }

    function moveActiveToDisplay(step: int) {
        const layer = focusedLayer;
        if (layer.active && !layer.active.minimized)
            layer.moveToDisplay(layer.active, layer.displayIndex + step, null, null);
    }

    function clientIdentity(appId: string): var {
        const owner = displays.find(layer => layer.find(appId) !== null) ?? windows;
        return owner.clientIdentity(appId);
    }

    function allWindows(): var {
        return displays.reduce((list, layer) => list.concat(layer.windows), []);
    }

    // `product`: the product Prism hosts that the client is (ProductHost's
    // productOf), or "". The window of a product Prism places goes where
    // Prism plans it; any other opens at the size its client asks for.
    function adoptSurface(shellSurface: var, toplevel: var, product: string): AppWindow {
        if (product !== "") {
            const record = ProductHost.product(product);
            return windows.adopt(shellSurface, toplevel, {
                                     id: product,
                                     frame: record.placed ? windows.planClient(record.width, record.height) : null
                                 });
        }
        // prism --spread-clients: each new client on the display with the
        // fewest windows (benchmarks put load on every output).
        let layer = windows;
        if (options.spreadClients)
            for (const other of displays)
                if (other.windows.length < layer.windows.length)
                    layer = other;
        return layer.adopt(shellSurface, toplevel, null);
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

    // ── Widgets ─────────────────────────────────────────────────────────────
    // Editing the desktop's widgets moves the windows aside and opens the
    // gallery in place of the Shelf.
    property bool editingWidgets: false

    function editWidgets(on: bool) {
        if (on && locked)
            return;
        if (on)
            setSurface("");
        editingWidgets = on;
    }

    function addWidget(id: string) {
        editWidgets(true);
        if (!widgets.add(id))
            toast.show("layout-grid", qsTr("There is no room for that widget"));
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
                                          qsTr("Notifications travel over Qt Remote Objects from qor-notifyd."),
                                          "bell-ring");
            break;
        case "space.go": switchSpace(args.index); break;
        case "window.space": focusedLayer.moveActiveTo(args.index); break;
        case "window.display": focusedLayer.moveToDisplay(focusedLayer.active, args.index, null, null); break;
        case "accent.set": settings.accentHue = args.hue; break;
        case "app.open": launch(args.id, args); break;
        case "widgets.edit": editWidgets(true); break;
        case "store.open": launch("store", args); break;
        case "home.open": openHome("home"); break;
        case "library.open": openHome("library"); break;
        case "library.play": playItem(Library.item(args.id)); break;
        case "theme.apply": Themes.apply(args.id ?? ""); break;
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
            case "widgets.add": desktop.addWidget(args.id); break;
            case "widgets.edit": desktop.editWidgets(true); break;
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
        Library.refresh();
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
            if (desktop.options.surface === "home" || desktop.options.surface === "library")
                desktop.openHome(desktop.options.surface);
            else
                desktop.setSurface(desktop.options.surface);
            if (desktop.options.surface === "beam" && desktop.options.query)
                beam.query = desktop.options.query;
        }
    }

    // ── Motion drivers ──────────────────────────────────────────────────────
    Spring { id: lockDepth; target: desktop.locked ? 1 : 0; preset: Motion.gentle }
    Spring { id: editDepth; target: desktop.editingWidgets ? 1 : 0; preset: Motion.smooth }
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

        WidgetLayer {
            id: widgets
            objectName: "widgetLayer"
            anchors.fill: parent
            area: desktop.workArea
            backdrop: wallpaperBackdrop
            editing: desktop.editingWidgets
            opacity: (1 - lockDepth.value) * (1 - overviewDepth.value)
            visible: opacity > 0.002
            // Only for the desktop itself. A right click on a window (or the
            // Rail, or the Shelf) reaches this layer's TapHandler too.
            onMenuRequested: scenePosition => {
                const area = desktop.workArea;
                if (desktop.surface === "" && !windows.windowAt(scenePosition)
                        && scenePosition.x >= area.x && scenePosition.x <= area.x + area.width
                        && scenePosition.y >= area.y && scenePosition.y <= area.y + area.height)
                    desktopMenu.popup(scenePosition.x, scenePosition.y);
            }
        }

        WindowLayer {
            id: windows
            objectName: "windowLayer"
            anchors.fill: parent
            displays: desktop.displays
            onWindowActivated: desktop.focusDisplay(desktop.windowLayer)
            workArea: desktop.workArea
            currentSpace: desktop.currentSpace
            spaceOffset: spaceTravel.value
            overview: desktop.overview
            opacity: (1 - lockDepth.value) * (1 - editDepth.value)
            scale: 1 - 0.035 * lockDepth.value
            visible: opacity > 0.002
            enabled: !desktop.editingWidgets && !desktop.locked
            locked: desktop.locked
            surfaceOpen: desktop.surface !== ""
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
            activeTitle: desktop.focusedLayer.activeTitle
            currentSpace: desktop.currentSpace
            spaceCount: desktop.spaceCount
            surface: desktop.surface
            onSurfaceRequested: name => desktop.toggleSurface(name)
        }

        WidgetGallery {
            id: widgetGallery
            objectName: "widgetGallery"
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width - 48, 1040)
            height: implicitHeight
            y: parent.height - height - 16 + (height + 40) * (1 - editDepth.value)
            open: desktop.editingWidgets
            visible: editDepth.value > 0.002
            backdrop: sceneBackdrop
            onAddRequested: id => desktop.addWidget(id)
            onStoreRequested: {
                desktop.editWidgets(false);
                desktop.launch("store", { page: "widgets" });
            }
            onDoneRequested: desktop.editWidgets(false)
        }

        Shelf {
            id: shelf
            objectName: "shelf"
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - height - 12 + 110 * Math.max(lockDepth.value, editDepth.value)
            backdrop: sceneBackdrop
            runningApps: desktop.runningApps
            attentionApps: desktop.attentionApps
            identityProvider: appId => desktop.clientIdentity(appId)
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

        Home {
            id: home
            objectName: "home"
            anchors.fill: parent
            topMargin: desktop.railHeight
            open: desktop.surface === "home"
            backdrop: sceneBackdrop
            onDismissed: desktop.setSurface("")
            onChosen: item => desktop.playItem(item)
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
        simulated: desktop.options.simulated === true
        unlockedBy: secret => desktop.tryUnlock(secret)
    }

    // ── Controller interface surfaces (above the lock screen too) ──────────
    ControllerHints {
        objectName: "controllerHints"
        x: 16
        y: parent.height - height - 18
        backdrop: sceneBackdrop
        active: desktop.controllerActive && !desktop.controllerForGame
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
    Shortcut { context: Qt.ApplicationShortcut; sequences: ["Toggle Media Play/Pause", "Media Play"]; onActivated: Services.media.togglePlayback() }
    Shortcut { context: Qt.ApplicationShortcut; sequences: ["Media Pause", "Media Stop"]; onActivated: Services.media.playing = false }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Media Next"; onActivated: Services.media.next() }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Media Previous"; onActivated: Services.media.previous() }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Volume Up"; onActivated: desktop.changeVolume(1) }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Volume Down"; onActivated: desktop.changeVolume(-1) }
    Shortcut { context: Qt.ApplicationShortcut; sequence: "Volume Mute"; onActivated: desktop.toggleMute() }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequences: ["Ctrl+Space", "Alt+Space"]
        enabled: !desktop.locked
        onActivated: desktop.toggleSurface("beam")
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequences: ["Ctrl+Up", "F3"]
        enabled: !desktop.locked
        onActivated: desktop.toggleSurface("overview")
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Alt+Left"
        enabled: !desktop.locked
        onActivated: desktop.switchSpace(desktop.currentSpace - 1)
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Alt+Right"
        enabled: !desktop.locked
        onActivated: desktop.switchSpace(desktop.currentSpace + 1)
    }
    // Move the active window to another Space, and go with it.
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Alt+Shift+Left"
        enabled: !desktop.locked
        onActivated: desktop.focusedLayer.moveActive(-1)
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Alt+Shift+Right"
        enabled: !desktop.locked
        onActivated: desktop.focusedLayer.moveActive(1)
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+Alt+L"
        enabled: !desktop.locked
        onActivated: desktop.lock()
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+,"
        enabled: !desktop.locked
        onActivated: desktop.launch("settings", {})
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+W"
        enabled: !desktop.locked && desktop.surface === ""
        onActivated: desktop.focusedLayer.closeActive()
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Ctrl+M"
        enabled: !desktop.locked && desktop.surface === ""
        onActivated: desktop.focusedLayer.minimizeActive()
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Escape"
        enabled: desktop.surface !== "" || desktop.editingWidgets
        onActivated: desktop.editingWidgets ? desktop.editWidgets(false) : desktop.setSurface("")
    }

    // Take the active window to the display on the left or right.
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Meta+Shift+Left"
        enabled: !desktop.locked && desktop.displays.length > 1
        onActivated: desktop.moveActiveToDisplay(-1)
    }
    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "Meta+Shift+Right"
        enabled: !desktop.locked && desktop.displays.length > 1
        onActivated: desktop.moveActiveToDisplay(1)
    }

    // The desktop's own menu (right click, or press and hold, on the desktop).
    Menu {
        id: desktopMenu
        objectName: "desktopMenu"
        MenuItem {
            objectName: "editWidgets"
            text: qsTr("Edit Widgets")
            onTriggered: desktop.editWidgets(true)
        }
        MenuItem {
            objectName: "changeTheme"
            text: qsTr("Change Theme…")
            onTriggered: desktop.launch("settings", { page: "appearance" })
        }
        MenuSeparator { }
        MenuItem {
            objectName: "openStore"
            text: qsTr("Get Themes and Widgets…")
            onTriggered: desktop.launch("store", { page: "discover" })
        }
    }
    Repeater {
        model: desktop.spaceCount
        delegate: Item {
            id: spaceShortcut
            required property int index
            Shortcut {
                context: Qt.ApplicationShortcut
                sequence: "Alt+" + (spaceShortcut.index + 1)
                enabled: !desktop.locked
                onActivated: desktop.switchSpace(spaceShortcut.index)
            }
            Shortcut {
                context: Qt.ApplicationShortcut
                sequence: "Alt+Shift+" + (spaceShortcut.index + 1)
                enabled: !desktop.locked
                onActivated: desktop.focusedLayer.moveActiveTo(spaceShortcut.index)
            }
        }
    }
}
