# Changelog

All notable changes to QOR OS, and before it QTOS, are recorded here. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the
project uses [Semantic Versioning](https://semver.org/) from 1.0 onward. The
carried products (QQ, ARQADE, QOR Media) keep their own history upstream in
demiurge-chain; pulls from it are recorded here.

## [Unreleased]

QOR OS begins: QTOS and Demiurge's device-side products in one repository
([QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md)).

### Added

- **QOR OS**, the private repository `QOR-MATRIX/qor-os`, seeded with QTOS's full history (36 commits and its tags). QTOS is frozen: its code at `37b621e`, with a note pointing here (`3ecdded`).
- **QQ, ARQADE and QOR Media**, carried from demiurge-chain (`28d9e97`) with their history by `git subtree`, at the same paths as upstream: `products/qq/`, `products/arqade/qt/`, `products/arqade/games/`, `products/arqade/tests/fixtures/` and `tools/qor-media/`, with `docs/design/tokens.json` copied at the same commit. All three build here and pass their suites: QQ 8 suites, ARQADE 4, QOR Media 9.
  - `tools/dev/products.ps1` builds and tests them with their own scripts, and tries a failing product once more.
  - `tools/upstream/demiurge.sh` shows their upstream state, pulls from demiurge-chain, and writes patches for upstream ([docs/UPSTREAM.md](docs/UPSTREAM.md)).
- **The QOR core**, built from demiurge-chain at a pinned commit through Corrosion (`core/`, `qor::core`, `QOR_BUILD_CORE`, `QOR_CORE_SOURCE_DIR`), never copied ([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)). `tst_qorcore` calls it through its C bridge: the core's version, CGT's units, amounts read exactly, the largest amount never rounded, refusals as errors, and no command that reads a key.
  - The pin moved from `28d9e97` to `e99c7ce`, the merge of demiurge-chain's L13.2 (#107), with the `desktop` feature: QOR ID's and Social's clients, the chain client, the Library's index, the Mesh and Qontrol are now in the core QOR OS builds.
  - On 28 October it moved to `6a4822d`, the merge of demiurge-chain #109: the same code, with the core's manifest declaring Apache-2.0 instead of MIT (demiurge-chain's ADR-122).
  - The pin (`QOR_CORE_COMMIT`) and features (`QOR_CORE_FEATURES`) are written on every configure instead of cached, so a build folder, CI's included, follows a moved pin.
- **One rulebook and one decision log.** `AGENTS.md` (with `CLAUDE.md` pointing to it) merges Demiurge's rules on checking claims, money, language and secrets with QTOS's build and CI practice. `docs/decisions/` holds QOR-0001 to QOR-0009, and QTOS's records as QTOS-0001 to QTOS-0010.
- `OWNER.md`: where QOR OS stands, on one page, rewritten every session.
- `tools/ci/runners.ps1` moves the workstation's two CI runners to the organisation or a repository and starts them. It leaves a runner already in place alone and never stops one that is running a job. Both runners serve the organisation since 27 October.
- `LICENSES/Apache-2.0.txt` and the carried paths in `THIRD_PARTY_NOTICES.md` and `LICENSE`.
- **The carried products build and test on Linux** (Phase 1, begun 28 October). `tools/dev/products.sh` is the Linux counterpart of `products.ps1`: it configures QQ, ARQADE and QOR Media with CMake and Ninja against the `gcc_64` kit, builds them, and runs the suites each registers with CTest, running failed suites once more. It fetches PDFium's pinned release for Linux and checks its SHA-256. **Under WSL it fences the run from the GPU**: three runs there that drew on it through Direct3D 12 hung the workstation on 28 October, each with a suite stuck in the driver where ctest's timeout could not reach it. The script runs itself again in a mount namespace where `/dev/dxg`, WSL's GPU device, is `/dev/null`, so Mesa draws in software and nothing reaches the GPU, and with systemd's user manager in a scope held to half the processors and three quarters of WSL's memory. In software QQ's walk and the QQ Player's frame-rate floor fail; `--gpu` draws on the GPU as before. The suites run on a display of the script's own, `tools/dev/products-display.qml`: a compositor on Qt's offscreen platform that keeps each window at its own size and gives the newest the keyboard, with no X display and no sound. No window or sound reaches the desktop, and nothing there can take a test window's keyboard. `--display prism` runs them inside Prism, as hidden; `--display session` on the desktop. Linux CI runs it when the carried code changes, as Windows CI runs `products.ps1`, but only where `QOR_RUN_LINUX_PRODUCTS` is set, which it is not: the Linux runner shares that workstation.
  - The Linux kit gains Qt Quick 3D Physics, which every QQ scene imports, and Qt PDF, which QOR Media links; both are in `.github/qt-packages.json`.
  - **QQ Studio keeps the creator's provider key in the Secret Service on Linux** ([QOR-0011](docs/decisions/QOR-0011-the-creators-provider-key-in-the-secret-service.md)), over Qt D-Bus, as it keeps it in Credential Manager on Windows: an item in the default collection, unlocked through the keyring's prompt. `AGENTS.md` §6 says a creator's own key for an outside provider is not a QOR key. In the hidden modes `products.sh` gives the run a session bus of its own with `tools/dev/secret-service.py`, an in-memory Secret Service (tested by `tests/tools/test_secret_service.py`), so the suites never reach a real keyring. `--tests <regex>` runs only the matching suites.
  - The run's display gives the keyboard to the window below one that closes only once the closed window's surface is gone. Moving it while the surface was being destroyed crashed Qt Wayland Compositor, whenever a window closed with another still open, as in QOR Media's reader suite, and left the suites after it with no display.
  - On Wayland the products' windows stay toplevels and Prism places them ([QOR-0010](docs/decisions/QOR-0010-on-wayland-prism-places-the-products-windows.md)); QOR Media's docking case is skipped there and still runs on Windows.
- **`docs/release/`**, the owner's pack for the six release gates, linked from ROADMAP.md's gates, which stay the only status: the letter to The Qt Company, a trademark knockout brief for an attorney, a brief for counsel on the terms, the privacy policy and the legal opinion, code signing and the Secure Boot key, an RFP for the external security review, and a runbook for replacing the 5 October key. It is research, not legal advice, and ticks no gate. `THIRD_PARTY_NOTICES.md` now says what licence the core's crate declares at the pin.
- **[QOR-0012](docs/decisions/QOR-0012-github-stays-the-source-of-truth-entire-mirrors-it.md): GitHub stays the source of truth.** Entire, the Git forge launched on 26 October 2026, mirrors QOR OS only once it documents private repositories, and nothing here depends on the mirror. demiurge-chain's mirror is its ADR-124. [CI.md](docs/CI.md#runners-for-the-organisation) now says what was seen and tried when both runners showed online but took no jobs, on 29 October. The same day the owner allowed Entire in full ([QOR-0013](docs/decisions/QOR-0013-entire-may-reach-every-repository.md), superseding QOR-0012 decision 2): its app keeps its access to every repository, QOR OS included, and Entire mirrors QOR OS too.
- **`tools/ci/runners.ps1 -Reregister`** removes and registers both runners again at the same scope, deleting the settings GitHub migrated for the removed registrations, for runners GitHub shows online that take no jobs ([CI.md](docs/CI.md#runners-for-the-organisation)).

### Changed

- The product is **QOR OS**. README, VISION, ROADMAP, ARCHITECTURE, BUILDING, DEVELOPMENT, CI, PERFORMANCE, QT-MODULES, STORE, CONTRIBUTING, SECURITY, issue templates, labels and milestones are rewritten for it.
- **The name is QOR OS in the code too** ([QOR-0009](docs/decisions/QOR-0009-the-name-is-qor-os.md), superseding QOR-0008's wait for clearance). QTOS now names only the predecessor.
  - Code: the `qor::` namespace, `qor_*` targets, `qor.*` logging categories, `QOR.*` QML modules (`QOR.Kit`, `QOR.Shell`, `QOR.Services`, `QOR.Store`, `QOR.Input`, `QOR.Apps`, `QOR.Prism`), `QOR_*` options and environment variables, `LicenseRef-QOR-OS-Proprietary`, and `project(qor-os)`.
  - Programs: `qor-shell`, `qorctl`, `qor-simd`, `qor-gamepads`; the SDK template `sdk/templates/qor-app`; `tools/dev/Enter-QorShell.ps1`.
  - The interface says QOR OS, and the generated music is **QOR Sessions**. The shell and simulator carry their settings and data (installed Store items among them) over from "QTOS Shell" and "QTOS Simulator" on first run.
  - The Store: the **QOR OS Store**, first-party items as `dev.qorsync.*` in `.qorpkg` packages, re-signed with the same store key (`store/keys/store.pub` unchanged). `build_store.py` uses `~/.qor-os/store-signing/`, where the keys moved on 27 October (verified byte for byte, with a dated backup), and falls back to `~/.qtos/store-signing/` on a machine that has only that.
  - CI variables are `QOR_*` on `qor-os`; the organisation domain is `qorsync.dev`.
- The roadmap is QOR OS's phases 0 to 4, with release gates. QTOS's phases are kept as history, and the phase numbers cited across the documents follow the new ones.
- The base image is **Fedora Atomic** with bootc, NVIDIA's open kernel modules and Mesa ([QOR-0004](docs/decisions/QOR-0004-the-base-image-is-fedora-atomic.md)).
- **Displays:** every display at its own refresh rate on every GPU is the acceptance test, and Prism gets its own KMS backend. QTOS's bare-metal decision gate is **deferred, not failed**, and the gate kit becomes a diagnostic ([QOR-0005](docs/decisions/QOR-0005-every-display-at-its-own-refresh.md)).
- **Money:** QOR OS 1.0 moves no money. It has free items and test CGT only, and the Store's paid stage waits for written decisions ([QOR-0006](docs/decisions/QOR-0006-qor-os-1-0-moves-no-money.md)).
- CI:
  - the presets require the QOR core;
  - the Windows job builds and tests the carried products when they change;
  - Lint skips the carried code's headers and formatting;
  - Linux CI installs `libdbus-1-dev`.

### Removed

- QTOS's Yocto skeleton (`image/meta-qtos`, `image/kas`), replaced by the Fedora Atomic plan in `image/README.md`.

## QTOS after 0.1.0 (unreleased, carried into QOR OS on 26 October 2026)

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
  - `shell_e2e` covers first-party apps on every platform. `prism_e2e` covers real Wayland clients (52 checks), with the new `prism-test-client`.
  - Documented in docs/DEVELOPMENT.md.
- **Wayland app identity.** The Shelf names and draws client apps from their desktop entry (`DesktopEntries`, with XDG icon-theme lookup), falling back to the window title.
- **Resizing from every edge and corner** (eight grips; left and top edges keep the opposite side in place). Client-initiated moves and resizes (`xdg_toplevel.move`/`resize`) go through the same springs, snapping and size limits. `WindowManagement.takeOver()` detaches Qt's built-in xdg-shell handlers, which otherwise fought Prism over the window size.
- **xdg-activation with focus-stealing prevention.** Tokens issued while the requesting client has keyboard focus (or by Prism, for launches) activate the window. Any other request only marks it as wanting attention: a Shelf hop and an accent dot, cleared on focus.
- **Client size limits and the resizing state.** Interactive resizing respects a client's minimum and maximum size, and clients receive `xdg_toplevel` `resizing` while it lasts.
- **Benchmark v2**, shared by `qtos-shell` and `prism` (`prism/shell/shellbenchmark.cpp`): `prism --benchmark` reports per-output frame pacing, missed frames at each output's refresh rate, and input-to-swap latency. Top-level JSON fields are unchanged. Baselines: input to swap 5.9 ms p50 on the RTX 4060 (360 Hz); nested Prism under WSLg holds 60 Hz. This is the tooling for the ADR-0003 decision gate, which needs a bare-metal run.
- **Several displays in Prism** ([SHELL.md](docs/design/SHELL.md#displays)).
  - One Wayland output per display: `prism --outputs N` nested; on bare metal (eglfs/KMS) every connected screen. The main display keeps the whole shell; the others show the wallpaper and their windows (`DisplayDesktop`).
  - Move windows between displays with the window menu, <kbd>Meta</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd>, or by dragging the title bar past a display's edge. Spaces, Overview and the lock screen span all displays, and one window is active across them all.
  - Clients are on the output of the display that shows them, so frame callbacks follow them. In-window frost samples each display's own wallpaper (`Materials.desktopFor`).
  - `--spread-clients` opens new clients on the emptiest display. Keyboard shortcuts work from every display.
  - **The decision-gate kit**: `tools/dev/prism-gate.sh` runs Prism on eglfs/KMS with an animating client per output, benchmarks it and judges the result against the gate's criteria. It finds the GPU that has both displays, offers a probe (`--probe`), a trial inside a desktop session (`--try`) and per-display modes (`--mode`), and packs the results for upload. `tools/dev/prism-gate-bundle.sh` packs Prism, the test client and their Qt runtime (about 100 MB) for a stock Ubuntu desktop, so a live USB session can run the gate without a Qt kit or a build. The benchmark keeps every output drawing while it measures (single-output numbers unchanged).
  - `prism_e2e` adds a two-display run (menu, shortcut, edge drag, Spaces across displays) and a two-output benchmark: 52 checks. Shell scripts reach windows on any display and report each window's `display` and the number of `displays`; screenshots take a `display`.
- **The QTOS Store, stage 1** ([docs/STORE.md](docs/STORE.md)).
  - A new **Store** app: Discover, Themes and Widgets, search, item pages with live previews, and a Library with updates. Apps lists what comes with QTOS. Games, Tools and Music arrive with the developer program, and the app says so.
  - Eight **themes** (Ember, Tidepool, Moss, Dusk, Paper, Graphite, Saffron, Glacier). A theme sets the accent, a preferred appearance, a tint for every neutral surface and the Aurora wallpaper's character. Choose one in the Store, Settings › Appearance or Beam.
  - **Desktop widgets** on a grid over the wallpaper: Clock, Calendar, Now Playing, System, Focus and Visualizer. Add them from the Store or the gallery (right-click the desktop › Edit Widgets, or Beam), drag them between cells and remove them. The layout persists.
  - **Every install is verified.** The store signs the catalogue, the developer signs each package's manifest, and every file is hashed. Signatures are ECDSA P-256, checked by CNG on Windows and OpenSSL elsewhere. Installs are staged and swapped in whole.
  - `QTOS.Store` (`StoreEngine`, `Store`, `Themes`, `ThemePreview`); `tools/store/build_store.py` builds, signs and checks the catalogue (CI checks it); `tst_store`, `store_widgets_qmllint` and a Store section in `shell_e2e`.
  - `Settings` gains `theme` and `widgets`; `Intents` gain `widgets.add` and `widgets.edit`; `Theme` gains `neutralHue`, `neutralTint`, `wallpaperSeed` and `wallpaperIntensity`, and `Oklch.themed` optional neutral arguments (Aurora's colours are unchanged). Shell scripts report `store`, `editingWidgets` and `desktopWidgets`.
  - Lucide icons `shopping-bag`, `badge-check`, `wrench` and `timer`.
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
