// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Generative album / media artwork derived from a single hue, used until real
// cover art is available. Deterministic: the same hue always looks the same.
Surface {
    id: root

    property real hue: 280
    property bool glyphVisible: true

    radius: Math.min(width, height) * 0.12
    color: Oklch.color(0.74, 0.15, hue + 28)
    gradientTo: Oklch.color(0.40, 0.15, hue - 26)
    strokeColor: Qt.rgba(1, 1, 1, 0.16)
    highlight: 0.8

    Item {
        anchors.fill: parent
        clip: true

        Surface {
            width: root.width * 0.95
            height: width
            x: root.width * 0.42
            y: -root.height * 0.38
            radius: width / 2
            exponent: 2
            color: Oklch.color(0.86, 0.10, root.hue + 70, 0.30)
        }
        Surface {
            width: root.width * 0.62
            height: width
            x: -root.width * 0.18
            y: root.height * 0.52
            radius: width / 2
            exponent: 2
            color: Oklch.color(0.30, 0.12, root.hue - 60, 0.45)
        }
    }

    Icon {
        anchors.centerIn: parent
        visible: root.glyphVisible && root.width >= 40
        name: "music"
        size: root.width * 0.28
        color: Qt.rgba(1, 1, 1, 0.55)
    }
}
