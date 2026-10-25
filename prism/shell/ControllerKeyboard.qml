// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Input

// The controller interface's on-screen keyboard. It opens when a text field
// takes focus (and on the left stick's click), and types into the focused
// field through the navigator. D-pad or left stick: choose a key; South:
// type it; West: delete; North: space; Start: enter; shoulders: move the text
// cursor; left trigger: shift; East: close.
Item {
    id: keyboard

    property Backdrop backdrop
    property ControllerNavigator navigator
    property bool open: false
    property bool shift: false
    property int row: 1
    property int column: 0

    readonly property var rows: [
        ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
        ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p"],
        ["a", "s", "d", "f", "g", "h", "j", "k", "l", "'"],
        ["shift", "z", "x", "c", "v", "b", "n", "m", ",", "."],
        ["-", "@", "space", "backspace", "enter"]
    ]

    function move(dx: int, dy: int) {
        row = (row + dy + rows.length) % rows.length;
        const count = rows[row].length;
        column = dy !== 0 ? Math.min(column, count - 1) : (column + dx + count) % count;
        navigator.feedback(0.08, 15);
    }
    function press(key: string) {
        switch (key) {
        case "shift": shift = !shift; return;
        case "space": navigator.typeText(" "); break;
        case "backspace": navigator.sendKey(Qt.Key_Backspace); break;
        case "enter": navigator.sendKey(Qt.Key_Return); break;
        default:
            navigator.typeText(shift ? key.toUpperCase() : key);
            shift = false;
        }
        navigator.feedback(0.15, 20);
    }
    function button(button: int) {
        switch (button) {
        case Gamepads.South: press(rows[row][column]); break;
        case Gamepads.West: press("backspace"); break;
        case Gamepads.North: press("space"); break;
        case Gamepads.Start: press("enter"); break;
        case Gamepads.LeftShoulder: navigator.sendKey(Qt.Key_Left); break;
        case Gamepads.RightShoulder: navigator.sendKey(Qt.Key_Right); break;
        case Gamepads.LeftTrigger: shift = !shift; break;
        case Gamepads.East: open = false; break;
        }
    }

    width: 760
    height: grid.implicitHeight + 28
    visible: presence.value > 0.002
    opacity: presence.value
    transform: Translate { y: (1 - presence.value) * 40 }
    Spring { id: presence; target: keyboard.open ? 1 : 0; preset: Motion.snappy }

    Glass {
        anchors.fill: parent
        backdrop: keyboard.backdrop
        radius: 24
        elevation: 5
    }

    Column {
        id: grid
        anchors.centerIn: parent
        spacing: 6
        Repeater {
            model: keyboard.rows
            delegate: Row {
                id: keyRow
                required property var modelData
                required property int index
                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                spacing: 6
                Repeater {
                    model: keyRow.modelData
                    delegate: Surface {
                        id: key
                        required property string modelData
                        required property int index
                        readonly property bool selected: keyboard.row === keyRow.index && keyboard.column === index
                        readonly property bool wide: modelData === "space"
                        readonly property bool special: modelData.length > 1
                        width: wide ? 250 : special ? 104 : 64
                        height: 46
                        radius: Tokens.radius.m
                        color: selected ? Theme.accent
                                        : modelData === "shift" && keyboard.shift ? Oklch.alpha(Theme.accent, 0.35)
                                                                                 : Theme.fillControl
                        strokeColor: selected ? "transparent" : Theme.windowStroke
                        scale: selected ? 1.06 : 1
                        Behavior on scale { NumberAnimation { duration: 90 } }
                        Text {
                            anchors.centerIn: parent
                            text: {
                                switch (key.modelData) {
                                case "shift": return qsTr("Shift");
                                case "space": return qsTr("Space");
                                case "backspace": return qsTr("Delete");
                                case "enter": return qsTr("Enter");
                                }
                                return keyboard.shift ? key.modelData.toUpperCase() : key.modelData;
                            }
                            font: key.special ? Theme.footnote : Theme.headline
                            color: key.selected ? Theme.onAccent : Theme.textPrimary
                        }
                        TapHandler {
                            onTapped: {
                                keyboard.row = keyRow.index;
                                keyboard.column = key.index;
                                keyboard.press(key.modelData);
                            }
                        }
                    }
                }
            }
        }
    }
}
