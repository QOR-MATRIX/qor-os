// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

// Lumen push button. `highlighted` renders the accent (primary) variant,
// `flat` the borderless variant. `icon.name` takes a Lumen symbol name.
T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: 14
    spacing: 6
    font: Theme.bodyStrong
    hoverEnabled: true

    readonly property color foreground: !enabled ? Theme.textDisabled
                                                 : highlighted ? Theme.onAccent : Theme.textPrimary

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: Math.max(row.implicitHeight, 18)
        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.icon.name.length > 0
                name: control.icon.name
                size: 15
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
        implicitWidth: 72
        implicitHeight: 30
        radius: Tokens.radius.m
        color: control.highlighted
               ? (control.hovered && !control.down ? Theme.accentHover : Theme.accent)
               : control.down ? Theme.fillControlPressed
                              : control.hovered ? Theme.fillControlHover
                                                : control.flat ? "transparent" : Theme.fillControl
        strokeColor: control.visualFocus ? Theme.focusRing
                                         : control.highlighted ? Qt.rgba(1, 1, 1, 0.22) : "transparent"
        strokeWidth: control.visualFocus ? 2 : 1
        highlight: control.highlighted ? 0.8 : 0
        elevation: control.highlighted && control.enabled ? 1 : 0
        opacity: control.enabled ? 1 : 0.6
        scale: press.value

        Spring {
            id: press
            target: control.down ? 0.97 : 1
            preset: Motion.snappy
        }
        Behavior on color { ColorAnimation { duration: 110 } }
    }
}
