// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// The QTOS mark: an accent-gradient tile holding a ring with an offset tail,
// a stylised "Q" that also reads as a lens.
Item {
    id: root

    property real size: 20
    property real hue: Theme.hue

    implicitWidth: size
    implicitHeight: size

    Surface {
        anchors.fill: parent
        radius: root.size * 0.3
        exponent: 4.4
        color: Oklch.color(0.74, 0.16, root.hue + 40)
        gradientTo: Oklch.color(0.52, 0.19, root.hue - 20)
        strokeColor: Qt.rgba(1, 1, 1, 0.3)
        highlight: 1
    }

    Surface {
        anchors.centerIn: parent
        width: root.size * 0.5
        height: width
        radius: width / 2
        exponent: 2
        color: "transparent"
        strokeColor: "white"
        strokeWidth: Math.max(1.4, root.size * 0.1)
    }

    Surface {
        x: root.size * 0.62
        y: root.size * 0.62
        width: root.size * 0.16
        height: width
        radius: width / 2
        exponent: 2
        color: "white"
    }
}
