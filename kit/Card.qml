// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Raised content container. Children stack vertically inside the padding.
Surface {
    id: root

    default property alias content: column.data
    property real padding: Tokens.spacing.l
    property real spacing: Tokens.spacing.m

    implicitWidth: column.implicitWidth + 2 * padding
    implicitHeight: column.implicitHeight + 2 * padding
    color: Theme.surfaceRaised
    strokeColor: Theme.windowStroke
    radius: Tokens.radius.l
    elevation: 1
    shadowStrength: 0.5

    Column {
        id: column
        x: root.padding
        y: root.padding
        width: root.width - 2 * root.padding
        spacing: root.spacing
    }
}
