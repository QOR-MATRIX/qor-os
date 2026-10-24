// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// "Aurora", the procedural QTOS wallpaper. Follows the accent hue and the
// appearance spring; animates only when `living` is on.
//
// The noise field is rendered once into a cached layer with a small overscan.
// Zoom and parallax (lock screen, Overview, Space switches) are applied as
// item transforms of that texture, so shell animations never re-run the
// shader — on integrated GPUs that is the difference between a blit and a
// full-screen fractal evaluation every frame.
Item {
    id: root

    property real hue: Theme.hue
    property real darkness: Theme.darkness
    property bool living: Theme.livingWallpaper && !Theme.reduceMotion
    // Offset in units of the wallpaper height (positive moves the view right).
    property vector2d parallax: Qt.vector2d(0, 0)
    property real zoom: 1
    property real intensity: 1
    property real seed: 42
    // Extra field rendered around the visible area to absorb parallax.
    property real overscan: 0.14

    property real _time: seed

    ShaderEffect {
        id: field
        width: root.width * (1 + root.overscan)
        height: root.height * (1 + root.overscan)
        x: -root.width * root.overscan / 2
           - Math.max(-1, Math.min(1, root.parallax.x / (root.overscan / 2))) * root.width * root.overscan / 2
        y: -root.height * root.overscan / 2
        scale: root.zoom
        layer.enabled: true
        layer.smooth: true

        readonly property vector2d resolution: Qt.vector2d(width, height)
        readonly property real time: root._time
        readonly property real hue: root.hue
        readonly property real darkness: root.darkness
        readonly property vector2d parallax: Qt.vector2d(0, 0)
        readonly property real intensity: root.intensity
        readonly property real zoom: 1
        fragmentShader: Qt.resolvedUrl("shaders/wallpaper.frag.qsb")
    }

    FrameAnimation {
        running: root.living && root.visible
        onTriggered: root._time += frameTime
    }
}
