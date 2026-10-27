// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit
import QOR.Input

// The controller interface's pointer: a soft accent ring around a bright
// centre (the hot spot), drawn above everything. It shrinks while pressed.
Item {
    id: cursor

    property ControllerNavigator navigator

    x: navigator ? navigator.cursor.x : 0
    y: navigator ? navigator.cursor.y : 0
    width: 0
    height: 0
    opacity: navigator && navigator.enabled && navigator.cursorVisible ? 1 : 0
    visible: opacity > 0.01
    Behavior on opacity { NumberAnimation { duration: 140 } }

    Spring { id: press; target: cursor.navigator && cursor.navigator.cursorPressed ? 0.78 : 1; preset: Motion.snappy }

    Rectangle {
        x: -width / 2
        y: -height / 2
        width: 34
        height: 34
        radius: width / 2
        scale: press.value
        color: Oklch.alpha(Theme.accent, 0.20)
        border.color: Oklch.alpha(Theme.accent, 0.9)
        border.width: 2
        antialiasing: true
    }
    Rectangle {
        x: -width / 2
        y: -height / 2
        width: 9
        height: 9
        radius: width / 2
        color: "white"
        border.color: Theme.accent
        border.width: 1.5
        antialiasing: true
    }
}
