// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QTOS.Kit

T.Pane {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 12
    font: Theme.body

    background: Rectangle {
        color: Theme.surface
    }
}
