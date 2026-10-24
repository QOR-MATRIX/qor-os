# Changelog

All notable changes to QTOS are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/) from 1.0 onward.

## [Unreleased]

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
