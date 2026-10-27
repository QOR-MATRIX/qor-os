// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma Singleton

import QtQuick

// The audio-reactive interface's one source of motion. qor-audiod analyses
// what is being heard (Services.audio's `analysis` signal, ~60 Hz); this turns
// it into values the interface animates with, smoothed at the display's frame
// rate:
//
//   energy    0..1  energy in the frequency range chosen in Settings, scaled
//                   by the chosen intensity. Drives glows and swells.
//   pulse     0..1  jumps on every beat in that range, then decays (~220 ms).
//   level     0..1  overall loudness.
//   spectrum        48 bands, 30 Hz..16 kHz, 0..1, for visualisers.
//
// `energy` and `pulse` are 0 unless Settings > Sound > Audio-reactive is on,
// and with Reduce Motion. Visualisers (`spectrum`) follow playback either way.
QtObject {
    id: root

    readonly property bool enabled: Services.connected && Services.settings.audioReactive
                                    && !Services.settings.reduceMotion
    readonly property real intensity: Services.connected ? Services.settings.reactiveIntensity : 0
    // True while something audible is playing.
    readonly property bool playing: Services.connected && Services.audio.active

    property real energy: 0
    property real pulse: 0
    property real level: 0
    property list<real> spectrum: []
    property int beats: 0

    signal beat(real strength)

    property real _energyTarget: 0
    property real _levelTarget: 0

    readonly property Connections _analysis: Connections {
        target: Services.connected ? Services.audio : null

        function onAnalysis(bands: var, level: real, reactive: real, beat: int, beatStrength: real) {
            root.spectrum = bands;
            root._levelTarget = level;
            root._energyTarget = root.enabled ? Math.min(1, reactive * root.intensity) : 0;
            if (beat !== root.beats) {
                root.beats = beat;
                if (root.enabled)
                    root.pulse = Math.max(root.pulse, Math.min(1, beatStrength * (0.4 + root.intensity)));
                root.beat(beatStrength);
            }
        }
    }

    readonly property FrameAnimation _clock: FrameAnimation {
        running: root._energyTarget > 0.001 || root.energy > 0.001 || root.pulse > 0.001
                 || root._levelTarget > 0.001 || root.level > 0.001
        onTriggered: {
            const dt = Math.min(frameTime, 0.1);
            const target = root.enabled ? root._energyTarget : 0;
            // Quick to rise, a little slower to fall: lively without flicker.
            const tau = target > root.energy ? 0.045 : 0.12;
            root.energy += (target - root.energy) * (1 - Math.exp(-dt / tau));
            root.pulse = root.enabled ? root.pulse * Math.exp(-dt / 0.22) : 0;
            root.level += (root._levelTarget - root.level) * (1 - Math.exp(-dt / 0.08));
            if (root.energy < 0.0005 && target === 0)
                root.energy = 0;
            if (root.pulse < 0.0005)
                root.pulse = 0;
            if (root.level < 0.0005 && root._levelTarget === 0)
                root.level = 0;
        }
    }
}
