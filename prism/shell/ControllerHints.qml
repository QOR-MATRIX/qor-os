// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Input

// What the controller's buttons do right now, with the glyphs printed on the
// controller in use. Fades after a while without input and returns with it.
Item {
    id: hints

    property Backdrop backdrop
    property bool active: false
    // [{ button: Gamepads.South, label: "Select", text: "" }]
    property var entries: []

    width: row.implicitWidth + 28
    height: 38
    opacity: active && awake ? 1 : 0
    visible: opacity > 0.01
    Behavior on opacity { NumberAnimation { duration: 260 } }

    property bool awake: true
    function wake() {
        awake = true;
        idle.restart();
    }
    Timer { id: idle; interval: 7000; onTriggered: hints.awake = false }
    onActiveChanged: if (active) wake()
    onEntriesChanged: wake()
    Connections {
        target: Gamepads
        function onPressed() { hints.wake(); }
    }

    Glass {
        anchors.fill: parent
        backdrop: hints.backdrop
        radius: height / 2
        elevation: 3
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 14
        Repeater {
            model: hints.entries
            delegate: Row {
                id: entry
                required property var modelData
                spacing: 6
                ControllerGlyph {
                    anchors.verticalCenter: parent.verticalCenter
                    button: entry.modelData.button
                    text: entry.modelData.text ?? ""
                    size: 20
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.modelData.label
                    font: Theme.footnote
                    color: Theme.textSecondary
                }
            }
        }
    }
}
