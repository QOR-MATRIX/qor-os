import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// The bar across the top: back to the Library, what is open, and the actions of whatever shows it.
//
// Changed for QOR OS: enter() gives it the keyboard, on its way back, ring showing: what is open does on Return, so a
// controller's A (Return, under Prism) reaches the actions, and its d-pad goes along them.
// Changed for QOR OS: a file's name and title are text, never markup (QOR-0018).
Rectangle {
    id: bar
    property string title
    property string subtitle
    default property alias actions: tools.data
    signal back()

    function enter() {
        backButton.forceActiveFocus(Qt.TabFocusReason)
    }
    height: 64
    gradient: Gradient {
        GradientStop { position: 0; color: Qt.alpha(Theme.void_, 0.92) }
        GradientStop { position: 1; color: "transparent" }
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 12
        IconButton {
            id: backButton
            iconName: "back"
            ToolTip.text: qsTr("Back to the Library (Esc)")
            onClicked: bar.back()
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                Layout.fillWidth: true
                text: bar.title
                textFormat: Text.PlainText
                color: Theme.ink
                font.family: Theme.ui
                font.pixelSize: Theme.sizeHeading
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: bar.subtitle
                textFormat: Text.PlainText
                color: Theme.inkMuted
                font.family: Theme.text
                font.pixelSize: Theme.sizeUi
                elide: Text.ElideRight
            }
        }
        Row {
            id: tools
            spacing: 4
        }
    }
}
