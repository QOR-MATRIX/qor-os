// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Apps

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

    // Set only for real Wayland clients (Prism compositor).
    property var shellSurface: null
    property var toplevel: null
    readonly property bool isClient: shellSurface !== null
    // False for clients that draw their own title bar (client-side
    // decorations): Prism then draws no title bar of its own.
    property bool decorated: true
    readonly property bool hasTitleBar: !isClient || decorated

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
    function adoptContentSize(contentWidth: real, contentHeight: real) {
        if (maximized || inOverview)
            return;
        normalW = contentWidth;
        normalH = contentHeight + (isClient && decorated ? titleBarHeight : 0);
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
    }

    // ── Geometry springs ────────────────────────────────────────────────────
    Spring { id: gx; target: win.inOverview ? win.overviewRect.x : win.frameRect.x; preset: Motion.smooth }
    Spring { id: gy; target: win.inOverview ? win.overviewRect.y : win.frameRect.y; preset: Motion.smooth }
    Spring { id: gw; target: win.frameRect.width; preset: Motion.smooth }
    Spring { id: gh; target: win.frameRect.height; preset: Motion.smooth }
    Spring { id: gs; target: win.inOverview ? win.overviewScale : 1; preset: Motion.smooth }
    Spring {
        id: presence
        value: 0
        target: win.closing || win.minimized ? 0 : 1
        preset: win.closing ? Motion.snappy : Motion.smooth
        onSettled: if (win.closing && value === 0) win.closed()
    }

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

    x: space * (manager ? manager.width : 0) + centerX - width * visualScale / 2
    y: centerY - height * visualScale / 2
    width: gw.value
    height: gh.value
    scale: visualScale
    transformOrigin: Item.TopLeft
    opacity: flies ? Math.min(1, t * 2.4) : t
    visible: opacity > 0.003

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

    function endMove(scenePoint: point) {
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

    // A client with its own title bar asks the compositor to move it when that
    // title bar is dragged. The press belongs to the client, so the frame
    // follows the pointer through `pointerWatch` and moves exactly like a
    // Prism title-bar drag (springs, edge snapping, maximise on top edge).
    property bool clientMoving: false
    property point clientMoveStart

    function beginClientMove() {
        if (!pointerWatch.active)
            return;
        clientMoveStart = pointerWatch.point.scenePosition;
        clientMoving = true;
        beginMove(clientMoveStart);
    }

    function beginResize() {
        manager.activate(win);
        if (maximized) {
            normalX = frameRect.x;
            normalY = frameRect.y;
            normalW = frameRect.width;
            normalH = frameRect.height;
            maximized = false;
        }
        dragStartRect = Qt.rect(normalX, normalY, normalW, normalH);
    }

    function resizeBy(dx: real, dy: real) {
        normalW = Math.max(420, dragStartRect.width + dx);
        normalH = Math.max(300, dragStartRect.height + dy);
        gw.snap(normalW);
        gh.snap(normalH);
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

            Text {
                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - 220)
                text: win.title
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
                    width: 26
                    height: 26
                    icon: "minus"
                    iconSize: 14
                    onClicked: win.manager.minimize(win)
                }
                IconButton {
                    width: 26
                    height: 26
                    icon: win.maximized ? "minimize-2" : "maximize-2"
                    iconSize: 13
                    onClicked: win.toggleMaximize()
                }
                IconButton {
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

    // Activate on any press without stealing it from the content.
    Item {
        anchors.fill: parent
        z: 10
        TapHandler {
            gesturePolicy: TapHandler.DragThreshold
            onPressedChanged: if (pressed && !win.focused) win.manager.activate(win)
        }
        HoverHandler { id: frameHover }
        // Observes (passively) the pointer while a client-requested move runs.
        PointHandler {
            id: pointerWatch
            property point lastScenePosition
            onPointChanged: {
                if (!active)
                    return;
                lastScenePosition = point.scenePosition;
                if (win.clientMoving)
                    win.moveBy(Qt.vector2d(point.scenePosition.x - win.clientMoveStart.x,
                                           point.scenePosition.y - win.clientMoveStart.y), point.scenePosition);
            }
            onActiveChanged: {
                if (!active && win.clientMoving) {
                    win.clientMoving = false;
                    win.endMove(lastScenePosition);
                }
            }
        }
    }

    // Resize grips (bottom, right, corner).
    Item {
        z: 11
        visible: !win.maximized && !win.inOverview
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 18
        height: 18
        HoverHandler { cursorShape: Qt.SizeFDiagCursor }
        DragHandler {
            target: null
            onActiveChanged: if (active) win.beginResize()
            onActiveTranslationChanged: if (active) win.resizeBy(activeTranslation.x, activeTranslation.y)
        }
    }
    Item {
        z: 11
        visible: !win.maximized && !win.inOverview
        anchors.right: parent.right
        y: win.titleBarHeight
        width: 6
        height: parent.height - y - 18
        HoverHandler { cursorShape: Qt.SizeHorCursor }
        DragHandler {
            target: null
            onActiveChanged: if (active) win.beginResize()
            onActiveTranslationChanged: if (active) win.resizeBy(activeTranslation.x, 0)
        }
    }
    Item {
        z: 11
        visible: !win.maximized && !win.inOverview
        anchors.bottom: parent.bottom
        width: parent.width - 18
        height: 6
        HoverHandler { cursorShape: Qt.SizeVerCursor }
        DragHandler {
            target: null
            onActiveChanged: if (active) win.beginResize()
            onActiveTranslationChanged: if (active) win.resizeBy(0, activeTranslation.y)
        }
    }

    // Overview picking.
    Item {
        anchors.fill: parent
        z: 12
        visible: win.inOverview
        HoverHandler { id: overviewHover }
        TapHandler { onTapped: win.manager.pickInOverview(win) }
    }
}
