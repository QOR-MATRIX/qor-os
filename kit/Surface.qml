// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// The Lumen base shape. A single shader draws the continuous-curvature fill,
// optional vertical gradient, inner hairline with specular falloff, inner top
// sheen and an analytic soft shadow. Children are drawn on top.
Item {
    id: root

    property color color: Theme.surface
    property color gradientTo: color
    property color strokeColor: "transparent"
    property real strokeWidth: 1
    property real radius: Tokens.radius.l
    property real exponent: Tokens.radius.squircleExponent
    property int elevation: 0
    property color shadowColor: Theme.shadow
    property real shadowStrength: 1
    property real highlight: 0

    readonly property var _elevation: Theme.elevation(elevation)
    readonly property real _pad: Math.ceil(_elevation.blur + Math.abs(_elevation.offsetY) + 1)

    ShaderEffect {
        x: -root._pad
        y: -root._pad
        width: root.width + 2 * root._pad
        height: root.height + 2 * root._pad

        readonly property vector2d effectSize: Qt.vector2d(width, height)
        readonly property vector2d shapeSize: Qt.vector2d(root.width, root.height)
        readonly property real radius: Math.max(0, Math.min(root.radius, root.width / 2, root.height / 2))
        readonly property real exponent: root.exponent
        readonly property vector4d fillTop: Qt.vector4d(root.color.r, root.color.g, root.color.b, root.color.a)
        readonly property vector4d fillBottom: Qt.vector4d(root.gradientTo.r, root.gradientTo.g,
                                                           root.gradientTo.b, root.gradientTo.a)
        readonly property vector4d strokeColor: Qt.vector4d(root.strokeColor.r, root.strokeColor.g,
                                                            root.strokeColor.b, root.strokeColor.a)
        readonly property real strokeWidth: root.strokeWidth
        readonly property vector4d shadowColor: Qt.vector4d(root.shadowColor.r, root.shadowColor.g, root.shadowColor.b,
                                                            root.shadowColor.a * root._elevation.opacity
                                                            * root.shadowStrength)
        readonly property real shadowBlur: root._elevation.blur
        readonly property real shadowOffsetY: root._elevation.offsetY
        readonly property real highlight: root.highlight

        fragmentShader: Qt.resolvedUrl("shaders/surface.frag.qsb")
    }
}
