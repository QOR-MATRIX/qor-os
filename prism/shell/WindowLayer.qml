// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services
import QTOS.Apps

// Window management for one output: stacking, focus, Spaces, Overview layout
// and magnetic snapping. Windows of every Space live on one horizontal strip
// that slides as `spaceOffset` springs between Spaces.
Item {
    id: layer

    property rect workArea
    property int currentSpace: 0
    property real spaceOffset: 0
    property bool overview: false
    property int spaceCount: 3
    property var originProvider: appId => Qt.rect(width / 2, height, 0, 0)
    // The Space chip under a scene point in Overview, or -1 (set by the shell).
    property var spaceAt: scenePoint => -1
    // The chip a window dragged in Overview would land on.
    property int spaceDropHint: -1

    // Every managed window, in creation order. Reassigned (never mutated in
    // place) so bindings such as runningApps re-evaluate.
    property var windows: []
    property AppWindow active: null
    property int stackCounter: 1
    property int serialCounter: 0

    // Content for windows of real Wayland clients. Set by the Prism compositor
    // (a ShellSurfaceItem component); null in simulation.
    property Component surfaceDelegate: null

    // Snap preview shown while dragging a window to a screen edge.
    property rect snapRect
    property bool snapVisible: false

    readonly property string activeTitle: active && !active.minimized && !active.closing ? active.title : ""
    readonly property var runningApps: {
        const ids = [];
        for (const w of windows)
            if (!w.closing && ids.indexOf(w.appId) < 0)
                ids.push(w.appId);
        return ids;
    }

    readonly property var attentionApps: {
        const ids = [];
        for (const w of windows)
            if (w.demandsAttention && !w.closing && ids.indexOf(w.appId) < 0)
                ids.push(w.appId);
        return ids;
    }

    signal overviewPicked()
    // The shell owns currentSpace (it is bound here): ask it to switch.
    signal spaceRequested(int index)
    function requestSpace(index: int) {
        if (index !== currentSpace)
            spaceRequested(index);
    }

    onOverviewChanged: relayout()
    onCurrentSpaceChanged: {
        relayout();
        const candidates = windowsOn(currentSpace).filter(w => !w.minimized);
        activate(candidates.length > 0 ? candidates.reduce((a, b) => a.z > b.z ? a : b) : null);
    }
    onWidthChanged: relayout()
    onHeightChanged: relayout()

    // ── Window management API ───────────────────────────────────────────────
    function windowsOn(space: int): var {
        return windows.filter(w => w.space === space && !w.closing);
    }

    // New windows cascade around the centre of the work area, never past its
    // edges (the top edge would put a title bar under the Rail).
    function cascadeX(w: real, cascade: int): real {
        const x = workArea.x + (workArea.width - w) / 2 + cascade * 30;
        return Math.max(workArea.x, Math.min(x, workArea.x + workArea.width - w));
    }
    function cascadeY(h: real, cascade: int): real {
        const y = workArea.y + (workArea.height - h) / 2 + cascade * 24;
        return Math.max(workArea.y, Math.min(y, workArea.y + workArea.height - h));
    }

    // Name and icon for an app that is not a first-party one (a Wayland
    // client): from its desktop entry, else its window title.
    function clientIdentity(appId: string): var {
        const entry = DesktopEntries.lookup(appId);
        const window = find(appId);
        return {
            name: entry.found ? entry.name : (window && window.title.length > 0 ? window.title : appId),
            icon: entry.found ? entry.icon : ""
        };
    }

    // Moves a window to another Space. With `follow`, the view goes along and
    // carries the window across; otherwise the window slides away to it.
    function moveToSpace(window: AppWindow, space: int, follow: bool) {
        space = Math.max(0, Math.min(spaceCount - 1, space));
        if (!window || window.closing || window.space === space)
            return;
        window.space = space;
        if (follow) {
            requestSpace(space);
            activate(window);
        } else if (active === window) {
            const others = windowsOn(currentSpace).filter(w => !w.minimized);
            activate(others.length > 0 ? others.reduce((a, b) => a.z > b.z ? a : b) : null);
        }
        relayout();
    }

    // The keyboard's moves: by a step, or to a Space, carrying the window.
    function moveActive(step: int) {
        if (active && !active.minimized)
            moveToSpace(active, active.space + step, true);
    }
    function moveActiveTo(space: int) {
        if (active && !active.minimized)
            moveToSpace(active, space, true);
    }

    // Overview dragging.
    function raiseInOverview(window: AppWindow) {
        window.z = ++stackCounter;
    }
    function hoverInOverview(scenePoint: point) {
        spaceDropHint = spaceAt(scenePoint);
    }
    function dropInOverview(window: AppWindow, scenePoint: point) {
        const target = spaceAt(scenePoint);
        spaceDropHint = -1;
        if (target >= 0 && target !== window.space)
            moveToSpace(window, target, false);
        else
            relayout();
    }

    // The window a person sees at a scene point: the highest visible one
    // whose frame contains it.
    function windowAt(scenePoint: point): AppWindow {
        let best = null;
        for (const w of windows) {
            if (w.closing || w.minimized || !w.visible || (best && w.z <= best.z))
                continue;
            const p = w.mapFromItem(null, scenePoint.x, scenePoint.y);
            if (p.x >= 0 && p.y >= 0 && p.x < w.width && p.y < w.height)
                best = w;
        }
        return best;
    }

    // The most recently raised open window of an app.
    function find(appId: string): AppWindow {
        let best = null;
        for (const w of windows)
            if (w.appId === appId && !w.closing && (!best || w.z > best.z))
                best = w;
        return best;
    }

    // Brings an app's window back (restoring it if minimised), or opens a
    // first-party app. Wayland clients are never in the registry, so a
    // running window is looked up first: that is how the Shelf restores them.
    function open(appId: string, origin: rect, args: var): AppWindow {
        const existing = find(appId);
        if (existing) {
            if (!existing.isClient)
                existing.launchArgs = args;
            if (existing.minimized) {
                existing.origin = origin;
                existing.minimized = false;
            }
            if (existing.space !== currentSpace)
                requestSpace(existing.space);
            activate(existing);
            relayout();
            return existing;
        }

        const app = AppRegistry.app(appId);
        if (!app)
            return null;

        const index = windowsOn(currentSpace).length;
        const w = Math.min(app.width, workArea.width - 60);
        const h = Math.min(app.height, workArea.height - 40);
        const cascade = (index % 5) - 2;
        const window = windowFactory.createObject(strip, {
            manager: layer,
            appId: appId,
            title: app.name,
            space: currentSpace,
            normalX: cascadeX(w, cascade),
            normalY: cascadeY(h, cascade),
            normalW: w,
            normalH: h,
            origin: origin,
            launchArgs: args,
            serial: ++serialCounter
        });
        window.closed.connect(() => layer.remove(window));
        windows = windows.concat([window]);
        activate(window);
        relayout();
        return window;
    }

    // Manages a window for a Wayland xdg-toplevel (Prism compositor only).
    function adopt(shellSurface: var, toplevel: var): AppWindow {
        const index = windowsOn(currentSpace).length;
        const w = Math.min(960, workArea.width - 60);
        const h = Math.min(640, workArea.height - 40);
        const cascade = (index % 5) - 2;
        const window = windowFactory.createObject(strip, {
            manager: layer,
            appId: toplevel.appId,
            title: toplevel.title,
            space: currentSpace,
            normalX: cascadeX(w, cascade),
            normalY: cascadeY(h, cascade),
            normalW: w,
            normalH: h,
            origin: Qt.rect(workArea.x + workArea.width / 2 - 40, workArea.y + workArea.height / 2 - 30, 80, 60),
            shellSurface: shellSurface,
            toplevel: toplevel,
            serial: ++serialCounter
        });
        // Clients usually set their title and app id just after creating the
        // toplevel, so both are followed rather than read once.
        toplevel.titleChanged.connect(() => window.title = toplevel.title);
        toplevel.appIdChanged.connect(() => window.appId = toplevel.appId);
        window.closed.connect(() => layer.remove(window));
        windows = windows.concat([window]);
        activate(window);
        relayout();
        return window;
    }

    function remove(window: AppWindow) {
        windows = windows.filter(w => w !== window);
        if (active === window)
            activate(topmost());
        window.destroy();
        relayout();
    }

    function topmost(): AppWindow {
        let best = null;
        for (const w of windowsOn(currentSpace))
            if (!w.minimized && (!best || w.z > best.z))
                best = w;
        return best;
    }

    // Brings a specific window to the front: restores it if minimised and
    // switches to its Space.
    function raise(window: AppWindow) {
        if (!window || window.closing)
            return;
        if (window.minimized) {
            window.origin = originProvider(window.appId);
            window.minimized = false;
        }
        if (window.space !== currentSpace)
            requestSpace(window.space);
        activate(window);
        relayout();
    }

    function activate(window: AppWindow) {
        if (active && active !== window)
            active.focused = false;
        active = window;
        if (!window)
            return;
        window.z = ++stackCounter;
        window.focused = true;
        window.focusContent();
    }

    function focusActive() {
        if (active)
            active.focusContent();
    }

    // Closes a window. Wayland clients are asked to close and are removed when
    // their surface is destroyed; `force` skips the request.
    function close(window: AppWindow, force: bool) {
        if (!window || window.closing)
            return;
        if (window.toplevel && !force) {
            window.toplevel.sendClose();
            return;
        }
        window.closing = true;
        if (active === window)
            activate(topmost());
        relayout();
    }

    function minimize(window: AppWindow) {
        if (!window)
            return;
        window.origin = originProvider(window.appId);
        window.minimized = true;
        if (active === window)
            activate(topmost());
        relayout();
    }

    function closeActive() { close(active, false); }
    function minimizeActive() { minimize(active); }

    function pickInOverview(window: AppWindow) {
        activate(window);
        overviewPicked();
    }

    // Arranges the current Space's windows in a centred grid for Overview.
    function relayout() {
        for (const w of windows)
            if (w.space !== currentSpace || !overview || w.minimized || w.closing)
                w.inOverview = false;
        if (!overview)
            return;

        const list = windowsOn(currentSpace).filter(w => !w.minimized);
        const n = list.length;
        if (n === 0)
            return;
        list.sort((a, b) => a.serial - b.serial);

        const area = Qt.rect(workArea.x + 48, workArea.y + 64, workArea.width - 96, workArea.height - 84);
        const cols = Math.max(1, Math.ceil(Math.sqrt(n * (area.width / area.height) / 1.6)));
        const rows = Math.ceil(n / cols);
        const cellW = area.width / cols;
        const cellH = area.height / rows;

        for (let i = 0; i < n; ++i) {
            const w = list[i];
            const col = i % cols;
            const row = Math.floor(i / cols);
            const inRow = row === rows - 1 ? n - cols * (rows - 1) : cols;
            const rowOffset = (cols - inRow) * cellW / 2;
            const s = Math.min((cellW - 36) / w.frameWidth, (cellH - 36) / w.frameHeight, 0.82);
            w.overviewScale = s;
            w.overviewRect = Qt.rect(area.x + rowOffset + col * cellW + (cellW - w.frameWidth * s) / 2,
                                     area.y + row * cellH + (cellH - w.frameHeight * s) / 2,
                                     w.frameWidth * s, w.frameHeight * s);
            w.inOverview = true;
        }
    }

    // Returns the snap target for a pointer position, or an empty rect.
    function snapTargetFor(point: point): rect {
        const edge = 8;
        if (point.y <= workArea.y - 10)
            return workArea;
        if (point.x <= workArea.x + edge)
            return Qt.rect(workArea.x, workArea.y, workArea.width / 2, workArea.height);
        if (point.x >= workArea.x + workArea.width - edge)
            return Qt.rect(workArea.x + workArea.width / 2, workArea.y, workArea.width / 2, workArea.height);
        return Qt.rect(0, 0, 0, 0);
    }

    // ── Scene ───────────────────────────────────────────────────────────────
    Component {
        id: windowFactory
        AppWindow { }
    }

    // Snap preview: a translucent glass slab where the window will land.
    Surface {
        id: snapPreview
        readonly property real presence: snapSpring.value
        x: layer.snapRect.x + 6 * (1 - presence)
        y: layer.snapRect.y + 6 * (1 - presence)
        width: Math.max(0, layer.snapRect.width - 12 * (1 - presence))
        height: Math.max(0, layer.snapRect.height - 12 * (1 - presence))
        visible: presence > 0.01
        opacity: presence
        radius: Tokens.radius.window + 4
        color: Oklch.alpha(Theme.accent, 0.16)
        strokeColor: Oklch.alpha(Theme.accent, 0.55)
        strokeWidth: 1.5
        Spring { id: snapSpring; target: layer.snapVisible ? 1 : 0; preset: Motion.snappy }
    }

    Item {
        id: strip
        x: -layer.spaceOffset * layer.width
        width: layer.width
        height: layer.height
    }

    // Horizontal two-finger swipes on empty desktop switch Spaces.
    WheelHandler {
        id: swipe
        property real accumulated: 0
        acceptedDevices: PointerDevice.TouchPad
        orientation: Qt.Horizontal
        onWheel: event => {
            accumulated += event.pixelDelta.x;
            if (Math.abs(accumulated) > 140) {
                const next = layer.currentSpace + (accumulated < 0 ? 1 : -1);
                accumulated = 0;
                if (next >= 0 && next < layer.spaceCount)
                    Intents.request("shell.space", { "index": next });
            }
        }
    }

    TapHandler {
        enabled: layer.overview
        onTapped: layer.overviewPicked()
    }
}
