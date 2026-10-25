# Audio

QTOS plays real audio: generated "QTOS Sessions", your own music files and
interface sounds. The interface reacts to what is playing, in a frequency
range and with an intensity you choose in **Settings › Sound**.

| Piece | Where | What it does |
|-------|-------|--------------|
| Audio engine | [`audio/`](../audio/) (`qtos_audio`) | Plays, equalises, limits and analyses |
| `Audio` and `Media` services | [`services/interfaces/`](../services/interfaces/), [`sim/`](../sim/) | Volume, outputs, EQ, interface sounds, analysis; now playing |
| `Reactive` | [`services/qml/Reactive.qml`](../services/qml/Reactive.qml) (`QTOS.Services`) | Turns the analysis into motion the interface animates with |
| `Spectrum`, `Sounds`, `Wallpaper` | [`kit/`](../kit/) (`QTOS.Kit`) | Visualiser, interface sounds, reactive light |
| Settings › Sound, Music, Live Activity, Control Center | [`apps/`](../apps/), [`prism/shell/`](../prism/shell/) | Where you see and control it |

On a device, `qtos-audiod` implements the same `Audio` contract over
PipeWire. In simulation (every desktop build today) the simulator owns a
`qtos::audio::Engine` and plays through the machine's own outputs.

## The engine

```text
 Source ──► transport fade ──► 5-band EQ ──┬──► volume ramp ──► soft limiter ──► QAudioSink callback
 (session                                   │    (cubic taper,     (transparent      (real-time thread)
  or file)                                  │     mute)             below -1.4 dBFS)
                                            └──► mono monitor ring ──► Analyzer (60 Hz) ──► Audio.analysis
```

- **Output** uses Qt Multimedia's real-time callback interface
  (`QAudioSink::start(callback)`, Qt 6.11+): WASAPI on Windows, PipeWire or
  PulseAudio on Linux, Core Audio on macOS. The engine asks for 48 kHz float
  stereo and adapts to what the device offers (other rates, `Int16`, mono or
  more channels).
- **Threads.** The `Engine` object is a thread-safe front. Control,
  decoding and analysis run on an engine thread (`qtos-audio`); mixing runs
  on the output's real-time thread. The [`Renderer`](../audio/src/renderer.h)
  never locks, allocates or frees on that thread: sources arrive through a
  wait-free queue and go back through another one to be deleted, and every
  parameter is an atomic.
- **No clicks.** Play, pause, seeks and track changes fade over 10 ms; a seek
  or track change waits for the fade-out before it takes effect. Volume and
  mute ramp over 20 ms.
