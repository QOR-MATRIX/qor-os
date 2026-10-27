// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Lumen application icon: a continuous-curvature tile with a perceptual
// two-stop gradient built from one hue, a specular rim and a white symbol.
Item {
    id: root

    property string glyph: "app-window"
    property real hue: Tokens.accentDefaultHue
    property real chroma: 0.16
    property real size: 48
    property int elevation: 2

    implicitWidth: size
    implicitHeight: size

    Surface {
        anchors.fill: parent
        radius: root.size * 0.27
        exponent: 4.6
        color: Oklch.color(0.76, root.chroma * 0.9, root.hue + 14)
        gradientTo: Oklch.color(0.52, root.chroma * 1.08, root.hue - 14)
        strokeColor: Qt.rgba(1, 1, 1, 0.30)
        strokeWidth: Math.max(1, root.size / 48)
        highlight: 1
        elevation: root.elevation
    }

    Icon {
        anchors.centerIn: parent
        name: root.glyph
        size: Math.round(root.size * 0.5)
        color: "white"
    }
}
