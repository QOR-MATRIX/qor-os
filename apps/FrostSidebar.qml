// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// App sidebar in Lumen Frost: the desktop wallpaper, blurred and tinted,
// showing through the window. Children stack vertically below the title bar.
Item {
    id: sidebar

    default property alias content: column.data
    property real topInset: 40
    property real spacing: 2

    Glass {
        anchors.fill: parent
        backdrop: Materials.desktop
        tint: Theme.frostTint
        radius: 0
        elevation: 0
        strokeWidth: 0
        highlight: 0
    }

    Divider {
        vertical: true
        anchors.right: parent.right
        height: parent.height
    }

    Column {
        id: column
        x: 10
        y: sidebar.topInset + 8
        width: parent.width - 20
        spacing: sidebar.spacing
    }
}
