// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QTOS.Kit

T.RadioButton {
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

    indicator: Surface {
        implicitWidth: 17
        implicitHeight: 17
        x: control.text.length > 0
           ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
           : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: width / 2
        exponent: 2
        color: control.checked ? Theme.accent : control.hovered ? Theme.fillControlHover : Theme.fillControl
        strokeColor: control.visualFocus ? Theme.focusRing
                                         : control.checked ? Qt.rgba(1, 1, 1, 0.2) : Theme.windowStroke
        strokeWidth: control.visualFocus ? 2 : 1
        opacity: control.enabled ? 1 : 0.5

        Surface {
            anchors.centerIn: parent
            width: 7
            height: 7
            radius: 3.5
            exponent: 2
            color: Theme.onAccent
            scale: dot.value
            Spring {
                id: dot
                target: control.checked ? 1 : 0
                preset: Motion.bouncy
            }
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
