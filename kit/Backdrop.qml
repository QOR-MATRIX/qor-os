// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Effects
import QTOS.Kit

// Produces one shared, blurred, saturated copy of `sourceItem` at a fraction
// of its resolution. Any number of Glass items sample it at their exact
// on-screen rectangle, so the scene is blurred once per frame no matter how
// many glass panels are visible, and only when the source actually changes.
//
// Place a Backdrop *outside* the subtree it captures to avoid recursion.
Item {
    id: root

    property Item sourceItem
    property real downsample: Tokens.material.downsample
    property real blurMax: Tokens.material.blurMax
    property real saturation: Tokens.material.saturation
    // Paused with solid materials (Reduce Transparency or Increase Contrast):
    // Glass falls back to opaque surfaces.
    property bool live: !Theme.solidMaterials

    // The texture provider Glass binds to.
    readonly property Item texture: blurred

    width: sourceItem ? sourceItem.width : 0
    height: sourceItem ? sourceItem.height : 0

    readonly property size _textureSize: Qt.size(Math.max(1, Math.round(width * downsample)),
                                                 Math.max(1, Math.round(height * downsample)))

    // Kept in the scene (opacity 0 rather than visible: false) so it remains a
    // live texture provider for the blur pass without drawing anything itself.
    ShaderEffectSource {
        id: capture
        width: root._textureSize.width
        height: root._textureSize.height
        opacity: 0
        sourceItem: root.sourceItem
        textureSize: root._textureSize
        live: root.live
        hideSource: false
        smooth: true
    }

    MultiEffect {
        id: blur
        width: root._textureSize.width
        height: root._textureSize.height
        source: capture
        autoPaddingEnabled: false
        blurEnabled: true
        blur: 1.0
        // In texture pixels: at downsample 0.25, 40 ≈ a 160 px blur on screen.
        blurMax: Math.round(root.blurMax)
        saturation: root.saturation
    }

    ShaderEffectSource {
        id: blurred
        width: 0
        height: 0
        sourceItem: blur
        textureSize: root._textureSize
        hideSource: true
        live: root.live
        smooth: true
    }
}
