// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
// Qt Wayland Compositor declares XdgShell, XdgDecorationManagerV1 and the
// text-input managers through C++ templates (QWaylandShellTemplate<T>) that
// its qmltypes cannot describe, so qmllint can neither resolve those types
// nor see that XdgSurface is a ShellSurface. Only those checks are relaxed.
// qmllint disable import unresolved-type
import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import QtWayland.Compositor.PresentationTime
import QTOS.Shell

// Prism: the QTOS Wayland compositor. The shell and every client window share
// one Qt Quick scene graph — client surfaces are ShellSurfaceItems inside the
// same AppWindow frames the simulator uses, so Lumen materials, springs and
// Overview apply to real apps unchanged.
//
// Status: experimental. Runs nested (inside another Wayland/X11 session or
// WSLg) today; the bare-metal KMS backend is a Phase 2 deliverable.
WaylandCompositor {
    id: compositor

    property var options: ({})

    WaylandOutput {
        id: output
        sizeFollowsWindow: true
        window: Window {
            id: outputWindow
            width: compositor.options.width ?? 1600
            height: compositor.options.height ?? 1000
            visible: true
            color: "black"
            title: qsTr("QTOS · Prism")

            Desktop {
                id: desktop
                objectName: "desktop"
                anchors.fill: parent
                options: compositor.options
                focus: true
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
            property size configuredSize: Qt.size(0, 0)

            function configure() {
                if (!toplevel || area.width <= 0 || area.height <= 0)
                    return;
                const states = [];
                if (activated)
                    states.push(XdgToplevel.ActivatedState);
                if (maximized)
                    states.push(XdgToplevel.MaximizedState);
                if (resizing)
                    states.push(XdgToplevel.ResizingState);
                configuredSize = Qt.size(Math.round(area.width), Math.round(area.height));
                toplevel.sendConfigure(configuredSize, states);
            }
            onAreaChanged: configureTimer.restart()
            onActivatedChanged: configureTimer.restart()
            onMaximizedChanged: configureTimer.restart()
            onResizingChanged: configureTimer.restart()
            // The client chose another size than configured (e.g. a terminal
            // snapping to whole cells, or its preferred size on first map).
            onGeometryChanged: {
                if (content.window && geometry.width > 0 && geometry.height > 0
                        && (geometry.width !== configuredSize.width || geometry.height !== configuredSize.height))
                    content.window.adoptContentSize(geometry.width, geometry.height);
            }

            ShellSurfaceItem {
                id: surfaceItem
                focus: true
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

    Component.onCompleted: desktop.windowLayer.surfaceDelegate = clientSurface

    XdgShell {
        onToplevelCreated: (toplevel, xdgSurface) => desktop.adoptSurface(xdgSurface, toplevel)
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
        // A trusted request focuses the window; any other only asks for attention.
        onActivationRequested: (surface, trusted) => {
            const window = desktop.windowLayer.windows.find(w => w.shellSurface && w.shellSurface.surface === surface);
            if (!window)
                return;
            if (trusted)
                desktop.windowLayer.raise(window);
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
