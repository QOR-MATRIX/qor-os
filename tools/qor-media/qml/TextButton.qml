import QtQuick
import QtQuick.Controls.Basic

// A button with words, in the tool register (ADR-108): Barlow Semi Condensed in tracked capitals, a control's corners,
// every colour from Theme. `primary` is the one action a moment is for: ember, with dark ink on it. Otherwise it is quiet,
// a hairline that lights under the pointer. Tab reaches it, and Return or Space presses it.
//
// Changed for QOR OS: the arrows go on to the next control, or back (Up, Left), as Tab does, so a controller's d-pad,
// which Prism gives a window as the arrows, goes from control to control. And lit, the primary keeps its fill and
// lights its edge: its ink is the theme's on-accent, white on Lumen's accent in QOR OS (QOR-0016).
Button {
    id: control
    property bool primary: false
    readonly property bool lit: control.enabled && (control.hovered || control.visualFocus)

    implicitHeight: 36
    leftPadding: 16
    rightPadding: 16
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : 0.4

    font.family: Theme.ui
    font.weight: Font.DemiBold
    font.pixelSize: Theme.sizeBody
    font.letterSpacing: Theme.sizeBody * Theme.trackingLabel
    font.capitalization: Font.AllUppercase

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.primary ? Theme.inkOnAccent : control.lit ? Theme.ink : Theme.inkBody
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 36
        radius: Theme.radiusControl
        color: control.primary ? Theme.accent
                               : control.down ? Theme.well : control.lit ? Theme.raised : "transparent"
        border.width: 1
        border.color: control.primary ? (control.lit ? Theme.ink : Theme.accentBright)
                                      : control.lit ? Theme.inkMuted : Theme.edge
        Behavior on color { ColorAnimation { duration: Theme.quick } }
        FocusRing { visible: control.visualFocus }
    }

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    Keys.onPressed: (event) => {
        const forward = event.key === Qt.Key_Down || event.key === Qt.Key_Right
        if (event.modifiers !== Qt.NoModifier || !(forward || event.key === Qt.Key_Up || event.key === Qt.Key_Left))
            return
        control.nextItemInFocusChain(forward).forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
        event.accepted = true
    }
}
