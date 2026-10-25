# Roadmap

Phases from [VISION.md §16](../VISION.md#16-roadmap-phases-with-exit-criteria),
tracked as GitHub milestones. Each phase ends at its **exit criteria**, not a
date.

## Phase 0 · Foundations: complete (v0.1.0)

Exit: shell runs on Windows with simulated services; tests and qmllint green.

- [x] Lumen tokens as a single OKLCH source → QML, C++ and docs
- [x] `Theme` with OKLab-interpolated appearance and accent springs
- [x] `Spring` (closed-form), SDF `Surface`, `Glass` + shared `Backdrop`, `RoundMask`
- [x] Aurora wallpaper (cached layer; transforms for zoom and parallax)
- [x] Lumen Qt Quick Controls style (23 controls)
- [x] Service contracts (QtRO), simulator, `qtos-simd`, `qtosctl`
- [x] Shell: lock, Rail, Live Activity, Shelf, Beam, Control Center, notifications, windows, snapping, Spaces, Overview
- [x] Apps: Files, Music, Notes, Terminal, Monitor, Settings
- [x] Lumen `QStyle` + `qtos` platform theme + gallery
- [x] Benchmark mode, passive frame stats, GPU preference
- [x] Tests (6 suites), zero-warning qmllint, clang-format, SPDX, pre-commit, CI workflows
- [x] Documentation set and ADRs

## Phase 1 · Nested Prism

Exit: a daily-drivable *nested* session for an hour with real Qt apps at
display refresh; compositor backend decision gate
([ADR-0003](adr/0003-compositor-strategy.md)).

- [x] Build and run `prism` nested on Linux and under WSLg; launch Qt apps into it
- [x] Client windows: configure with all states, server- and client-side decorations, client moves and resizes, resizing from every edge, maximize and minimize requests, popups, size limits, app identity, and XDG activation with focus-stealing prevention. All are covered by `prism_e2e` (31 checks).
- [ ] Spaces for client windows. Overview, minimise and Shelf restore for clients are done and covered by `prism_e2e`.
- [x] Wayland extensions: fractional-scale, cursor-shape, presentation-time, idle-inhibit (`tools/dev/prism-protocol-check.sh`)
- [ ] Three-finger gestures for Overview and Spaces (libinput through Qt)
- [ ] Decision-gate benchmark: multi-output frame pacing and input latency
- [ ] Evaluate Qt Interface Framework for service codegen and simulation (ADR)
- [ ] Qt Design Studio project for Lumen components

## Phase 2 · Bootable alpha

Exit: boots on QEMU and the reference laptop; flicker-free boot; Widgets apps
themed.

- [ ] `meta-qtos` image builds (pin layer SHAs); Boot2Qt evaluation
- [ ] KMS backend (atomic, per-output scheduling, overlay planes) or Waylib fallback
- [ ] greetd + Prism greeter; flicker-free handoff
- [ ] `qtos` platform theme and Lumen QStyle installed system-wide
- [ ] Browser app (Qt WebEngine)
- [ ] Qt Sensors: auto-rotate and ambient light

## Phase 3 · Platform

Exit: an unattended update and a forced rollback both succeed; third-party
Flatpaks run sandboxed.

- [ ] Real daemons for every contract (`qtos-netd`, `-btd`, `-powerd`, `-audiod`, `-notifyd`, `-settingsd`)
- [ ] xdg-desktop-portal backend (`qtos-portald`) with trusted-UI prompts
- [ ] Qt Application Manager packages; Flatpak; Store
- [ ] RAUC A/B updates with rollback; fwupd
- [ ] `qtos-indexd` (SQLite FTS5) for Beam file search
- [ ] Translations (Qt Linguist)

## Phase 4 · Premium

Exit: all performance budgets met in CI; screen-reader walkthrough of every
shell surface.

- [ ] HDR, VRR, fractional scaling
- [ ] Spatial Audio UI sounds, Lottie symbols, VectorImage icons
- [ ] Quick Look (Qt PDF, Qt Multimedia, Qt Quick 3D)
- [ ] Window Stacks, Projects, Stash, hero transitions (`qtos_hero`)
- [ ] Qt-native screen reader (TextToSpeech), Increase Contrast, larger text
- [ ] Radial appearance sweep; Increase Contrast tokens
- [ ] Region-limited backdrop capture (integrated GPU p95 < 8 ms)

## Phase 5 · Ecosystem

Exit: an external developer ships an app using only the public SDK.

- [ ] Installable SDK (Kit + Lumen + Services) and Qt Creator wizard
- [ ] Intents → Flows automation; HTTP and MCP surfaces for automation
- [ ] Continuity (KDE Connect protocol)
- [ ] Optional on-device intelligence (`qtos-assistd`, Qt gRPC)
- [ ] Spatial mode (Qt Quick 3D Xr)
