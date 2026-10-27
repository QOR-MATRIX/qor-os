// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Layer effect that clips content to a continuous-curvature rounded rect.
//
//   Item { layer.enabled: true; layer.effect: RoundMask { radius: 14 } }
ShaderEffect {
    property variant source
    property real radius: Tokens.radius.window
    property real exponent: Tokens.radius.squircleExponent
    readonly property vector2d size: Qt.vector2d(width, height)
    fragmentShader: Qt.resolvedUrl("shaders/roundmask.frag.qsb")
}
