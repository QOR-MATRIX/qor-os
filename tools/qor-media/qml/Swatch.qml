import QtQuick
import QtQuick.Controls.Basic
import QOR.Media

// A colour a mark can be (PdfMarks.colours), as a round button Tab reaches, Return or Space presses, and its name tells.
//
// Changed for QOR OS: the arrows go on to the next control, or back (Up, Left), as Tab does, so a controller's d-pad,
// which Prism gives a window as the arrows, goes from control to control.
Button {
    id: swatch
    required property var modelData
    property bool picked: false
    objectName: "swatch" + modelData.name
    implicitWidth: 32
    implicitHeight: 32
    flat: true
    focusPolicy: Qt.TabFocus
    ToolTip.text: modelData.name
    ToolTip.visible: (hovered || visualFocus) && ToolTip.text.length > 0
    ToolTip.delay: visualFocus ? 0 : 600
    background: Item {
        Rectangle {
            anchors.centerIn: parent
            width: swatch.picked ? 24 : 20
            height: width
            radius: width / 2
            color: swatch.modelData.colour
            border.width: swatch.picked ? 2 : 1
            border.color: swatch.picked ? Theme.ink : Theme.edge
        }
        FocusRing { visible: swatch.visualFocus; radius: width / 2 }
    }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    Keys.onPressed: (event) => {
        const forward = event.key === Qt.Key_Down || event.key === Qt.Key_Right
        if (event.modifiers !== Qt.NoModifier || !(forward || event.key === Qt.Key_Up || event.key === Qt.Key_Left))
            return
        swatch.nextItemInFocusChain(forward).forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
        event.accepted = true
    }
}
