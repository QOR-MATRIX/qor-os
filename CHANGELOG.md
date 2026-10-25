# Changelog

All notable changes to QTOS are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/) from 1.0 onward.

## [Unreleased]

### Added

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
  - `shell_e2e` covers first-party apps on every platform. `prism_e2e` covers real Wayland clients (31 checks), with the new `prism-test-client`.
  - Documented in docs/DEVELOPMENT.md.
- **Wayland app identity.** The Shelf names and draws client apps from their desktop entry (`DesktopEntries`, with XDG icon-theme lookup), falling back to the window title.
- **Resizing from every edge and corner** (eight grips; left and top edges keep the opposite side in place). Client-initiated moves and resizes (`xdg_toplevel.move`/`resize`) go through the same springs, snapping and size limits. `WindowManagement.takeOver()` detaches Qt's built-in xdg-shell handlers, which otherwise fought Prism over the window size.
- **xdg-activation with focus-stealing prevention.** Tokens issued while the requesting client has keyboard focus (or by Prism, for launches) activate the window. Any other request only marks it as wanting attention: a Shelf hop and an accent dot, cleared on focus.
- **Client size limits and the resizing state.** Interactive resizing respects a client's minimum and maximum size, and clients receive `xdg_toplevel` `resizing` while it lasts.
- **Benchmark v2**, shared by `qtos-shell` and `prism` (`prism/shell/shellbenchmark.cpp`): `prism --benchmark` reports per-output frame pacing, missed frames at each output's refresh rate, and input-to-swap latency. Top-level JSON fields are unchanged. Baselines: input to swap 5.9 ms p50 on the RTX 4060 (360 Hz); nested Prism under WSLg holds 60 Hz. This is the tooling for the ADR-0003 decision gate, which needs a bare-metal run.

### Fixed

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
