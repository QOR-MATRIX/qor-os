// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Input

// Asks, when a controller connects, whether to switch to the controller
// interface. Answer with the controller (A switch, B not now, X or Y "always")
// or with the mouse and keyboard.
Item {
    id: prompt

    property Backdrop backdrop
    property bool open: false
    property var gamepad: ({})
    signal accepted(bool always)
    signal dismissed()

    function ask(pad: var) {
        gamepad = pad;
        always.checked = false;
        open = true;
        timeout.restart();
    }
    function close() {
        open = false;
        timeout.stop();
    }
    // Buttons from the controller while the interface is still off.
    function controllerButton(button: int) {
        if (!open)
            return;
        if (button === Gamepads.South)
            accept();
        else if (button === Gamepads.East)
            dismiss();
        else if (button === Gamepads.West || button === Gamepads.North)
            always.checked = !always.checked;
    }
    function accept() {
        close();
        accepted(always.checked);
    }
    function dismiss() {
        close();
        dismissed();
    }

    width: 420
    height: column.implicitHeight + 40
    visible: presence.value > 0.002
    opacity: presence.value
    scale: 0.94 + 0.06 * presence.value
    transformOrigin: Item.Top

    Spring { id: presence; target: prompt.open ? 1 : 0; preset: Motion.snappy }
    Timer { id: timeout; interval: 20000; onTriggered: prompt.dismiss() }

    Glass {
        anchors.fill: parent
        backdrop: prompt.backdrop
        radius: 24
        elevation: 5
    }

    Column {
        id: column
        x: 20
        y: 20
        width: parent.width - 40
        spacing: 14

        Row {
            spacing: 14
            Surface {
                width: 44
                height: 44
                radius: 22
                exponent: 2
                color: Theme.accent
                highlight: 0.8
                Icon { anchors.centerIn: parent; name: "gamepad-2"; size: 22; color: Theme.onAccent }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: column.width - 58
                spacing: 2
                Text {
                    width: parent.width
                    text: qsTr("%1 connected").arg(prompt.gamepad.name ?? qsTr("Controller"))
                    font: Theme.headline
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: qsTr("Use the controller interface? Steer with the left stick, select with %1 and go back with %2.")
                          .arg(Gamepads.buttonName(Gamepads.South, prompt.gamepad.family ?? "xbox"))
                          .arg(Gamepads.buttonName(Gamepads.East, prompt.gamepad.family ?? "xbox"))
                    font: Theme.footnote
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                }
            }
        }

        Row {
            spacing: 8
            CheckBox {
                id: always
                objectName: "controllerAlways"
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Always switch automatically")
            }
            ControllerGlyph {
                anchors.verticalCenter: parent.verticalCenter
                button: Gamepads.North
                family: prompt.gamepad.family ?? "xbox"
                size: 18
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 10
            Button {
                objectName: "controllerNotNow"
                text: qsTr("Not now")
                onClicked: prompt.dismiss()
                ControllerGlyph {
                    anchors.right: parent.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    button: Gamepads.East
                    family: prompt.gamepad.family ?? "xbox"
                    size: 18
                }
            }
            Item { width: 18; height: 1 }
            Button {
                objectName: "controllerSwitch"
                text: qsTr("Switch")
                highlighted: true
                onClicked: prompt.accept()
                ControllerGlyph {
                    anchors.right: parent.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    button: Gamepads.South
                    family: prompt.gamepad.family ?? "xbox"
                    size: 18
                }
            }
        }
    }
}
