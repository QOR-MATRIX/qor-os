// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 5
    horizontalPadding: 14
    font: checked ? Theme.bodyStrong : Theme.body
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.checked ? Theme.textPrimary : Theme.textSecondary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Surface {
        implicitHeight: 26
        radius: Tokens.radius.m - 2
        color: control.checked ? (Theme.darkness > 0.5 ? Qt.lighter(Theme.surfaceRaised, 1.35) : Theme.surfaceRaised)
                               : control.hovered ? Theme.fillControl : "transparent"
        strokeColor: control.visualFocus ? Theme.focusRing : control.checked ? Theme.windowStroke : "transparent"
        strokeWidth: control.visualFocus ? 2 : 1
        elevation: control.checked ? 1 : 0
        shadowStrength: 0.6
    }
}
