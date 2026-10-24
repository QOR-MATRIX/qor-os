// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QTOS.Kit

// Lumen toggle: the thumb rides a spring and the track color morphs in OKLab.
T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 4
    spacing: 10
    font: Theme.body
    hoverEnabled: true

    indicator: Item {
        implicitWidth: 38
        implicitHeight: 22
        x: control.text.length > 0
           ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
           : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        Spring {
            id: travel
            target: control.visualPosition
            preset: Motion.snappy
        }

        Surface {
            anchors.fill: parent
            radius: height / 2
            exponent: 2
            color: Oklch.mix(Theme.fillControlPressed, Theme.accent, travel.value)
            strokeColor: control.visualFocus ? Theme.focusRing : "transparent"
            strokeWidth: 2
            opacity: control.enabled ? 1 : 0.5
        }

        Surface {
            width: control.pressed ? 22 : 18
            height: 18
            y: 2
            x: 2 + travel.value * (parent.width - width - 4)
            radius: 9
            exponent: 2
            color: "white"
            elevation: 1
            shadowStrength: 1.2
            Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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
