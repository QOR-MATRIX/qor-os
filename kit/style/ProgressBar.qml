// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 4

        Surface {
            width: control.indeterminate ? parent.width * 0.3 : control.visualPosition * parent.width
            height: parent.height
            x: control.indeterminate ? sweep.value * (parent.width - width) : 0
            radius: height / 2
            exponent: 2
            color: Theme.accent
        }

        Spring {
            id: sweep
            preset: Motion.gentle
        }
        Timer {
            interval: 900
            repeat: true
            running: control.indeterminate && control.visible
            onTriggered: sweep.target = sweep.target > 0.5 ? 0 : 1
        }
    }

    background: Surface {
        implicitWidth: 200
        implicitHeight: 4
        y: (control.height - height) / 2
        height: 4
        radius: 2
        exponent: 2
        color: Theme.fillControlPressed
    }
}
