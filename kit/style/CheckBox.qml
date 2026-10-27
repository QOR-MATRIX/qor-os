// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 4
    spacing: 8
    font: Theme.body
    hoverEnabled: true

    readonly property bool on: checkState !== Qt.Unchecked

    indicator: Surface {
        implicitWidth: 17
        implicitHeight: 17
        x: control.text.length > 0
           ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
           : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: 5
        color: control.on ? (control.hovered ? Theme.accentHover : Theme.accent)
                          : control.hovered ? Theme.fillControlHover : Theme.fillControl
        strokeColor: control.visualFocus ? Theme.focusRing
                                         : control.on ? Qt.rgba(1, 1, 1, 0.2) : Theme.windowStroke
        strokeWidth: control.visualFocus ? 2 : 1
        highlight: control.on ? 0.6 : 0
        scale: pop.value
        opacity: control.enabled ? 1 : 0.5

        Spring {
            id: pop
            target: control.pressed ? 0.88 : 1
            preset: Motion.bouncy
        }
        Icon {
            anchors.centerIn: parent
            visible: control.on
            name: control.checkState === Qt.PartiallyChecked ? "minus" : "check"
            size: 13
            color: Theme.onAccent
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: control.enabled ? Theme.textPrimary : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
