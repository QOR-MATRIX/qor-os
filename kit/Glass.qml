// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Lumen Glass material: a translucent panel that shows a blurred, tinted view
// of whatever is behind it (read from a shared Backdrop), finished with grain,
// a specular hairline, an inner sheen and a soft shadow.
//
// Without a backdrop — or with Theme.solidMaterials (Reduce Transparency or
// Increase Contrast) — it degrades to an opaque tinted surface with identical
// geometry.
Item {
    id: root

    property Backdrop backdrop: null
    property color tint: Theme.glassTint
    property color strokeColor: Theme.glassStroke
    property real strokeWidth: 1
    property real radius: Tokens.radius.xl
    property real exponent: Tokens.radius.squircleExponent
    property int elevation: 3
    property color shadowColor: Theme.shadow
    property real shadowStrength: 1
    property real highlight: 1
    property real grain: Tokens.material.grain

    readonly property bool _hasBackdrop: backdrop !== null && backdrop.width > 0 && backdrop.height > 0
                                         && !Theme.solidMaterials
    readonly property var _elevation: Theme.elevation(elevation)
    readonly property real _pad: Math.ceil(_elevation.blur + Math.abs(_elevation.offsetY) + 1)

    SceneRect {
        id: placement
        item: effect
        relativeTo: root._hasBackdrop ? root.backdrop.sourceItem : null
    }

    ShaderEffect {
        id: effect
        visible: root._hasBackdrop
        x: -root._pad
        y: -root._pad
        width: root.width + 2 * root._pad
        height: root.height + 2 * root._pad

        readonly property vector2d effectSize: Qt.vector2d(width, height)
        readonly property vector2d shapeSize: Qt.vector2d(root.width, root.height)
        readonly property real radius: Math.max(0, Math.min(root.radius, root.width / 2, root.height / 2))
        readonly property real exponent: root.exponent
        readonly property vector4d backdropRect: root._hasBackdrop
            ? Qt.vector4d(placement.rect.x / root.backdrop.width, placement.rect.y / root.backdrop.height,
                          placement.rect.width / root.backdrop.width, placement.rect.height / root.backdrop.height)
            : Qt.vector4d(0, 0, 1, 1)
        readonly property real backdropMix: Theme.solidMaterials ? 0 : 1
        readonly property vector4d tint: Qt.vector4d(root.tint.r, root.tint.g, root.tint.b, root.tint.a)
        readonly property real grain: root.grain
        readonly property vector4d strokeColor: Qt.vector4d(root.strokeColor.r, root.strokeColor.g,
                                                            root.strokeColor.b, root.strokeColor.a)
        readonly property real strokeWidth: root.strokeWidth
        readonly property vector4d shadowColor: Qt.vector4d(root.shadowColor.r, root.shadowColor.g, root.shadowColor.b,
                                                            root.shadowColor.a * root._elevation.opacity
                                                            * root.shadowStrength)
        readonly property real shadowBlur: root._elevation.blur
        readonly property real shadowOffsetY: root._elevation.offsetY
        readonly property real highlight: root.highlight
        property variant backdropTex: root._hasBackdrop ? root.backdrop.texture : null

        fragmentShader: Qt.resolvedUrl("shaders/glass.frag.qsb")
    }

    Surface {
        anchors.fill: parent
        visible: !root._hasBackdrop
        color: Oklch.alpha(root.tint, 0.97)
        strokeColor: root.strokeColor
        strokeWidth: root.strokeWidth
        radius: root.radius
        exponent: root.exponent
        elevation: root.elevation
        shadowColor: root.shadowColor
        shadowStrength: root.shadowStrength
        highlight: root.highlight
    }
}
