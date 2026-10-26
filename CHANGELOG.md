# Changelog

All notable changes to QTOS are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/) from 1.0 onward.

## [Unreleased]

### Added

- **Sound, everywhere.** QTOS plays real audio through the machine's outputs ([docs/AUDIO.md](docs/AUDIO.md)).
  - The new audio engine (`audio/`, `qtos_audio`) uses Qt Multimedia's real-time output callback, with lock-free hand-off to the audio thread, click-free fades, a perceptual volume taper and a soft limiter.
  - **QTOS Sessions**: original music generated live and deterministically by a small synthesiser (band-limited oscillators, state-variable filters, synthesised drums, ping-pong delay, reverb) in four styles. The five scenario tracks are sessions.
  - **Your music**: files in the Music folder and `QTOS_MUSIC_DIR` (MP3, FLAC, Ogg, Opus, WAV, AAC, WMA, AIFF) are decoded with `QAudioDecoder` and queued after the sessions.
  - **Five-band equaliser** with seven presets, and a response curve drawn over the live spectrum in Settings.
  - **Output devices** follow the system: real devices are listed, hot-plugging is tracked, and the chosen output is remembered.
  - **Interface sounds** (`Sounds` in `QTOS.Kit`): notification, lock, unlock, volume and connect cues, synthesised by `tools/sounds/generate_sounds.py`, with their own switch and volume.
- **Audio-reactive interface.** The wallpaper's light swells with the music and flares on beats, the Live Activity shows the real spectrum, the Music artwork breathes on the beat, and Music and Control Center gain visualisers.
  - Settings › Sound adds **React to sound**, an **Intensity** slider, and the frequencies it **Reacts to**: presets (Bass, Vocals, Highs, Full range) or any range on a logarithmic range slider over the live spectrum. A response meter shows the energy and beats.
  - The analysis (48-band spectrum, loudness, range energy, beat detection) runs in the engine and reaches the interface as the `Audio` service's `analysis` signal; `Reactive` (`QTOS.Services`) smooths it at the display's frame rate. Reduce Motion stills it.
- Service contracts: `Audio` gains `outputAvailable`, the equaliser, interface-sound settings, `active` and `analysis`; `Media` gains `buffering` and queue entries with `kind`, `session` and `path`; `Settings` gains `audioReactive`, `reactiveIntensity`, `reactiveLowHz` and `reactiveHighHz`.
- Kit `Spectrum` and Lumen `RangeSlider`.
- `qtosctl media seek`, `eq`, `sounds`, `reactive` and `spectrum` (a live spectrum in the terminal); `status` shows the output, EQ, reactive settings and play position.
- `qtos-simd --silent`. Automated runs (`--screenshot`, `--benchmark`, `--script`) and every test use the engine's null output, which renders in real time to nowhere.
- `qtos-shell --open settings/sound` opens an app at a page.
- **Controller interface.** QTOS can be driven entirely with a game controller ([docs/CONTROLLERS.md](docs/CONTROLLERS.md)).
  - When a controller connects, QTOS asks whether to switch (South: switch; East: not now; "always switch automatically" remembers). When the last controller disconnects, QTOS returns to keyboard and mouse.
  - The left stick drives a Lumen cursor; South clicks and West right-clicks; East goes back; the D-pad moves focus. North opens Beam, Start Control Center, Back Overview, Guide home or unlock. The shoulders switch Spaces, the triggers set the volume, and the right stick scrolls.
  - An on-screen keyboard opens for text fields. A hint bar shows each button's job with the glyphs of the controller in use (A B X Y, or Cross Circle Square Triangle). PlayStation light bars follow the accent colour. Selections give a light vibration.
  - Native backends with no third-party libraries:
    - XInput, including the Guide button: Xbox controllers, SteelSeries Stratus Duo and other XInput pads.
    - Raw HID: DualShock 4 and DualSense over USB and Bluetooth, with rumble, light bar and battery; generic HID pads.
    - Linux evdev, with hotplug.
  - The same controller seen twice (Steam Input) never acts twice.
  - Settings › Controllers: the interface, auto-switch, cursor speed, vibration and light bar; connected controllers with battery and Identify; a live tester of every button, stick and trigger; and the button map.
  - `qtos-gamepads` lists controllers and streams their state. `tools/dev/uinput-gamepad.py` creates a kernel game pad on Linux.
  - Shell scripts gain a virtual controller (`gamepad` steps). `tst_input` covers reports, CRCs, the mappings, dead zones, the manager and the navigator, and `shell_e2e` the whole controller flow.
