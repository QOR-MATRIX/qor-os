// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.Popup {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 12

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: 220; easing.type: Easing.OutBack }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 }
    }

    background: Surface {
        color: Theme.surfaceRaised
        strokeColor: Theme.windowStroke
        radius: Tokens.radius.l
        elevation: 4
        shadowStrength: 0.8
    }

    T.Overlay.modal: Rectangle {
        color: Theme.scrim
    }
}
