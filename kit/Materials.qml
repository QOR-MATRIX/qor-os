// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma Singleton

import QtQuick
import QOR.Kit

// Shared material sources published by the shell.
//
//   desktop – blurred wallpaper only. Use it for in-window Frost (sidebars,
//             inspectors): sampling it from inside a window never recurses.
//   scene   – blurred wallpaper + windows. Shell chrome (Rail, Shelf, Beam)
//             uses it so glass shows the windows underneath.
//
// A texture can only be sampled in the window that renders it, so with
// several displays (Prism) each publishes its own desktop backdrop: use
// desktopFor(Window.window) from inside a window.
QtObject {
    property Backdrop desktop: null
    property Backdrop scene: null
    // [{ window, backdrop }] for displays other than the main one.
    property var displayDesktops: []

    function desktopFor(window: var): Backdrop {
        for (const entry of displayDesktops)
            if (entry.window === window)
                return entry.backdrop;
        return desktop;
    }
}
