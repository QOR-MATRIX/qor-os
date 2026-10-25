// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QTOS.Kit

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding)

    padding: 4
    hoverEnabled: true

    readonly property real handleSize: 18

    // `node` is control.first or control.second (a RangeSliderNode, which
    // QML cannot name as a property type).
    component Handle: Surface {
        id: handle
        required property var node
        x: control.leftPadding + (control.horizontal
                                  ? node.visualPosition * (control.availableWidth - width)
                                  : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal
                                 ? (control.availableHeight - height) / 2
                                 : node.visualPosition * (control.availableHeight - height))
        implicitWidth: control.handleSize
        implicitHeight: control.handleSize
        radius: width / 2
        exponent: 2
        color: "white"
        strokeColor: control.visualFocus && node.handle.activeFocus ? Theme.focusRing : Qt.rgba(0, 0, 0, 0.06)
        strokeWidth: control.visualFocus && node.handle.activeFocus ? 2 : 1
        elevation: 2
        shadowStrength: 0.9
        scale: grow.value

        Spring {
            id: grow
            target: handle.node.pressed ? 1.15 : handle.node.hovered ? 1.06 : 1
            preset: Motion.snappy
        }
    }

    first.handle: Handle { node: control.first }
    second.handle: Handle { node: control.second }

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
            readonly property real span: (control.horizontal ? parent.width : parent.height) - control.handleSize
            readonly property real from: control.first.visualPosition * span + control.handleSize / 2
            readonly property real to: control.second.visualPosition * span + control.handleSize / 2
            x: control.horizontal ? Math.min(from, to) : 0
            y: control.horizontal ? 0 : Math.min(from, to)
            width: control.horizontal ? Math.abs(to - from) : parent.width
            height: control.horizontal ? parent.height : Math.abs(to - from)
            radius: 2
            exponent: 2
            color: control.enabled ? Theme.accent : Theme.textDisabled
        }
    }
}
