// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    padding: 6
    leftPadding: 10
    rightPadding: 10
    verticalAlignment: TextInput.AlignVCenter
    font: Theme.body
    color: enabled ? Theme.textPrimary : Theme.textDisabled
    placeholderTextColor: Theme.textTertiary
    selectionColor: Theme.fillSelected
    selectedTextColor: Theme.textPrimary
    selectByMouse: true

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Surface {
        implicitWidth: 200
        implicitHeight: 30
        radius: Tokens.radius.m
        color: control.activeFocus ? Theme.surfaceRaised : Theme.fillControl
        strokeColor: control.activeFocus ? Theme.focusRing : Theme.windowStroke
        strokeWidth: control.activeFocus ? 2 : 1
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}
