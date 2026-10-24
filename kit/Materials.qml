// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma Singleton

import QtQuick
import QTOS.Kit

// Shared material sources published by the shell.
//
//   desktop – blurred wallpaper only. Use it for in-window Frost (sidebars,
//             inspectors): sampling it from inside a window never recurses.
//   scene   – blurred wallpaper + windows. Shell chrome (Rail, Shelf, Beam)
//             uses it so glass shows the windows underneath.
QtObject {
    property Backdrop desktop: null
    property Backdrop scene: null
}