- **Volume** has a perceptual cubic taper (as PulseAudio's), so equal steps
  of the slider sound like equal steps of loudness.
- **Equaliser**: low shelf at 60 Hz, peaks at 250 Hz, 1 kHz and 4 kHz, and
  a high shelf at 12 kHz, ±12 dB (RBJ cookbook biquads, transposed direct
  form II). Presets: Flat, Bass Boost, Warm, Vocal, Bright, Loudness, Late
  Night. A flat EQ costs nothing.
- **Soft limiter**: transparent below 0.85 (−1.4 dBFS), then a `tanh` knee
  towards 0.98, so EQ boosts and loud material never clip hard.
- **Idle**: after 3 s of silence with nothing playing, the device stream is
  suspended (and resumed on play), so an idle QTOS costs no audio wake-ups.
- **Device changes** (a headset plugged in, the default output changing, a
  device disappearing) are followed through `QMediaDevices`. A chosen output
  is remembered by name.

### Sources

**QTOS Sessions** ([`session.cpp`](../audio/src/session.cpp)) are original
music generated live and deterministically from a small spec, so QTOS has
sound out of the box without shipping recordings:

| Field | Values |
|-------|--------|
| `style` | `drive` (four on the floor), `pulse` (rolling bass), `chill` (half-time), `ambient` |
| `mode` | `minor`, `dorian`, `major`, `lydian` |
| `bpm`, `root` (MIDI note), `seed` | |

A session has an arrangement (intro, build, main, break, main, outro) and
layers that follow it: a voice-led pad of detuned band-limited (PolyBLEP)
saws through state-variable filters, a side-chained bass, plucked arpeggios
through a ping-pong delay, synthesised drums and a Freeverb-style reverb.
It renders about 50 to 100 times faster than real time.

**Files** ([`filesource.cpp`](../audio/src/filesource.cpp)) are decoded by
`QAudioDecoder` (FFmpeg) on the engine thread into 16-bit chunks that are
published to the audio thread lock-free, so playback starts while decoding
continues. MP3, FLAC, Ogg Vorbis, Opus, WAV, AAC/M4A, WMA and AIFF are
supported, up to about 93 minutes per file. The simulator queues the files
in your Music folder and in `QTOS_MUSIC_DIR` (several folders separated like
`PATH`), three levels deep, after the sessions. Titles come from the layout:
`Artist - Title.ext` or `Artist/Album/01 Title.ext`.

## Analysis

The [`Analyzer`](../audio/src/analyzer.h) looks at what is being heard,
about 60 times a second: a 2048-sample Hann-windowed FFT of the mono mix
taken *after* the EQ and *before* the volume (so turning the volume down
does not calm the interface), aligned with the output's latency.

| Value | Meaning |
|-------|---------|
| `bands` | 48 log-spaced bands from 30 Hz to 16 kHz, 0..1, tilted +3 dB/octave so typical music reads level; fast attack (12 ms), slower fall (180 ms) |
| `level` | RMS loudness, −60..0 dBFS → 0..1 |
| `reactive` | Energy in the chosen range, normalised against its own recent peak (decaying 4 dB/s, over a 24 dB span), so quiet and loud music both move the interface |
| `beat` | Counts onsets in the chosen range: positive spectral flux of log magnitudes against an adaptive threshold from the last second (mean + max(1.5σ, 0.3·mean)), at most every 120 ms |
| `beatStrength` | How far the last onset stood out, 0..1 |

The `Audio` service sends these as its `analysis(bands, level, reactive,
beat, beatStrength)` signal while something is audible, then once more at
rest, and sets `active`.

## The audio-reactive interface

[`Reactive`](../services/qml/Reactive.qml) is the one place the interface
gets its motion from. It smooths the analysis at the display's frame rate:

| Property | Meaning |
|----------|---------|
| `energy` | 0..1: range energy × intensity. Rises in ~45 ms, falls in ~120 ms |
| `pulse` | 0..1: jumps on each beat (by its strength and the intensity), decays in ~220 ms |
| `level` | 0..1 loudness |
| `spectrum` | The 48 bands, for visualisers |
| `enabled`, `playing`, `beat(strength)` | |

`energy` and `pulse` are zero when **React to sound** is off and under
**Reduce Motion**. Visualisers follow `spectrum` either way, like any other
media content.

What reacts:

- **Wallpaper**: light rising from the bottom corners swells with `energy`, a
  third bloom flares on beats, and the field gets a gentle push (`zoom`) on
  each beat. The glow is a separate, cheap shader over Aurora's cached noise
  field, which is never re-rendered for it.
- **Live Activity** (the Rail capsule): four bars of the real spectrum, and
  a rim that catches the energy.
- **Music**: the artwork breathes on the beat and its glow brightens with the
  energy; a mirrored visualiser sits under the controls.
- **Control Center**: a mini visualiser on the Now Playing tile.
- **Settings › Sound**: the live spectrum, with the chosen range lit.

### Settings › Sound

| Control | Setting | Default |
|---------|---------|---------|
| Output volume, Mute, Output device | `Audio.volume`, `muted`, `selectOutput()` | 60 %, off, system default |
| Equalizer: presets and five bands with the response curve over the live spectrum | `Audio.applyEqualizerPreset()`, `setEqualizerBand()` | Flat |
| Play interface sounds, Sound volume | `Audio.systemSounds`, `systemVolume` | on, 50 % |
| React to sound | `Settings.audioReactive` | on |
| Intensity (Subtle, Balanced, Vivid) | `Settings.reactiveIntensity` | 60 % |
| Reacts to: Bass (40–160 Hz), Vocals (300 Hz–3 kHz), Highs (4–16 kHz), Full range, or any range on the slider (20 Hz–20 kHz, logarithmic) | `Settings.reactiveLowHz`, `reactiveHighHz` | Bass |
| Response | | Live meter of `energy`; the dot flashes on beats |

Everything persists (`QSettings` groups `audio` and `reactive`) unless the
simulator runs ephemerally.

## Interface sounds

[`Sounds`](../kit/Sounds.qml) (`QTOS.Kit`) plays short cues with Qt
Multimedia's `SoundEffect`: `notification`, `lock`, `unlock`, `volume`
(when a volume slider is released) and `connect` (charging starts). The
shell binds it to the `Audio` service: on when interface sounds are on and a
device exists, at `volume³ × systemVolume` (relative to the output volume),
on the chosen output. Notifications are silent under Focus. Nothing touches
the audio system until sounds are enabled.

The cues are synthesised by
[`tools/sounds/generate_sounds.py`](../tools/sounds/generate_sounds.py) (FM
bells, shaped tones, filtered noise). The output is byte-for-byte stable;
rerun the script after changing a recipe.

## Running silently

Tests, screenshots, benchmarks and scripted runs never make sound. The
engine's **null output** renders in real time to nowhere, so playback,
positions and analysis behave exactly as with a device:

- `QTOS_AUDIO_OUTPUT=null` (ctest sets it for every test),
- `qtos-shell` and `prism` with `--screenshot`, `--benchmark` or `--script`,
- `qtos-simd --silent`.

With the null output the `Audio` service presents the scenario's simulated
devices and `outputAvailable` is false.

## Tools and tests

```bash
qtosctl media play
qtosctl spectrum 10          # live spectrum, level, range energy and beats in the terminal
qtosctl eq "Bass Boost"
qtosctl reactive range 300 3000
qtosctl reactive intensity 80
qtosctl status               # includes output, EQ, reactive settings and the play position
```

- `tst_audio`: FFT scaling, EQ band gains, limiter, volume taper, analysis
  (range selectivity, spectrum peaks, beats on test signals and on sessions
  at a known tempo), session determinism, headroom and speed, renderer fades
  and end of track, file decoding while playing, library naming, and the
  engine with the null output.
- `tst_services`: playback through the engine over Qt Remote Objects,
  analysis reaching the replica, EQ presets and bands, reactive settings.
- `shell_e2e`: the Sound page's EQ presets, the React to sound switch and the
  range presets under synthetic input.

## Troubleshooting

| Symptom | Check |
|---------|-------|
| No sound | `qtosctl status`: is media playing, and is the output a real device (not "(silent)")? Is `QTOS_AUDIO_OUTPUT=null` set? The log names the device it opened: `qtos.audio: Output: "<device>", 48000 Hz, 2 ch, Float` |
| Linux build: undefined `pa_*` symbols | Qt Multimedia links PulseAudio: `sudo apt-get install libpulse0` |
| Linux: a crash in `QPlatformAudioDevices::create` | Qt 6.12's PipeWire backend crashes when libpipewire is installed but no PipeWire daemon runs (WSLg serves PulseAudio; Weston pulls in libpipewire). QTOS detects the missing PipeWire socket and asks for the PulseAudio backend (`qtos::audio::configureBackend()`); other Qt programs need `QT_AUDIO_BACKEND=pulseaudio` |
| WSL: no device | WSLg's server is `unix:/mnt/wslg/PulseServer`; set `PULSE_SERVER` if your environment lacks it |
| A music file does not appear | Only the extensions above, at most three folders deep, at most 500 files |
| The interface does not react | **React to sound** on? **Reduce Motion** off? Is the chosen range where the music has energy? `qtosctl spectrum` shows the range energy |
