// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import QTOS.Kit
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
            id: window
            width: compositor.options.width ?? 1600
            height: compositor.options.height ?? 1000
            visible: true
            color: "black"
            title: qsTr("QTOS · Prism")

            Desktop {
                id: desktop
                anchors.fill: parent
                options: compositor.options
                focus: true
            }
        }
    }

    // Window content for real clients, plugged into the shell's WindowLayer.
    Component {
        id: clientSurface
        ShellSurfaceItem {
            id: surfaceItem
            property var window: null
            shellSurface: window ? window.shellSurface : null
            autoCreatePopupItems: true
            sizeFollowsSurface: false
            anchors.fill: parent

            function configure() {
                if (window && window.toplevel && width > 0 && height > 0)
                    window.toplevel.sendConfigure(Qt.size(width, height), window.maximized ? [XdgToplevel.MaximizedState] : []);
            }
            onWidthChanged: configureTimer.restart()
            onHeightChanged: configureTimer.restart()
            onSurfaceDestroyed: if (window) window.manager.close(window, true)

            Timer {
                id: configureTimer
                interval: 16
                onTriggered: surfaceItem.configure()
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

    TextInputManager { }
    QtTextInputMethodManager { }
}
