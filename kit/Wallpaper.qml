// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// "Aurora", the procedural QTOS wallpaper. Follows the accent hue, the
// appearance spring and the theme's seed and intensity; animates only when
// `living` is on.
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
    property real intensity: _intensity.value
    property real seed: Theme.wallpaperSeed
    // Extra field rendered around the visible area to absorb parallax.
    property real overscan: 0.14
    // Audio-reactive light (Reactive.energy and Reactive.pulse in the shell):
    // blooms over the cached field, so reacting never re-renders the noise.
    property real energy: 0
    property real pulse: 0

    property real _time: seed
    property real _glowTime: 0
    // A new seed (another theme) restarts the field there, even after the
    // living wallpaper has moved it on.
    onSeedChanged: _time = seed

    Spring {
        id: _intensity
        target: Theme.wallpaperIntensity
        preset: Tokens.motion.smooth
    }

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

    ShaderEffect {
        anchors.fill: parent
        visible: root.energy + root.pulse > 0.002
        readonly property vector2d resolution: Qt.vector2d(width, height)
        readonly property real time: root._glowTime
        readonly property real hue: root.hue
        readonly property real darkness: root.darkness
        readonly property real energy: root.energy
        readonly property real pulse: root.pulse
        fragmentShader: Qt.resolvedUrl("shaders/glow.frag.qsb")
    }

    FrameAnimation {
        running: root.living && root.visible
        onTriggered: root._time += frameTime
    }
    FrameAnimation {
        running: root.energy + root.pulse > 0.002 && root.visible
        onTriggered: root._glowTime += frameTime
    }
}