- **Music library.** Songs, Albums and Artists views, and QTOS Sessions and Music Folder collections with real counts (the placeholder playlists are gone). Play and Shuffle buttons start a view; the playing song shows a live spectrum.
- **Shuffle and repeat** (`Media.shuffle`, `Media.repeat`): shuffle plays every track once per pass; repeat cycles through all, one and off. Both persist, and `qtosctl media shuffle|repeat` sets them.
- **Media and volume keys** everywhere, including the lock screen, with a glass volume indicator (`VolumeHud`).
- Lucide icons `repeat-1`, `disc-3`, `mic-vocal`, `list-music`, `library`, `audio-lines` and `gamepad-2` (113 in all).
- On Linux without a running PipeWire daemon (WSLg), QTOS asks Qt Multimedia for its PulseAudio backend, avoiding a crash in Qt 6.12's PipeWire backend.
- Shell scripts: a `scroll` step; the state includes `audio`, `media` and the reactive settings; item targets are found among `Repeater` delegates and must not be clipped or scrolled out of view.
- Tests: `tst_audio` (24 cases), playback and analysis over Qt Remote Objects in `tst_services`, and the Sound page in `shell_e2e`.
- **Increase Contrast**, a new accessibility setting.
  - It is available through the `increaseContrast` setting, in Settings → Accessibility, as a Beam action, as `qtosctl contrast on|off`, and as `qtos-shell --contrast high`.
  - Roles can define `highContrast` token values, which `Theme.contrast` blends in OKLab on a spring.
  - Targets: 7:1 for primary, secondary and accent text; 4.5:1 for tertiary text and status colors; 3:1 for focus rings.
  - Separators, strokes and control fills strengthen, and glass becomes solid (`Theme.solidMaterials`).
  - The Widgets palette follows `QTOS_CONTRAST=high`.
- **Luminance-capped tokens.** `maxLuminance` lowers lightness until a role's WCAG luminance fits under the cap. White text on `accent` and `accentHover` therefore stays at least 4.5:1 at *every* accent hue, which OKLCH lightness alone cannot guarantee. Implemented in `color::capLuminance`, mirrored in the token generator.
- **Golden-image tests (`tst_golden`, label `gpu`).**
  - Covers Lumen controls, Kit components, and wallpaper with backdrop and glass, each in light, dark, high-contrast and RTL variants.
  - Comparison is a perceptual per-pixel OKLab diff with failure heatmaps, plus an `update_golden` target. CI uploads the diffs when a case fails.
  - One reference set serves Windows (Direct3D 11) and Linux (OpenGL), which differ on under 0.01% of pixels. Per-OS overrides are optional.
- Token tests that enforce the WCAG contrast guarantees.
- `qtosctl transparency on|off`.
- **Prism runs real Wayland clients** (verified nested in WSLg with `weston-terminal` and a Qt Widgets app).
  - Server-side decorations for `xdg-decoration` clients; none for clients that draw their own.
  - Client window geometry is aligned with the frame (shadows clipped, never scaled), and the frame adopts the client's chosen size.
  - Activated and maximized states are sent; client maximize and minimize requests are honoured.
  - Moves started from a client's own title bar use Prism's springs and edge snapping.
  - Clients are configured for the target size, not every frame of a resize animation.
  - New `prism` options: `--socket`, `--size`, `--surface`, `--screenshot` and `--delay`. Prism prints the socket clients should use.
- **Wayland protocols in Prism**, verified on the wire by `tools/dev/prism-protocol-check.sh`.
  - Prism implements `wp_fractional_scale_v1` and `wp_cursor_shape_v1` (v2) itself, with protocol XML vendored from wayland-protocols 1.47.
  - Qt's `wp_presentation` is enabled, fed with swap timestamps, and works around a Qt 6.12 initialisation bug for the QML element.
  - Qt's idle-inhibit and text-input-v3 are enabled.
