// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

// Single-line search input with a leading symbol and a clear button.
T.TextField {
    id: control

    property string icon: "search"

    implicitWidth: 220
    implicitHeight: 30
    leftPadding: 32
    rightPadding: clear.visible ? 30 : 10
    verticalAlignment: TextInput.AlignVCenter
    font: Theme.body
    color: Theme.textPrimary
    placeholderTextColor: Theme.textTertiary
    selectionColor: Theme.fillSelected
    selectedTextColor: Theme.textPrimary
    selectByMouse: true

    background: Surface {
        radius: Tokens.radius.m
        color: control.activeFocus ? Theme.surfaceRaised : Theme.fillControl
        strokeColor: control.activeFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
    }

    Text {
        x: control.leftPadding
        width: control.width - control.leftPadding - control.rightPadding
        anchors.verticalCenter: parent.verticalCenter
        visible: control.length === 0 && !control.preeditText
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        elide: Text.ElideRight
    }

    Icon {
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        name: control.icon
        size: 15
        color: Theme.textTertiary
    }

    IconButton {
        id: clear
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        width: 22
        height: 22
        icon: "x"
        iconSize: 13
        visible: control.text.length > 0
        onClicked: control.clear()
    }
}
