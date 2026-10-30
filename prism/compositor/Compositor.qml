// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound
// Qt Wayland Compositor declares XdgShell, XdgDecorationManagerV1 and the
// text-input managers through C++ templates (QWaylandShellTemplate<T>) that
// its qmltypes cannot describe, so qmllint can neither resolve those types
// nor see that XdgSurface is a ShellSurface. Only those checks are relaxed.
// qmllint disable import unresolved-type
import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import QtWayland.Compositor.PresentationTime
import QOR.Shell

// Prism: the QOR OS Wayland compositor. The shell and every client window share
// one Qt Quick scene graph — client surfaces are ShellSurfaceItems inside the
// same AppWindow frames the simulator uses, so Lumen materials, springs and
// Overview apply to real apps unchanged.
//
// Outputs: one per display, left to right (`options.outputs`). Nested, each
// is a window of the host; on bare metal (eglfs), each is a screen. The
// first hosts the shell's Desktop; the others a DisplayDesktop.
//
// Status: experimental. Runs nested (inside another Wayland/X11 session or
// WSLg) and on Qt's eglfs (KMS) platform; Prism's own KMS backend is a Phase 2
// deliverable.
WaylandCompositor {
    id: prism

    property var options: ({})
    // [{ width, height, screen?, fullScreen? }]
    readonly property var outputs: options.outputs ?? [{ width: options.width ?? 1600, height: options.height ?? 1000 }]

    function outputX(index: int): real {
        let x = 0;
        for (let i = 0; i < index; ++i)
            x += outputs[i].width;
        return x;
    }

    // A fullscreen output on its screen, or a window of the host.
    function show(window: Window, spec: var) {
        if (spec.fullScreen && spec.screen !== undefined)
            window.screen = Application.screens[spec.screen];
        window.visibility = spec.fullScreen ? Window.FullScreen : Window.Windowed;
    }

    WaylandOutput {
        id: output
        sizeFollowsWindow: true
        position: Qt.point(0, 0)
        window: Window {
            id: outputWindow
            width: prism.outputs[0].width
            height: prism.outputs[0].height
            color: "black"
            title: qsTr("QOR OS · Prism")
            Component.onCompleted: prism.show(outputWindow, prism.outputs[0])

            Desktop {
                id: desktop
                objectName: "desktop"
                anchors.fill: parent
                options: prism.options
                focus: true
            }
        }
    }

    Instantiator {
        model: prism.outputs.length - 1
        delegate: WaylandOutput {
            id: extra
            required property int index
            readonly property var spec: prism.outputs[index + 1]
            // Instantiator parents its objects too late for the output to
            // find its compositor by itself.
            compositor: prism
            sizeFollowsWindow: true
            position: Qt.point(prism.outputX(index + 1), 0)
            window: Window {
                id: displayWindow
                width: extra.spec.width
                height: extra.spec.height
                color: "black"
                title: qsTr("QOR OS · Prism · Display %1").arg(extra.index + 2)
                Component.onCompleted: prism.show(displayWindow, extra.spec)

                DisplayDesktop {
                    objectName: "display" + (extra.index + 1)
                    anchors.fill: parent
                    primary: desktop
                    focus: true
                }
                PresentationFeedback {
                    presentationTime: presentation // qmllint disable incompatible-type
                    window: displayWindow
                }
            }
        }
    }

    // Window content for real clients, plugged into the shell's WindowLayer.
    //
    // The Loader in AppWindow sizes this FocusScope to the content area. The
    // surface item inside keeps the surface's own size and is offset so that
    // the client's window geometry fills the area: client-side decorated apps
    // draw shadows outside their window geometry, which must not be scaled in.
    Component {
        id: clientSurface
        FocusScope {
            id: content

            property var window: null
            readonly property XdgSurface xdgSurface: content.window ? content.window.shellSurface : null
            readonly property XdgToplevel toplevel: content.window ? content.window.toplevel : null
            readonly property rect geometry: xdgSurface ? xdgSurface.windowGeometry : Qt.rect(0, 0, 0, 0)
            // Configure for where the frame is going, not where its spring is
            // now, so clients lay out once per resize rather than every frame.
            readonly property size area: content.window ? content.window.contentTargetSize : Qt.size(0, 0)
            readonly property bool activated: content.window ? content.window.focused : false
            readonly property bool maximized: content.window ? content.window.maximized : false
            readonly property bool resizing: content.window ? content.window.resizing : false
            readonly property bool awaitingSize: content.window ? content.window.awaitingSize : false
            property size configuredSize: Qt.size(0, 0)

            // A window awaiting its size is configured with none (0 x 0): the
            // client chooses, and its first commit says what it chose.
            function configure() {
                configureTimer.stop();
                if (!toplevel || area.width <= 0 || area.height <= 0)
                    return;
                const states = [];
                if (activated)
                    states.push(XdgToplevel.ActivatedState);
                if (maximized)
                    states.push(XdgToplevel.MaximizedState);
                if (resizing)
                    states.push(XdgToplevel.ResizingState);
                configuredSize = awaitingSize && !maximized ? Qt.size(0, 0)
                                                            : Qt.size(Math.round(area.width), Math.round(area.height));
                toplevel.sendConfigure(configuredSize, states);
            }
            onAreaChanged: configureTimer.restart()
            onActivatedChanged: configureTimer.restart()
            onMaximizedChanged: configureTimer.restart()
            onResizingChanged: configureTimer.restart()
            // Planned around the size the client chose: configured at once, so
            // that a commit at the size it chose is not taken for another
            // choice before the client has seen the plan.
            onAwaitingSizeChanged: if (!awaitingSize) configure()
            // The client chose another size than configured (e.g. a terminal
            // snapping to whole cells, or its preferred size on first map).
            // Only a commit that answers the latest configure says so: while
            // a resize is under way, a client that keeps drawing still commits
            // sizes configured earlier. Checked on every commit, since the
            // answer may be the size the client already had. A window
            // awaiting its size has only been configured with none, so any
            // commit says what the client chose: it need not have answered
            // the latest configure, which a client that answers only when it
            // redraws, on a frame callback, does not do for an unseen window.
            function adoptClientSize() {
                if (content.window && geometry.width > 0 && geometry.height > 0
                        && (geometry.width !== configuredSize.width || geometry.height !== configuredSize.height)
                        && (awaitingSize || WindowManagement.configureAcked(toplevel)))
                    content.window.adoptContentSize(geometry.width, geometry.height);
            }
            onGeometryChanged: adoptClientSize()

            // A window out of view (another Space, minimised, behind the lock
            // screen) takes its client off the output: wl_surface.leave, and no
            // frame callbacks, so the client stops drawing until it returns.
            readonly property bool shown: content.window !== null && content.window.visible
            function updateOutput() {
                // The output of the display the window is on.
                const window = surfaceItem.Window.window;
                surfaceItem.output = shown && surfaceItem.compositor && window
                    ? surfaceItem.compositor.outputFor(window) : null;
            }
            onShownChanged: updateOutput()
            Connections {
                target: surfaceItem.surface
                function onRedraw() {
                    if (!content.window)
                        return;
                    content.window.commits++; // for tests
                    content.adoptClientSize();
                }
            }

            ShellSurfaceItem {
                id: surfaceItem
                focus: true
                // Qt reassigns the output when the surface or window changes.
                onSurfaceChanged: Qt.callLater(content.updateOutput)
                onWindowChanged: Qt.callLater(content.updateOutput)
                shellSurface: content.xdgSurface // qmllint disable incompatible-type
                autoCreatePopupItems: true
                x: -content.geometry.x
                y: -content.geometry.y
                onSurfaceDestroyed: if (content.window) content.window.manager.close(content.window, true)
                // Prism, not Qt's built-in integration, handles move, resize,
                // maximise and fullscreen requests.
                onShellSurfaceChanged: WindowManagement.takeOver(surfaceItem)
                Component.onCompleted: WindowManagement.takeOver(surfaceItem)
            }

            // The client's size limits bound interactive resizing.
            Binding {
                when: content.window !== null && content.toplevel !== null
                target: content.window
                property: "minimumContentSize"
                value: content.toplevel?.minSize ?? Qt.size(0, 0)
            }
            Binding {
                when: content.window !== null && content.toplevel !== null
                target: content.window
                property: "maximumContentSize"
                value: content.toplevel?.maxSize ?? Qt.size(0, 0)
            }

            // Clients that keep their own title bar get no Prism title bar.
            Binding {
                when: content.window !== null && content.toplevel !== null
                target: content.window
                property: "decorated"
                value: content.toplevel?.decorationMode === XdgToplevel.ServerSideDecoration
            }

            // Window-management requests from the client.
            Connections {
                target: content.toplevel
                function onStartMove() { content.window.beginClientMove(); }
                function onStartResize(seat, edges) { content.window.beginClientResize(edges); }
                function onSetMaximized() { if (!content.window.maximized) content.window.toggleMaximize(); }
                function onUnsetMaximized() { if (content.window.maximized) content.window.toggleMaximize(); }
                function onSetMinimized() { content.window.manager.minimize(content.window); }
            }

            Timer {
                id: configureTimer
                interval: 16
                onTriggered: content.configure()
            }
        }
    }

    Component.onCompleted: {
        desktop.windowLayer.surfaceDelegate = clientSurface;
        // A window Prism opens (a product it starts) may take focus.
        desktop.activationToken = () => activation.createToken();
    }

    XdgShell {
        // A product Prism started is known by its process, never by what its
        // toplevel says it is.
        onToplevelCreated: (toplevel, xdgSurface) => desktop.adoptSurface(
                               xdgSurface, toplevel, ProductHost.productOf(xdgSurface.surface.client.processId))
    }

    XdgDecorationManagerV1 {
        preferredMode: XdgToplevel.ServerSideDecoration
    }

    // ── Protocol extensions (see prism/README.md for the full list) ─────────
    TextInputManager { }
    TextInputManagerV3 { }
    QtTextInputMethodManager { }
    IdleInhibitManagerV1 { }
    FractionalScaleManagerV1 {
        preferredScale: desktop.Screen.devicePixelRatio
    }
    CursorShapeManagerV1 { }
    XdgActivationV1 {
        id: activation
        // A trusted request focuses the window; any other only asks for attention.
        onActivationRequested: (surface, trusted) => {
            const window = desktop.allWindows().find(w => w.shellSurface && w.shellSurface.surface === surface);
            if (!window)
                return;
            if (trusted)
                window.manager.raise(window);
            else if (!window.focused)
                window.demandsAttention = true;
        }
    }
    PresentationTime { id: presentation }
    PresentationFeedback {
        presentationTime: presentation // qmllint disable incompatible-type
        window: outputWindow
    }
}
