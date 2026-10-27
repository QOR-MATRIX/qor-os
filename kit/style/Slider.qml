// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    padding: 4
    hoverEnabled: true

    readonly property real handleSize: 18

    handle: Surface {
        x: control.leftPadding + (control.horizontal
                                  ? control.visualPosition * (control.availableWidth - width)
                                  : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal
                                 ? (control.availableHeight - height) / 2
                                 : control.visualPosition * (control.availableHeight - height))
        implicitWidth: control.handleSize
        implicitHeight: control.handleSize
        radius: width / 2
        exponent: 2
        color: "white"
        strokeColor: control.visualFocus ? Theme.focusRing : Qt.rgba(0, 0, 0, 0.06)
        strokeWidth: control.visualFocus ? 2 : 1
        elevation: 2
        shadowStrength: 0.9
        scale: grow.value

        Spring {
            id: grow
            target: control.pressed ? 1.15 : control.hovered ? 1.06 : 1
            preset: Motion.snappy
        }
    }

    background: Item {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)
        implicitWidth: control.horizontal ? 200 : 4
        implicitHeight: control.horizontal ? 4 : 200
        width: control.horizontal ? control.availableWidth : implicitWidth
        height: control.horizontal ? implicitHeight : control.availableHeight

        Surface {
            anchors.fill: parent
            radius: 2
            exponent: 2
            color: Theme.fillControlPressed
        }
        Surface {
            readonly property real fill: control.visualPosition
                                         * ((control.horizontal ? parent.width : parent.height) - control.handleSize)
                                         + control.handleSize / 2
            width: control.horizontal ? fill : parent.width
            height: control.horizontal ? parent.height : parent.height - fill
            y: control.horizontal ? 0 : fill
            radius: 2
            exponent: 2
            color: control.enabled ? Theme.accent : Theme.textDisabled
        }
    }
}
