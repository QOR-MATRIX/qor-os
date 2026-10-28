// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound
//
// The display tools/dev/products.sh runs the products' suites on: a Wayland
// compositor that shows nothing anywhere. Qt's qml tool runs it on the
// offscreen platform, where it composes in memory. It does what a desktop's
// compositor does for a new window and nothing more: the window keeps the size
// it asks for, the newest window has the keyboard, and when it closes the one
// before has it back. Prism sizes and places windows itself, which is the next
// part of Phase 1, so the suites do not run in it by default.
//
//   QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software qml products-display.qml -- <socket name>

// qmllint disable import unresolved-type
// Qt Wayland Compositor declares XdgShell and XdgDecorationManagerV1 through
// C++ templates its qmltypes cannot describe (see prism/compositor/Compositor.qml).
import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell

WaylandCompositor {
    id: display

    // The socket's name, the last argument: a string the compositor takes as bytes.
    socketName: Qt.application.arguments[Qt.application.arguments.length - 1] // qmllint disable incompatible-type

    function closed(index: int) {
        windows.remove(index);
        if (windows.count > 0)
            (stack.itemAt(windows.count - 1) as ShellSurfaceItem)?.takeFocus();
    }

    ListModel {
        id: windows
    }

    WaylandOutput {
        sizeFollowsWindow: true
        window: Window {
            width: 1920
            height: 1080
            visible: true

            Repeater {
                id: stack
                model: windows

                ShellSurfaceItem {
                    id: item

                    required property int index
                    required shellSurface

                    onSurfaceDestroyed: display.closed(index)

                    Connections {
                        target: item.surface
                        function onHasContentChanged() {
                            if (item.surface.hasContent && item.index === windows.count - 1)
                                item.takeFocus();
                        }
                    }
                }
            }
        }
    }

    XdgShell {
        onToplevelCreated: (toplevel, xdgSurface) => windows.append({ shellSurface: xdgSurface })
    }

    // The products draw no decorations of their own, so their surfaces are
    // their windows' content and nothing else.
    XdgDecorationManagerV1 {
        preferredMode: XdgToplevel.ServerSideDecoration
    }
}
