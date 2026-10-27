// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// A Lumen symbol from kit/icons, rasterised at the exact device pixel size
// and tinted on the GPU, so one icon file serves every theme and accent.
//
//   Icon { name: "wifi"; size: 16; color: Theme.textSecondary }
Item {
    id: root

    property string name
    property real size: 16
    property color color: Theme.textPrimary

    implicitWidth: size
    implicitHeight: size

    Image {
        id: raster
        anchors.fill: parent
        visible: false
        smooth: true
        source: root.name.length > 0 ? Qt.resolvedUrl("icons/" + root.name + ".svg") : ""
        sourceSize: Qt.size(Math.ceil(root.width * Screen.devicePixelRatio),
                            Math.ceil(root.height * Screen.devicePixelRatio))
    }

    ShaderEffect {
        anchors.fill: parent
        visible: raster.status === Image.Ready
        property variant source: raster
        readonly property vector4d tint: Qt.vector4d(root.color.r, root.color.g, root.color.b, root.color.a)
        fragmentShader: Qt.resolvedUrl("shaders/icon.frag.qsb")
    }
}
