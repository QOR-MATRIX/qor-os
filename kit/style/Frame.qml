// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.Frame {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 14
    font: Theme.body

    background: Surface {
        color: Theme.surfaceRaised
        strokeColor: Theme.windowStroke
        radius: Tokens.radius.l
        elevation: 1
        shadowStrength: 0.5
    }
}
