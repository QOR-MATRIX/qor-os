// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: 10
    spacing: 9
    font: Theme.body
    hoverEnabled: true

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: Math.max(row.implicitHeight, 18)
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.icon.name.length > 0
                name: control.icon.name
                size: 16
                color: control.highlighted ? Theme.accentText : Theme.textSecondary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                // What a list shows often comes from outside: a network's name, a file's. Text, never markup.
                textFormat: Text.PlainText
                font: control.font
                color: control.enabled ? Theme.textPrimary : Theme.textDisabled
                elide: Text.ElideRight
            }
        }
    }

    background: Surface {
        implicitWidth: 120
        implicitHeight: 32
        radius: Tokens.radius.m
        color: control.highlighted ? Theme.fillSelected
                                   : control.down ? Theme.fillControlPressed
                                                  : control.hovered ? Theme.fillControl : "transparent"
        strokeColor: control.visualFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
        Behavior on color { ColorAnimation { duration: 110 } }
    }
}
