// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

// Overlay scroll bar: hidden at rest, fades in while scrolling or hovered.
T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 3
    visible: control.policy !== T.ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    contentItem: Rectangle {
        implicitWidth: control.interactive ? (control.hovered || control.pressed ? 8 : 6) : 3
        implicitHeight: control.interactive ? (control.hovered || control.pressed ? 8 : 6) : 3
        radius: Math.min(width, height) / 2
        color: Theme.textPrimary
        opacity: 0.0

        Behavior on implicitWidth { NumberAnimation { duration: 120 } }
        Behavior on implicitHeight { NumberAnimation { duration: 120 } }

        states: State {
            name: "active"
            when: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0)
            PropertyChanges { control.contentItem.opacity: control.pressed ? 0.5 : 0.32 }
        }

        transitions: Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation { duration: 600 }
                NumberAnimation { target: control.contentItem; duration: 260; property: "opacity"; to: 0.0 }
            }
        }
    }
}