- Linux on WSL2 build and run guide (docs/BUILDING.md).
- **Shell scripts and end-to-end tests.**
  - `qtos-shell --script` and `prism --script` drive the shell with synthetic input through `QWindowSystemInterface` (clicks, drags, keys, shell calls), and assert with `expect` steps.
  - `shell_e2e` covers first-party apps on every platform. `prism_e2e` covers real Wayland clients (42 checks), with the new `prism-test-client`.
  - Documented in docs/DEVELOPMENT.md.
- **Wayland app identity.** The Shelf names and draws client apps from their desktop entry (`DesktopEntries`, with XDG icon-theme lookup), falling back to the window title.
- **Resizing from every edge and corner** (eight grips; left and top edges keep the opposite side in place). Client-initiated moves and resizes (`xdg_toplevel.move`/`resize`) go through the same springs, snapping and size limits. `WindowManagement.takeOver()` detaches Qt's built-in xdg-shell handlers, which otherwise fought Prism over the window size.
- **xdg-activation with focus-stealing prevention.** Tokens issued while the requesting client has keyboard focus (or by Prism, for launches) activate the window. Any other request only marks it as wanting attention: a Shelf hop and an accent dot, cleared on focus.
- **Client size limits and the resizing state.** Interactive resizing respects a client's minimum and maximum size, and clients receive `xdg_toplevel` `resizing` while it lasts.
- **Benchmark v2**, shared by `qtos-shell` and `prism` (`prism/shell/shellbenchmark.cpp`): `prism --benchmark` reports per-output frame pacing, missed frames at each output's refresh rate, and input-to-swap latency. Top-level JSON fields are unchanged. Baselines: input to swap 5.9 ms p50 on the RTX 4060 (360 Hz); nested Prism under WSLg holds 60 Hz. This is the tooling for the ADR-0003 decision gate, which needs a bare-metal run.
- **Three-finger gestures** ([SHELL.md](docs/design/SHELL.md#gestures)). Swipe sideways between Spaces, and up and down for Overview, on a touchpad or a touchscreen. The desktop follows the fingers, and the springs carry on with their velocity.
  - Overview opens progressively: windows sit between their places and the Overview grid as far as the fingers go.
  - `SystemGestures` (`QTOS.Input`) takes swipes of three or more fingers for the whole window, before apps and Wayland clients. Touchpad swipes arrive as Qt's native gestures (the host's pointer gestures under nested Prism). Touchscreen swipes come from touch points, and the apps under the fingers get a touch cancel.
  - Windows reports no touchpad gestures to apps, so QTOS reads precision touchpads through Raw Input (`QTOS_TOUCHPAD=off` turns this off; automated runs do).
  - Shell scripts gain `gesture` and `touch` steps on a virtual clock. The state includes `spaceOffset`, `overviewProgress` and `gesture`, windows their `overviewMix`, and `expect` compares single numbers with `near`. `tst_input` covers frame assembly, swipe tracking and recognition; `shell_e2e` and `prism_e2e` cover the shell.
- **Spaces for every window, Wayland clients included** ([SHELL.md](docs/design/SHELL.md#spaces)).
  - Windows ride their Space along the strip, and windows on Spaces out of view are not drawn.
  - Take the active window along with <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd>, <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>1</kbd>…<kbd>3</kbd> or Beam's "Move Window to Space".
  - Send any window away from its new title-bar menu (right-click: Minimize, Maximize, Move to Space, Close), or drag it onto a Space chip in Overview.
  - Clients on Spaces out of view leave the output and get no frame callbacks, so they stop drawing. `prism_e2e` checks this with an animating client (`prism-test-client --animate`).
  - Shell scripts gain `rightClick` steps. The window state includes `visible`, `commits` and the surface's `onOutput`, and the state includes the shell's `frames`. `prism-e2e.py` keeps each client's output next to Prism's log.

### Fixed

- **Clicking an icon button in a window that overlapped another raised the window underneath.** `IconButton`'s `TapHandler` takes only a passive grab, so the press reached the activation watcher of every window below. Only the topmost window at the press now activates. `shell_e2e` checks it.
- **Automated runs could talk to a QTOS session in use.** On Windows a named pipe accepts several servers, so a second simulator "hosted" the default endpoint alongside a running session, and clients could reach either. The simulator now probes the endpoint first, and `--script`, `--screenshot` and `--benchmark` runs use a private one.
- On Linux the keyboard's play/pause key (delivered as Media Play) only ever started playback; it now toggles.
- **A Wayland client that kept drawing during a resize could pull its window back to an earlier size.** Prism took any committed size other than the latest configured one as the client's own choice, including frames drawn for an earlier configure. It now adopts a client's size only once the client has acknowledged the latest configure (`WindowManagement.configureAcked`). Found by `prism_e2e` with an animating client.
- **Scripted tests could be disturbed by real input.** A real pointer motion with no button held (a person's mouse, or WSLg's pointer crossing a nested Prism) cancels Qt Quick drags, the likely reason `prism_e2e`'s edge resize occasionally stopped part-way. While a script runs, only its own input and the controller interface's (`qtos::input::InjectedInput`) now reach the window.
- **Synthetic keys lost their modifiers inside Prism.** QtWaylandCompositor rebuilds every key event from its XKB state, which only real modifier key events change. Shell scripts' <kbd>Ctrl</kbd>+<kbd>Alt</kbd> shortcuts, the controller's <kbd>Shift</kbd>+<kbd>Tab</kbd> and capitals from the on-screen keyboard therefore arrived without modifiers. Synthetic key strokes (`qtos::input::sendKeyStroke`) now press the modifier keys around the key, as a keyboard does, and carry evdev codes for US punctuation and F1–F12.

- `tools/dev/run.ps1 -- <shell options>` passed the first shell option as `-Preset` (so `run.ps1 -- --unlocked` failed with "not built for preset"). `-Preset` is now named-only.

- **Mouse presses never reached Wayland clients, nor item-based controls inside app windows** (Qt Quick Controls, text fields). AppWindow's activation overlay used a `TapHandler`. In Qt 6.12 that leaves a mouse press accepted (QTBUG-145896), so Qt Quick stopped offering it to items underneath. Activation now uses the overlay's `PointHandler`. Found by the new end-to-end tests.
- A minimised Wayland client could not be restored: `WindowLayer.open()` consulted the first-party app registry before looking for an existing window.
- Wayland clients' app ids were lost: they are set just after the toplevel is created, and only the title was followed.
- New windows could cascade above the work area, under the Rail.
- Default dark-appearance accent: white text on it was 2.9:1. The accent is now a deeper indigo (OKLCH L 0.575) and meets AA. Accent hover darkens instead of lightening, so text keeps its contrast while hovered.
- CI Qt manifest: Qt 6.8+ add-on IDs are architecture-independent. The old per-kit IDs are virtual, and the installer skipped them while still reporting success. `install_qt.py` now verifies the expected CMake packages afterwards.
- `install_qt.py` installs additional packages through the existing installation's MaintenanceTool, since the online installer refuses a non-empty root. It also accepts an existing Qt Account sign-in (`qtaccount.ini`).
- Removed three unused QML imports flagged by qmllint.
- `prism` failed to load on Qt 6 (`ShellSurfaceItem.sizeFollowsSurface` no longer exists). It was never built before: this is its first run.

### Changed

- The scenario no longer starts playing on its own: press play.

- CI: the Linux job runs on a self-hosted WSL2 runner with the commercial kit. With warnings as errors (as on Windows), it runs the GPU suites on WSLg and the Prism protocol check with real Wayland clients; about 40 s incremental.
- CI: a newer push cancels an older Build run on the same branch, so a run queued for an offline runner no longer blocks later builds. GPU suites check for a display, time out after 120 s and retry once.
- CI: per-platform Qt sources (`QTOS_WINDOWS_QT_DIR`, `QTOS_LINUX_QT_DIR`), so a Windows self-hosted kit no longer affects the Linux job. Self-hosted builds are incremental. Qt's bundled CMake and Ninja are put on `PATH` automatically. The optional `QTOS_RUN_GPU_TESTS` runs GPU suites. Build also runs on `v*` tags.
- CI: the installable Windows artifact is uploaded only for tags and manual runs (7-day retention), to stay within the Free-plan storage quota.
- pre-commit: vendored fonts and icons and generated token files are excluded from all hooks. Multi-document YAML (Qt Application Manager manifests) is allowed. clang-format skips the SDK template, matching CI.
- Developer setup uses a repo-local `.venv` for the pinned tooling.
- `QT_QML_GENERATE_QMLLS_INI` is a cache option, so a second build of the same checkout (for example a WSL build of a Windows working tree) can turn it off without repointing the editor.

## [0.1.0] - 2026-10-23

The Phase 0 foundation: the complete Prism shell running in simulation on
Windows, with the Lumen design system, simulated system services over Qt
Remote Objects, six first-party apps, platform integration for Qt Widgets, and
a full development and CI setup.

### Added

- **Lumen design system (QTOS.Kit).**
  - Single-source OKLCH design tokens (`kit/tokens/lumen.tokens.json`), generated into QML, C++ and docs.
  - Live `Theme` with OKLab-interpolated light/dark and accent-hue springs.
  - `Spring`: a closed-form damped-harmonic animation driver.
  - SDF `Surface` (continuous-curvature shape, gradient, hairline, sheen, analytic shadow).
  - `Glass` and `Backdrop`, with one shared blur per frame for every glass panel.
  - `RoundMask`, the procedural Aurora `Wallpaper`, and GPU-tinted `Icon`.
  - `AppIcon`, `Artwork`, `QtosMark`, and app building blocks: `SidebarItem`, `Card`, `SegmentedControl`, `SearchField`, `Kbd`, `Divider`, `IconButton`.
  - `FrameStats`, a passive frame-pacing probe.
- **Lumen Qt Quick Controls style** (`Lumen` module): 23 controls on Qt Quick Templates, with Basic fallback.
- **Prism shell (QTOS.Shell).**
  - Lock screen with a focus-pull unlock.
  - Rail with the Live Activity capsule and status cluster.
  - Shelf with magnification and launch hops.
  - Beam command bar: fuzzy search, safe calculator, preview pane.
  - Control Center, notification banners and history, and the Media popover.
  - Spring-driven window manager: 1:1 drag, magnetic snapping, resize, minimise-to-Shelf, maximise.
  - Three Spaces with parallax, and an Overview grid with Space switcher.
  - Keyboard shortcuts throughout.
- **System services.**
  - Nine Qt Remote Objects contracts (`services/interfaces/*.rep`).
  - Replicas typed for QML (`QTOS.Services`).
  - The `Intents` app→system API.
- **Simulator (`qtos-sim`, `qtos-simd`)**: scenario-driven Wi-Fi, Bluetooth, battery, audio, display, media, notifications and persisted settings.
- **Apps (QTOS.Apps)**: Files, Music, Notes, Terminal, Monitor (Qt Graphs) and Settings (8 pages).
- **Tools.**
  - `qtosctl` CLI.
  - `qtos-shell` options: `--benchmark` (frame pacing as JSON), `--screenshot`, `--gpu`.
  - Token generator, Lucide icon importer, screenshot capture, developer PowerShell scripts.
- **Platform**: Lumen `QStyle` plugin for Qt Widgets, the `qtos` QPA platform theme plugin, and `lumen-widgets-gallery`.
- **Prism compositor** (`prism`, Linux, experimental): Qt Wayland Compositor host for the same shell.
- **Image**: `meta-qtos` Yocto layer skeleton and kas configuration (Phase 2).
- **Quality.**
  - 6 test suites: springs, OKLCH, the QtRO round trip, Beam search, Kit QML, and the token generator.
  - Zero-warning `qmllint`, pinned clang-format, SPDX header check, and pre-commit.
- **CI**: lint workflow, plus Windows and Linux build workflows using commercial Qt via secrets or a self-hosted kit.
- **Docs**: vision, architecture, building, development, CI, performance, roadmap, Lumen and shell specifications, service reference, Qt module map, and ADRs 0001–0010.
