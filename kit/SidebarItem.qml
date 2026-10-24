// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// A navigation row for app sidebars: symbol, label, optional trailing badge.
Item {
    id: root

    property string icon
    property string text
    property string badge
    property bool selected: false
    property color iconColor: selected ? Theme.accentText : Theme.textSecondary

    signal clicked()

    implicitWidth: 200
    implicitHeight: 30
    activeFocusOnTab: true

    Accessible.role: Accessible.ListItem
    Accessible.name: text
    Accessible.selected: selected
    Keys.onSpacePressed: root.clicked()
    Keys.onReturnPressed: root.clicked()

    Surface {
        anchors.fill: parent
        radius: Tokens.radius.m
        color: root.selected ? Theme.fillSelected : hover.hovered ? Theme.fillControl : "transparent"
        strokeColor: root.activeFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 9

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 16
            color: root.iconColor
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font: root.selected ? Theme.bodyStrong : Theme.body
            color: Theme.textPrimary
        }
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        visible: root.badge.length > 0
        text: root.badge
        font: Theme.footnote
        color: Theme.textTertiary
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked() }
}
