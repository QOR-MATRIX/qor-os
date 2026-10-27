// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

// Borderless toolbar button; icon-only when `text` is empty.
T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: text.length > 0 ? 10 : 6
    spacing: 6
    font: Theme.body
    hoverEnabled: true

    readonly property color foreground: !enabled ? Theme.textDisabled
                                                 : checked ? Theme.accentText
                                                           : hovered ? Theme.textPrimary : Theme.textSecondary

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: Math.max(row.implicitHeight, 16)
        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.icon.name.length > 0
                name: control.icon.name
                size: 16
                color: control.foreground
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.text.length > 0
                text: control.text
                font: control.font
                color: control.foreground
            }
        }
    }

    background: Surface {
        implicitWidth: 28
        implicitHeight: 28
        radius: Tokens.radius.m
        color: control.checked ? Theme.fillSelected
                               : control.down ? Theme.fillControlPressed
                                              : control.hovered ? Theme.fillControlHover : "transparent"
        strokeColor: control.visualFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
        Behavior on color { ColorAnimation { duration: 110 } }
    }
}
