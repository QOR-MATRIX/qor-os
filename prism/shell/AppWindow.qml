// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Apps

// A managed window: Prism-drawn decoration, app content clipped to a
// continuous-curvature shape, and spring-driven geometry for open, close,
// minimise, maximise, snapping and Overview.
//
// In simulation the content is an in-process QML app; under the Prism
// compositor the same frame hosts a Wayland ShellSurfaceItem.
Item {
    id: win

    // ── Identity ────────────────────────────────────────────────────────────
    property WindowLayer manager
    property string appId
    property string title
    property var launchArgs: ({})
    property int space: 0
    property int serial: 0
    // Buffers a Wayland client has committed (counted by Prism; tests use it
    // to see that a hidden client stops drawing).
    property int commits: 0

    // Set only for real Wayland clients (Prism compositor).
    property var shellSurface: null
    property var toplevel: null
    readonly property bool isClient: shellSurface !== null
    // False for clients that draw their own title bar (client-side
    // decorations): Prism then draws no title bar of its own.
    property bool decorated: true
    readonly property bool hasTitleBar: !isClient || decorated
    // A product Prism started, not a client that opened a window of its own.
    property bool startedByPrism: false
    // A client's window opening at the size the client asks for: unseen, at
    // a provisional frame, until the client's first commit says what that
    // size is (adoptContentSize).
    property bool awaitingSize: false
    // The app asked for focus but was not allowed to take it (focus-stealing
    // prevention); the Shelf shows it until the window is focused.
    property bool demandsAttention: false
    onFocusedChanged: if (focused) demandsAttention = false

    // ── State ───────────────────────────────────────────────────────────────
    property real normalX: 100
    property real normalY: 100
    property real normalW: 800
    property real normalH: 560
    property bool maximized: false
    property rect restoreRect: Qt.rect(0, 0, 0, 0)
    property bool minimized: false
    property bool closing: false
    property bool focused: false
    property rect origin: Qt.rect(0, 0, 0, 0)
    property bool inOverview: false
    // Placed in the Overview grid (in Overview, or during an Overview gesture).
    property bool inOverviewGrid: false
    property rect overviewRect: Qt.rect(0, 0, 0, 0)
    property real overviewScale: 1

    signal closed()

    readonly property rect frameRect: maximized ? manager.workArea : Qt.rect(normalX, normalY, normalW, normalH)
    readonly property real frameWidth: frameRect.width
    readonly property real frameHeight: frameRect.height
    readonly property real titleBarHeight: 40
    // The size a client's content settles at (the frame animates towards it).
    readonly property size contentTargetSize: Qt.size(frameWidth,
                                                      frameHeight - (isClient && decorated ? titleBarHeight : 0))

    // A client chose a different size from the one Prism configured, for
    // example a terminal snapping to whole character cells: fit the frame.
    // Its first size, which it chose itself, is planned like a new window's:
    // within the work area (the client is then configured to fit), and the
    // window flies in at that frame.
    function adoptContentSize(contentWidth: real, contentHeight: real) {
        const chrome = isClient && decorated ? titleBarHeight : 0;
        if (awaitingSize) {
            const frame = manager.planFrame(contentWidth, contentHeight + chrome, win);
            normalX = frame.x;
            normalY = frame.y;
            normalW = frame.width;
            normalH = frame.height;
            arrive();
            awaitingSize = false;
            manager.arrived(win);
            return;
        }
        if (maximized || inOverview)
            return;
        // Never larger than the work area: the frame, and the texture its
        // shadow is drawn from, are what a client's size decides.
        const area = manager.workArea;
        normalW = area.width > 0 ? Math.min(contentWidth, area.width) : contentWidth;
        normalH = area.height > 0 ? Math.min(contentHeight + chrome, area.height) : contentHeight + chrome;
    }

    function focusContent() {
        const content = appLoader.item as Item;
        if (content)
            content.forceActiveFocus();
    }

    // Apps and client surfaces opt into these properties by name, so the
    // content is deliberately handled as an untyped object here.
    function configureContent(content: var) {
        if (isClient) {
            content.window = win;
            return;
        }
        if ("topInset" in content)
            content.topInset = titleBarHeight;
        if ("launchArgs" in content)
            content.launchArgs = Qt.binding(() => win.launchArgs);
    }

    function toggleMaximize() {
        if (!maximized)
            restoreRect = Qt.rect(normalX, normalY, normalW, normalH);
        maximized = !maximized;
        // A client that opens maximised has the work area for its size.
        if (awaitingSize) {
            awaitingSize = false;
            manager.arrived(win);
        }
    }

    // ── Geometry springs ────────────────────────────────────────────────────
    // How far the window is into its Overview place: 0 or 1, or an Overview
    // gesture's progress, which the springs then follow exactly.
    readonly property bool overviewTracking: manager !== null && manager.overviewTracking
    readonly property real overviewMix: manager !== null && manager.overviewGesture >= 0 && inOverviewGrid
                                        ? manager.overviewGesture : inOverview ? 1 : 0
    Spring {
        id: gx
        target: win.frameRect.x + (win.overviewRect.x - win.frameRect.x) * win.overviewMix
        enabled: !win.overviewTracking
        preset: Motion.smooth
    }
    Spring {
        id: gy
        target: win.frameRect.y + (win.overviewRect.y - win.frameRect.y) * win.overviewMix
        enabled: !win.overviewTracking
        preset: Motion.smooth
    }
    Spring { id: gw; target: win.frameRect.width; preset: Motion.smooth }
    Spring { id: gh; target: win.frameRect.height; preset: Motion.smooth }
    Spring {
        id: gs
        target: 1 + (win.overviewScale - 1) * win.overviewMix
        enabled: !win.overviewTracking
        preset: Motion.smooth
    }
    // The window's Space on the strip. Moving it with the view carries the
    // window across while the desktop slides underneath.
    Spring { id: gspace; target: win.space; preset: Motion.smooth }
    // Offset while dragged in Overview (towards a Space chip).
    property point overviewDrag: Qt.point(0, 0)
    Spring { id: odx; target: win.overviewDrag.x; preset: Motion.snappy }
    Spring { id: ody; target: win.overviewDrag.y; preset: Motion.snappy }
    Spring {
        id: presence
        value: 0
        target: win.closing || win.minimized || win.awaitingSize ? 0 : 1
        preset: win.closing ? Motion.snappy : Motion.smooth
        onSettled: if (win.closing && value === 0) win.closed()
    }
    // A window closing while out of sight (minimised, or never shown) has
    // nothing to shrink and no spring to settle: it goes at once.
    onClosingChanged: if (closing && presence.value === 0 && !presence.running) Qt.callLater(() => win.closed())

    // Open/minimise fly between `origin` and the frame; close shrinks in place.
    readonly property real t: presence.value
    readonly property bool flies: !closing
    readonly property real visualScale: gs.value * (flies ? 0.06 + 0.94 * t : 0.94 + 0.06 * t)
    readonly property real targetCx: gx.value + gw.value * gs.value / 2
    readonly property real targetCy: gy.value + gh.value * gs.value / 2
    readonly property real originCx: origin.x + origin.width / 2
    readonly property real originCy: origin.y + origin.height / 2
    readonly property real centerX: flies ? originCx + (targetCx - originCx) * t : targetCx
    readonly property real centerY: flies ? originCy + (targetCy - originCy) * t : targetCy

    x: gspace.value * (manager ? manager.width : 0) + centerX - width * visualScale / 2 + odx.value
    y: centerY - height * visualScale / 2 + ody.value
    width: gw.value
    height: gh.value
    scale: visualScale
    transformOrigin: Item.TopLeft
    opacity: flies ? Math.min(1, t * 2.4) : t
    // On the Space in view, or sliding past it. Windows out of view are not
    // drawn at all, and Prism takes their clients off the output so they stop
    // rendering too.
    readonly property bool onScreen: !manager || Math.abs(gspace.value - manager.spaceOffset) < 0.999
    visible: opacity > 0.003 && onScreen

    // ── Interaction helpers ─────────────────────────────────────────────────
    property point dragStart
    property rect dragStartRect

    function beginMove(scenePoint: point) {
        manager.activate(win);
        if (maximized) {
            // Pull the window out of maximised/snapped state under the pointer.
            const w = restoreRect.width > 0 ? restoreRect.width : normalW;
            const h = restoreRect.height > 0 ? restoreRect.height : normalH;
            const local = mapFromItem(null, scenePoint.x, scenePoint.y);
            const ratio = local.x / width;
            maximized = false;
            normalW = w;
            normalH = h;
            normalX = scenePoint.x - w * ratio;
            normalY = scenePoint.y - 18;
            gw.snap(w);
            gh.snap(h);
        }
        dragStart = Qt.point(normalX, normalY);
    }

    function moveBy(translation: vector2d, scenePoint: point) {
        normalX = dragStart.x + translation.x;
        normalY = Math.max(manager.workArea.y - 8, dragStart.y + translation.y);
        gx.snap(normalX);
        gy.snap(normalY);
        const target = manager.snapTargetFor(scenePoint);
        manager.snapVisible = target.width > 0;
        if (target.width > 0)
            manager.snapRect = target;
    }

    // Lands on a new display (WindowLayer.receive): no flight from where it was.
    function arrive() {
        gx.snap(gx.target);
        gy.snap(gy.target);
        gw.snap(gw.target);
        gh.snap(gh.target);
    }

    function endMove(scenePoint: point) {
        // Carried past the edge onto the next display.
        const beyond = manager.displayBeyond(scenePoint);
        if (beyond >= 0) {
            manager.snapVisible = false;
            const grab = Qt.point(scenePoint.x - normalX, scenePoint.y - normalY);
            const from = manager;
            // After the drag's own event delivery has finished.
            Qt.callLater(() => from.moveToDisplay(win, beyond, scenePoint, grab));
            return;
        }
        const target = manager.snapTargetFor(scenePoint);
        manager.snapVisible = false;
        if (target.width <= 0)
            return;
        restoreRect = Qt.rect(normalX, normalY, normalW, normalH);
        if (target.width === manager.workArea.width && target.height === manager.workArea.height) {
            maximized = true;
        } else {
            normalX = target.x;
            normalY = target.y;
            normalW = target.width;
            normalH = target.height;
        }
    }

    // A client with its own decorations asks the compositor to move or resize
    // it when its title bar or border is dragged (xdg_toplevel move/resize).
    // The press belongs to the client, so the frame follows the pointer
    // through `pointerWatch`, with the same springs, snapping and size limits
    // as Prism's own title bar and grips.
    property bool clientMoving: false
    property bool clientResizing: false
    property point clientDragStart

    function beginClientMove() {
        if (!pointerWatch.active)
            return;
        clientDragStart = pointerWatch.point.scenePosition;
        clientMoving = true;
        beginMove(clientDragStart);
    }

    function beginClientResize(edges: int) {
        if (!pointerWatch.active || maximized)
            return;
        clientDragStart = pointerWatch.point.scenePosition;
        clientResizing = true;
        beginResize(edges);
    }

    // Size limits for the content, from the client (xdg_toplevel min/max
    // size); 0 means unconstrained. Resizing keeps the frame within them.
    property size minimumContentSize: Qt.size(0, 0)
    property size maximumContentSize: Qt.size(0, 0)
    // True during an interactive resize, so clients can relayout cheaply.
    property bool resizing: false
    // The edges being dragged (Qt.LeftEdge | Qt.TopEdge | ...).
    property int resizeEdges: Qt.RightEdge | Qt.BottomEdge

    function beginResize(edges: int) {
        manager.activate(win);
        if (maximized) {
            normalX = frameRect.x;
            normalY = frameRect.y;
            normalW = frameRect.width;
            normalH = frameRect.height;
            maximized = false;
        }
        dragStartRect = Qt.rect(normalX, normalY, normalW, normalH);
        resizeEdges = edges;
        resizing = true;
    }

    // Moves the dragged edges by (dx, dy). Left and top edges move the
    // origin too, so the opposite edges stay where they are.
    function resizeBy(dx: real, dy: real) {
        const chrome = isClient && decorated ? titleBarHeight : 0;
        const minW = Math.max(isClient ? 160 : 420, minimumContentSize.width);
        const minH = Math.max(isClient ? 120 : 300, minimumContentSize.height + chrome);
        const maxW = maximumContentSize.width > 0 ? maximumContentSize.width : Infinity;
        const maxH = maximumContentSize.height > 0 ? maximumContentSize.height + chrome : Infinity;
        const r = dragStartRect;
        const clampW = w => Math.min(maxW, Math.max(minW, w));
        const clampH = h => Math.min(maxH, Math.max(minH, h));
        if (resizeEdges & Qt.RightEdge)
            normalW = clampW(r.width + dx);
        if (resizeEdges & Qt.LeftEdge) {
            normalW = clampW(r.width - dx);
            normalX = r.x + r.width - normalW;
        }
        if (resizeEdges & Qt.BottomEdge)
            normalH = clampH(r.height + dy);
        if (resizeEdges & Qt.TopEdge) {
            normalH = clampH(r.height - dy);
            normalY = r.y + r.height - normalH;
        }
        gx.snap(normalX);
        gy.snap(normalY);
        gw.snap(normalW);
        gh.snap(normalH);
    }

    function endResize() {
        resizing = false;
    }

    // ── Frame ───────────────────────────────────────────────────────────────
    Surface {
        anchors.fill: parent
        color: Theme.surface
        radius: Tokens.radius.window
        elevation: win.focused && !win.inOverview ? 5 : 3
        shadowStrength: win.focused ? 1.0 : 0.65
    }

    Item {
        id: body
        anchors.fill: parent
        enabled: !win.inOverview && !win.closing
        layer.enabled: true
        layer.smooth: true
        layer.mipmap: win.visualScale < 0.9
        layer.effect: RoundMask { radius: Tokens.radius.window }

        // Server-side decorated Wayland clients sit below the title bar;
        // first-party apps extend under it for a unified toolbar, and
        // client-side decorated clients fill the whole frame.
        Rectangle {
            visible: win.isClient && win.decorated
            width: parent.width
            height: win.titleBarHeight
            color: Theme.surfaceSunken
        }

        Loader {
            id: appLoader
            objectName: "content"
            anchors.fill: parent
            anchors.topMargin: win.isClient && win.decorated ? win.titleBarHeight : 0
            focus: true
            sourceComponent: win.isClient ? win.manager.surfaceDelegate : AppRegistry.component(win.appId)
            onLoaded: {
                win.configureContent(item);
                if (win.focused)
                    win.focusContent();
            }
        }

        // Title bar, drawn by Prism over the app's unified toolbar area.
        Item {
            id: titleBar
            objectName: "titleBar"
            visible: win.hasTitleBar
            width: parent.width
            height: win.titleBarHeight

            DragHandler {
                id: move
                target: null
                dragThreshold: 2
                onActiveChanged: {
                    if (active)
                        win.beginMove(centroid.scenePressPosition);
                    else
                        win.endMove(centroid.scenePosition);
                }
                onActiveTranslationChanged: if (active) win.moveBy(activeTranslation, centroid.scenePosition)
            }
            TapHandler {
                onTapped: win.manager.activate(win)
                onDoubleTapped: win.toggleMaximize()
            }
            // The window menu.
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: eventPoint => {
                    win.manager.activate(win);
                    windowMenu.popup(titleBar, eventPoint.position);
                }
            }

            Menu {
                id: windowMenu
                objectName: "windowMenu"
                MenuItem {
                    text: qsTr("Minimize")
                    onTriggered: win.manager.minimize(win)
                }
                MenuItem {
                    text: win.maximized ? qsTr("Restore") : qsTr("Maximize")
                    onTriggered: win.toggleMaximize()
                }
                MenuSeparator { }
                MenuItem {
                    objectName: "moveToSpace0"
                    text: qsTr("Move to Space 1")
                    enabled: win.space !== 0
                    onTriggered: win.manager.moveToSpace(win, 0, false)
                }
                MenuItem {
                    objectName: "moveToSpace1"
                    text: qsTr("Move to Space 2")
                    enabled: win.space !== 1
                    onTriggered: win.manager.moveToSpace(win, 1, false)
                }
                MenuItem {
                    objectName: "moveToSpace2"
                    text: qsTr("Move to Space 3")
                    enabled: win.space !== 2
                    onTriggered: win.manager.moveToSpace(win, 2, false)
                }
                // With several displays: move to another.
                Instantiator {
                    model: win.manager && win.manager.displays.length > 1 ? win.manager.displays.length : 0
                    delegate: MenuItem {
                        required property int index
                        objectName: "moveToDisplay" + index
                        text: qsTr("Move to Display %1").arg(index + 1)
                        enabled: index !== win.manager.displayIndex
                        onTriggered: win.manager.moveToDisplay(win, index, null, null)
                    }
                    onObjectAdded: (index, object) => windowMenu.insertItem(6 + index, object)
                    onObjectRemoved: (index, object) => windowMenu.removeItem(object)
                }
                MenuSeparator { }
                MenuItem {
                    text: qsTr("Close")
                    onTriggered: win.manager.close(win, false)
                }
            }

            Text {
                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - 220)
                text: win.title
                textFormat: Text.PlainText
                font: Theme.bodyStrong
                color: win.focused ? Theme.textPrimary : Theme.textTertiary
                elide: Text.ElideRight
            }

            Row {
                id: controls
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                opacity: win.focused || frameHover.hovered ? 1 : 0.55

                IconButton {
                    objectName: "minimize"
                    width: 26
                    height: 26
                    icon: "minus"
                    iconSize: 14
                    onClicked: win.manager.minimize(win)
                }
                IconButton {
                    objectName: "maximize"
                    width: 26
                    height: 26
                    icon: win.maximized ? "minimize-2" : "maximize-2"
                    iconSize: 13
                    onClicked: win.toggleMaximize()
                }
                IconButton {
                    objectName: "close"
                    width: 26
                    height: 26
                    icon: "x"
                    iconSize: 15
                    hoverColor: Theme.critical
                    onClicked: win.manager.close(win, false)
                }
            }
        }
    }

    // Crisp hairline above the content.
    Surface {
        anchors.fill: parent
        color: "transparent"
        strokeColor: win.inOverview && overviewHover.hovered ? Theme.accent : Theme.windowStroke
        strokeWidth: win.inOverview && overviewHover.hovered ? 2 / Math.max(0.2, win.visualScale) : 1
        radius: Tokens.radius.window
    }

    // Watches the pointer over the whole window without taking it from the
    // content: activates the window on any press, and follows the pointer
    // during a client-requested move. Only handlers that leave a mouse press
    // unaccepted may live here. A TapHandler marks it accepted (Qt 6.12,
    // QTBUG-145896), which stops every item underneath (Wayland client
    // surfaces, Qt Quick Controls in apps) from receiving the press.
    Item {
        anchors.fill: parent
        z: 10
        HoverHandler { id: frameHover }
        PointHandler {
            id: pointerWatch
            acceptedButtons: Qt.AllButtons
            property point lastScenePosition
            onPointChanged: {
                if (!active)
                    return;
                lastScenePosition = point.scenePosition;
                const dx = point.scenePosition.x - win.clientDragStart.x;
                const dy = point.scenePosition.y - win.clientDragStart.y;
                if (win.clientMoving)
                    win.moveBy(Qt.vector2d(dx, dy), point.scenePosition);
                else if (win.clientResizing)
                    win.resizeBy(dx, dy);
            }
            onActiveChanged: {
                // Delivery reaches this handler in windows *under* the press
                // too when the item pressed only takes a passive grab (a
                // TapHandler's default): activate only the window on top.
                if (active && !win.focused && win.manager.windowAt(point.scenePressPosition) === win)
                    win.manager.activate(win);
                if (!active && win.clientMoving) {
                    win.clientMoving = false;
                    win.endMove(lastScenePosition);
                }
                if (!active && win.clientResizing) {
                    win.clientResizing = false;
                    win.endResize();
                }
            }
        }
    }

    // Resize grips on every edge and corner. They sit above the content, so a
    // client-side decorated app's own (clipped) border is never needed.
    Repeater {
        model: [
            { edges: Qt.LeftEdge | Qt.TopEdge, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.TopEdge, cursor: Qt.SizeVerCursor },
            { edges: Qt.RightEdge | Qt.TopEdge, cursor: Qt.SizeBDiagCursor },
            { edges: Qt.RightEdge, cursor: Qt.SizeHorCursor },
            { edges: Qt.RightEdge | Qt.BottomEdge, cursor: Qt.SizeFDiagCursor },
            { edges: Qt.BottomEdge, cursor: Qt.SizeVerCursor },
            { edges: Qt.LeftEdge | Qt.BottomEdge, cursor: Qt.SizeBDiagCursor },
            { edges: Qt.LeftEdge, cursor: Qt.SizeHorCursor }
        ]
        delegate: Item {
            id: grip

            required property var modelData
            readonly property int edges: modelData.edges
            readonly property bool horizontal: (edges & (Qt.LeftEdge | Qt.RightEdge)) !== 0
            readonly property bool vertical: (edges & (Qt.TopEdge | Qt.BottomEdge)) !== 0
            readonly property real band: 6
            readonly property real corner: 12

            z: 11
            visible: !win.maximized && !win.inOverview
            x: edges & Qt.LeftEdge ? 0 : edges & Qt.RightEdge ? win.width - width : corner
            y: edges & Qt.TopEdge ? 0 : edges & Qt.BottomEdge ? win.height - height : corner
            width: horizontal && vertical ? corner : horizontal ? band : win.width - 2 * corner
            height: horizontal && vertical ? corner : vertical ? band : win.height - 2 * corner

            HoverHandler { cursorShape: grip.modelData.cursor }
            DragHandler {
                target: null
                onActiveChanged: active ? win.beginResize(grip.edges) : win.endResize()
                onActiveTranslationChanged: if (active) win.resizeBy(activeTranslation.x, activeTranslation.y)
            }
        }
    }

    // Overview: pick a window, or drag it onto a Space chip to move it there.
    Item {
        anchors.fill: parent
        z: 12
        visible: win.inOverview
        HoverHandler { id: overviewHover }
        TapHandler { onTapped: win.manager.pickInOverview(win) }
        DragHandler {
            // The centroid is already reset when the drag ends, so the drop
            // point is remembered on the way.
            property point lastScenePosition
            target: null
            onActiveChanged: {
                if (active) {
                    win.manager.raiseInOverview(win);
                } else {
                    win.manager.dropInOverview(win, lastScenePosition);
                    win.overviewDrag = Qt.point(0, 0);
                }
            }
            onCentroidChanged: {
                if (!active)
                    return;
                lastScenePosition = centroid.scenePosition;
                const dx = centroid.scenePosition.x - centroid.scenePressPosition.x;
                const dy = centroid.scenePosition.y - centroid.scenePressPosition.y;
                win.overviewDrag = Qt.point(dx, dy);
                odx.snap(dx);
                ody.snap(dy);
                win.manager.hoverInOverview(centroid.scenePosition);
            }
        }
    }
}
