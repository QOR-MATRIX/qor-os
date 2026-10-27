// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// A keycap used to display keyboard shortcuts.
Surface {
    id: root

    property string text

    implicitWidth: Math.max(20, label.implicitWidth + 10)
    implicitHeight: 20
    radius: Tokens.radius.s
    color: Theme.fillControl
    strokeColor: Theme.windowStroke

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        font: Theme.caption
        color: Theme.textSecondary
    }
}
