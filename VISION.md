# QOR OS: Vision & Architecture

> The operating system for players, game makers and creators, where every pixel a person sees is drawn by Qt 6.
>
> **Status:** living vision. QOR OS joins QTOS (the operating system) with Demiurge's device-side products
> ([QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md)). What is built is in
> [OWNER.md](OWNER.md) and [docs/ROADMAP.md](docs/ROADMAP.md). Decisions are records in
> [docs/decisions/](docs/decisions/README.md): QOR-NNNN for QOR OS, QTOS-NNNN inherited from QTOS, ADR-NNN in
> demiurge-chain. Sections marked *(decided)* link to the record.

---

## 0. The pitch in eight lines

1. **A gaming and creating OS, top to bottom.** Play, build and publish games, apps, themes and media on one
   machine, with one account and one library, driven by a controller as well as a keyboard.
2. **Linux kernel underneath, Qt everywhere above it.** A Qt 6 compositor, shell, services, game engine, studio,
   arcade, media player and SDK, on an image-based Fedora Atomic base with drivers for every RTX card and for
   Intel and AMD.
3. **The shell is the compositor.** Windows and games are live items in one Qt Quick scene graph, so every window can
   be animated, blurred, morphed, tiled or put in 3D space with QML and shaders, while full-screen games scan out
   directly.
4. **Every display at its own refresh rate.** 60 Hz and 360 Hz screens side by side, VRR where they support it, on
   laptops whose displays are split across two GPUs.
5. **QQ, the QOR Engine, is built in.** QQ Studio builds lit 3D and pixel-perfect 2D games with physics, logic,
   sound and animation, saved as diffable QML; the QQ Player plays them natively and in a browser; ARQADE is the
   arcade they play in.
6. **Every PC game people already own.** Steam and Proton run inside gamescope, as a client of Prism.
7. **Lumen**, a design language built on physically inspired materials, spring motion, continuous-curvature shapes,
   perceptual color and HDR-aware highlights, so every Qt app looks native on day one.
8. **Secure and boring underneath.** Signed boot, whole-image updates with rollback, sandboxed games and apps, a vault
   that keeps keys outside every app, and system prompts drawn by the compositor so nothing can fake them.

### Who it serves

| Person | What QOR OS gives them |
|--------|------------------------|
| **The player** | A controller-first Home with every game in one Library (QQ games, ARQADE, Steam); full-screen games at the display's own refresh rate with VRR; friends, chat and performance in an overlay over any game; one QOR ID with levels and XP |
| **The game designer** | QQ Studio: worlds in 3D and 2D, physics, particles, sound, animation, prefabs, undo; Play and Stop in place; an agent that designs and builds with them over MCP, on their own key |
| **The game developer** | QQ's runtime and logic files, deterministic steps and replay; publishing from the Studio with a provenance record on the QOR network; a Store page with signed packages |
| **The app developer** | QTOS.Kit, the Lumen controls, typed system services, single sign-on through QOR ID, and a simulator that runs the whole shell on Windows |
| **The creator** | Themes and widgets in the Store, versioning with Qontrol, distribution over the Mesh, and royalties that follow a remix once real value exists |

---

## 1. Principles

| # | Principle | What it means in practice |
|---|-----------|---------------------------|
| P1 | **The Qt Line** | Everything user-facing is Qt. Below the line, only *headless* infrastructure is allowed, and only behind a Qt service API. |
| P2 | **Motion is meaning** | Every transition has spatial continuity: things come from somewhere and go somewhere. All animations can be interrupted and keep their velocity when retargeted. |
| P3 | **Direct manipulation** | Gestures track 1:1 with fingers, trackpad or stick, then hand velocity to springs. Nothing waits for a fixed-duration animation to finish. |
| P4 | **Render on demand** | Zero frames, zero wakeups when nothing changes. A premium feel also means good battery life, and the GPU stays free for the game. |
| P5 | **Trust is visible** | Camera, mic, screen capture, location, permission and key-approval prompts live in a compositor-owned layer that apps can't draw over. |
| P6 | **Local-first** | Playing, making and the whole OS work offline. QOR ID connects people; it is never required to use the machine. Intelligence features run on the device. |
| P7 | **Leverage, don't rewrite** | Reuse the mature Qt and Linux ecosystem (KDE Frameworks, Fedora Atomic, gamescope, Proton, Flatpak) wherever it doesn't compromise the experience. |
| P8 | **The game comes first** | A full-screen game gets direct scan-out, its display's own refresh rate and the whole GPU. The shell's work stops while a game runs. |
| P9 | **Makers are paid when their work is used** | Licences, remixes and royalties are recorded on the QOR network. No money moves until the legal and economic decisions are made ([QOR-0006](docs/decisions/QOR-0006-qor-os-1-0-moves-no-money.md)), and nothing is ever framed as an investment. |

---

## 2. The stack at a glance

```text
┌────────────────────────────────────────────────────────────────────┐
│ NETWORK     QOR ID · Social · the Demiurge chain · the Mesh        │  services QOR runs
│             (demiurge-chain, public)                               │  (QOR-0002)
╞════════════════════════════════════════════════════════════════════╡
│ GAMES/APPS  QQ Studio · QQ Player · ARQADE · QOR Media · Steam +   │
│             Proton in gamescope · first-party apps · Flatpaks      │
│ PLATFORM    QTOS Kit · Lumen QQC2 style · QStyle · platform theme  │  100% Qt
│ SHELL       Prism: compositor + shell in ONE Qt Quick scene graph  │
│ SERVICES    qtos-*d daemons (Qt Core, C++) · the vault and chain   │
│             client from the QOR core (Rust, C bridge)              │
│             internal: Qt Remote Objects · external: D-Bus          │
╞═══════════════════════════ THE QT LINE ════════════════════════════╡
│ SUBSTRATE   Fedora Atomic: systemd + logind · PipeWire · Network-  │  headless,
│             Manager · BlueZ · UPower · fwupd · Mesa · NVIDIA open  │  wrapped by
│             kernel modules · libinput · HarfBuzz/FreeType          │  Qt services
│ KERNEL      Linux: DRM/KMS · evdev · Landlock · KVM · btrfs        │
│ BOOT        UEFI Secure Boot · bootc image with rollback · TPM2    │
└────────────────────────────────────────────────────────────────────┘
```

**Why not "Qt all the way down"?** Qt is not a kernel and should not be one. The Linux kernel and the GPU vendors'
drivers provide support for real hardware, and that is not something to recreate. The aim is a uniform *experience*,
not purity for its own sake.

---

## 3. Foundation: image, boot, updates, security

### Image and build *(decided: [QOR-0004](docs/decisions/QOR-0004-the-base-image-is-fedora-atomic.md))*

- **Fedora Atomic, built with bootc.** A QOR OS image is an OCI container image built from a Fedora bootc base and
  booted as a whole operating system. Fedora keeps the kernel, Mesa and firmware current.
- **Graphics drivers:** NVIDIA's open kernel modules for every RTX card (Turing onward), built for the image's kernel
  and signed with QOR OS's Secure Boot key; Mesa for Intel and AMD.
- **Root filesystem:** read-only and image-based; user data lives on **btrfs** (zstd compression, snapshots for
  Rewind; see §10).
- **Encryption:** LUKS2 full-disk encryption, unlocked by TPM2 + PIN (`systemd-cryptenroll`), with a recovery key
  generated at setup.
- **Developer overlay:** `systemd-sysext` images let a developer drop a freshly built compositor or service onto a
  running device without rebuilding the image.

### Boot

- UEFI Secure Boot → signed kernel → measured boot into TPM2 PCRs.
- **Flicker-free boot:** firmware logo (BGRT) → early splash that reuses the BGRT logo → Prism takes over **without a
  modeset** (same mode, same framebuffer contents) → greeter. The handoff should feel like a single continuous
  animation.

### Updates

- **OS:** whole images. `bootc upgrade` stages the next image; a boot that fails returns to the last good one.
- **Firmware:** `fwupd` (LVFS) shown in the same Software Update UI.
- **Games and apps:** updated independently of the OS (see §8).

### Security model

- **Wayland isolation by default:** no global key snooping and no screen scraping without the portal.
- **App and game sandbox:** bubblewrap namespaces + seccomp + Landlock. Permissions declared in the manifest and
  granted at runtime through portals. A stranger's QQ game is also confined inside the engine (Demiurge ADR-087).
- **Trusted UI layer:** permission prompts, password prompts, key approvals and privacy indicators are drawn by Prism in
  a layer clients can never cover. A subtle "trusted" visual cue (for example a specular edge that only system UI can
  render) makes spoofing obvious.
- **Keys live in one place.** The vault (the QOR core, in Rust) runs as its own sandboxed service, outside Prism and
  every app. Nothing it offers answers with a key ([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)).
- **Memory-safe edges:** network-facing, key-handling and parser-heavy code (the vault, the chain client, update
  verification, metadata extraction) is written in **Rust** where practical.

---

## 4. Graphics and the compositor: **Prism**

### The defining idea

In **Qt Wayland Compositor**, each client surface is a `QQuickItem` (`ShellSurfaceItem`). The shell, the wallpaper and
every app window share one scene graph, rendered through QRhi (Vulkan / OpenGL). That makes all of these
straightforward:

- Window open/close morphs from the exact icon or thumbnail that launched it
- Live backdrop blur behind *any* window or shell surface
- Overview / Spaces zoom with live window contents, at full frame rate
- An in-game overlay (friends, chat, performance) composed over any game by the compositor itself
- Window "stacks" (tabs at compositor level for any app)
- Mapping windows onto 3D panels in Qt Quick 3D (Spatial mode, §10)
- A compositor-level magnifier, color filters and accessibility effects implemented as shaders

```qml
// prism/compositor/Compositor.qml (sketch)
import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell

WaylandCompositor {
    id: compositor

    WaylandOutput {
        sizeFollowsWindow: true
        window: Window {
            visible: true
            Desktop { id: desktop }          // wallpaper, Spaces, Shelf, Rail, Beam…
        }
    }

    XdgShell {
        onToplevelCreated: (toplevel, xdgSurface) => desktop.spaces.current.adopt(xdgSurface)
    }

    // Prism draws title bars itself, so every window gets Lumen chrome
    XdgDecorationManagerV1 { preferredMode: XdgToplevel.ServerSideDecoration }
}
```

### What drives the hardware *(decided: [QTOS-0003](docs/decisions/qtos/QTOS-0003-compositor-strategy.md), [QOR-0005](docs/decisions/QOR-0005-every-display-at-its-own-refresh.md))*

Prism stays on Qt Wayland Compositor for the scene and the protocols, and gets **its own KMS backend**:

| Requirement | What the backend does |
|-------------|-----------------------|
| Every display at its own rate | Atomic modesetting and frame scheduling per output: a 360 Hz panel and a 60 Hz television run at once, each paced by its own vblank |
| Every GPU | NVIDIA RTX on the open kernel modules, Intel and AMD on Mesa; on hybrid laptops it renders on one GPU and scans out on another (dmabuf, PRIME) |
| VRR | Variable refresh on displays that support it, for games first |
| Games | Direct scan-out of a full-screen client, so a game's frame skips composition entirely |
| Hotplug | Displays come and go without restarting the session |

The acceptance test is the owner's requirement: on the reference laptop, its panel at 360 Hz and a television at
60 Hz together, with under 1% missed frames per output at its own rate and input to swap within two refresh periods
at p95. Qt's `eglfs_kms` drives one GPU and is a development path only. QTOS's bare-metal decision gate is deferred:
if the backend cannot meet these targets, a later record revisits Waylib for the backend only, keeping the shell.

### Protocols

Qt Wayland Compositor ships xdg-shell, xdg-decoration, linux-dmabuf, viewporter, presentation-time, text-input and
more. Prism adds the rest in C++ with `QWaylandCompositorExtensionTemplate` + `qtwaylandscanner`:

- Standard: fractional-scale, cursor-shape, idle-inhibit and presentation-time (done); tearing-control,
  color-management, session-lock (internal), screencopy (for portals), xdg-activation, pointer-constraints and
  relative-pointer (games)
- QOR OS extensions (`qtos_*`), designed to be upstreamable:
  - `qtos_material`: an app requests backdrop blur/vibrancy regions (precedent: KDE's `org_kde_kwin_blur`)
  - `qtos_hero`: an app tags a rect as a shared element so Prism can do cross-app morph transitions
  - `qtos_titlebar`: content extends under a unified title bar while Prism still owns the controls
  - `qtos_activity`: Live Activities (§6) published by apps

### PC games and X11

Most PC games, and the Steam client itself, are X11 programs run through Proton. **QOR OS runs Steam and its games
inside gamescope**, Valve's game compositor, as a Wayland client of Prism. That brings XWayland, frame limiting and
upscaling with it, and Prism presents gamescope's output with direct scan-out. Prism's own XWayland window management
follows for other X11 apps. Some anti-cheat systems block Linux; that is each publisher's decision.

### Rendering techniques that make it feel premium

- **SDF "Surface" primitive** (custom `QSGMaterial`): one shader renders continuous-curvature (squircle) shape, fill,
  hairline border, inner specular highlight and analytic soft shadow in a single batched draw. Crisp at any scale and
  cheap.
- **Shared backdrop blur**: downsampled, cached when the backdrop is static, invalidated by damage.
- **Text:** `Text.CurveRendering` for text that scales during zoom animations; native rendering for static body text.
- **Zero-copy:** client dmabufs imported directly; full-screen clients get direct scanout, and overlay planes are used
  for video and cursor.
- **Frame pacing:** present-timed rendering per output, VRR on supported panels, and pipeline caches warmed at install
  time so the first animation never stutters.
- **HDR:** QRhi already supports HDR swapchain formats. On bare KMS, Prism sets `HDR_OUTPUT_METADATA` and colorspace
  properties itself. SDR UI is composited at reference white, and only accents, media and games use HDR headroom.

---

## 5. Lumen: the design language

### Materials

| Material | Use | Recipe |
|----------|-----|--------|
| **Glass** | Shelf, Rail, Beam, Control Center, popovers, the game overlay | Blur + saturation lift + 2% blue-noise grain (prevents banding) + tint from tokens |
| **Frost** | Sidebars, inspectors inside apps | Lighter blur of the *window's own* backdrop via `qtos_material` |
| **Solid** | Content areas, documents | Opaque and calm; content comes first |
| **Lens** | Trusted system UI only | Glass + a slowly moving specular rim that apps cannot reproduce (see P5) |

### Depth

Five elevation levels. Each level is a *pair* of SDF shadows (a tight contact shadow plus a wide ambient one) and a
stronger backdrop blur. Focused windows sit one level above unfocused ones, which gives a subtle but readable sense of
where focus is.

### Motion

- **Springs instead of durations.** `Spring` in C++ (ticked from the scene graph's animation driver) with `response` /
  `damping` parameters. Retargeting mid-flight keeps velocity, and gestures write velocity into it directly.
- **Presets:** `snappy` (controls, hover), `smooth` (windows, sheets), `bouncy` (playful, rare: drops onto
  Shelf/Stash), `gentle`.
- **Choreography:** staggered entries (about 18 ms per item), a shared-axis rule for navigation (forward = slide left +
  fade, back = reverse), and container transforms for anything that opens from a visible element.
- **Reduce Motion** remaps every spring to a short crossfade through one global token, so no per-component handling is
  needed.

### Shape, type, color, iconography, sound

- **Shape:** continuous-curvature corners everywhere (SDF squircle). Radii scale with element size, and nested radii
  follow `inner = outer − padding`.
- **Type:** **Inter** (variable, with optical sizing) for UI, **JetBrains Mono** for code. Type scale for desktop:
  11 / 12 / 13 / 15 / 17 / 22 / 28 / 34.
- **Color:** tokens defined in **OKLCH** and generated into sRGB and Display-P3. Light ↔ dark transitions interpolate
  in OKLCH, so there is no muddy midpoint.
- **Icons:** one vector icon family drawn on a 24-unit grid, converted to native QML shapes.
- **Sound:** a small, quiet palette of UI sounds, and an audio engine whose analysis the interface reacts to.
- **Grid:** 4 px base unit. Spacing scale 4 / 8 / 12 / 16 / 24 / 32 / 48.

### One source of truth for tokens *(decided: [QTOS-0009](docs/decisions/qtos/QTOS-0009-oklch-tokens-single-source.md))*

`kit/tokens/lumen.tokens.json` is generated into a `Tokens.qml` singleton, the Widgets QStyle's palette and metrics,
and the design docs. **Demiurge's one look** (obsidian and gilt, Cinzel and Barlow; ADR-108), which QQ, ARQADE and
QOR Media carry in `docs/design/tokens.json`, is mapped onto Lumen's roles in Phase 1, so the whole system again
has one source.

### Signature moments (the demo reel)

1. **Unlock:** the wallpaper's depth-of-field pulls from blurred to sharp as you authenticate, and windows rise into
   place from where they were.
2. **Launch:** an app window grows out of its Shelf icon on a `smooth` spring, gaining sharpness as it grows.
3. **Overview:** a three-finger swipe up tracks your fingers 1:1. Windows fly to a grid, and the wallpaper recedes
   with a slight parallax.
4. **Beam:** results stream in with staggered springs. Arrowing through them morphs the preview pane.
5. **Theme change:** light/dark sweeps across the screen as a soft radial wave of OKLCH interpolation.
6. **Hero handoff:** a photo double-clicked in Files expands smoothly into its viewer (`qtos_hero`).
7. **Into the game:** from Home, a game's card grows to fill the screen and hands the display to the game at its own
   refresh rate; the Guide button brings the overlay back over it.

---

## 6. The shell experience

| Surface | Concept |
|---------|---------|
| **Home** (controller first) | Where a controller lands: recent games and creations, friends online, what's new in ARQADE. Every action has a button and a glyph from the controller in use. |
| **Library** | Every game and creation in one place: QQ games, ARQADE, Steam titles, media; filters by source and by who made it. |
| **Game overlay** (Guide button) | Drawn by Prism over any game: friends and chat, performance, screenshots and recording, volume, quitting the game. |
| **Rail** (top) | Left: app name + optional global menu. Center: **Live Activities** capsule (media, timers, downloads, a game's session). Right: status indicators, which open Control Center. |
| **Shelf** (bottom, floating) | Pinned + running apps, live window previews, progress rings and badges. Hides when a window touches it and reappears with a nudge at the edge. |
| **Stash** (end of Shelf) | A drag-and-drop pocket: drop files, text or images in, switch Spaces or apps, then drag them out again. |
| **Beam** (Super / Ctrl+Space) | Universal command bar: apps, games, files, settings (deep links), window switching, calculator, and **app actions** (intents). |
| **Spaces** | A horizontal strip of workspaces with three-finger swipe between them. Named Spaces become **Projects** that relaunch their apps and restore state. |
| **Windows** | Floating by default with magnetic snapping, keyboard tiling, and **Stacks** (tabs for any app). |
| **Overview** | Semantic zoom: windows → Spaces → **Timeline** (recent files, apps and activity by time). |
| **Control Center** | Modular tiles: Wi-Fi/Bluetooth, per-app volume and output picker (PipeWire), brightness/Night Light, Focus, controllers, recording, accessibility shortcuts. |
| **Notifications** | Compatible with `org.freedesktop.Notifications`. Grouped by app and thread, with inline reply and actions. **Focus** modes and a game mode filter them. |
| **Greeter & Lock** | `greetd` + Prism in greeter mode, signing in to the machine and, once, to QOR ID. The lock screen is compositor-owned, so a crashing client can never reveal the session. |
| **Quick Look** | Spacebar preview anywhere: images, video, PDF, code, **3D models** and QQ scenes. |

**Gestures (trackpad and touch):** 3-finger up = Overview · 3-finger left/right = Spaces · all tracked 1:1 with
spring handoff. **Controller:** the left stick drives a Lumen cursor, the D-pad moves focus, North opens Beam, Guide
goes Home or opens the overlay.

---

## 7. System services

All first-party daemons are **Qt Core** processes (C++20), or **Rust** where they handle keys or parse untrusted
input. Each is socket-activated, sandboxed by systemd, and costs nothing while idle.

| Service | Responsibility | Built on |
|---------|----------------|----------|
| `qtos-sessiond` | Session lifecycle, autostart, app state save/restore, Projects | Qt SCXML state machine |
| `qtos-settingsd` | Typed, schema-validated settings with change notifications; per-app scoping | SQLite + Qt Remote Objects |
| `qtos-notifyd` | Notifications, Focus rules, history | freedesktop D-Bus API + QtRO to the shell |
| `qtos-netd` | Wi-Fi, Ethernet, VPN, cellular | NetworkManagerQt |
| `qtos-btd` | Bluetooth pairing, audio devices, controllers, battery levels | BluezQt |
| `qtos-audiod` | Per-app volume, routing, device switching | PipeWire / WirePlumber |
| `qtos-powerd` | Battery health, power profiles, inhibitors, lid and suspend policy | UPower, power-profiles-daemon, logind |
| `qtos-portald` | `xdg-desktop-portal` backend: file chooser, screenshot, screencast, permissions | UI rendered by Prism in the trusted layer |
| `qtos-updated` | OS (bootc), firmware (fwupd), apps and games (Flatpak/qpk) | Rust for signature verification |
| **The vault service** | QOR ID profiles, wallet keys, approvals; signs only after an approval Prism draws | **The QOR core** (Rust), from demiurge-chain at a pinned commit |
| **The chain client** | Reads and submits to the Demiurge chain: assets, licences, royalties, publishing | The QOR core |
| **The Library index** | Games, creations and files, searchable (SQLite FTS5) | The QOR core |
| **The Mesh node** | Peer-to-peer distribution of games and creations; off until turned on | The QOR core (iroh) |
| `qtos-sensord` | Auto-rotate, ambient light, lid angle | Qt Sensors, iio-sensor-proxy |
| `qtos-backupd` | btrfs snapshots, Rewind, external backup | btrfs + Qt Core |
| `qtos-assistd` *(optional, later)* | On-device models (§10) | Qt gRPC API around a local inference runtime |

The services that come from the QOR core reach the rest of the system only through the core's C bridge, which takes
one JSON request and gives one JSON answer, and never answers with a key
([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)). Names for those services are chosen with
the owner when they are built.

### IPC: two mechanisms, not five *(decided: [QTOS-0006](docs/decisions/qtos/QTOS-0006-service-ipc-qt-remote-objects.md))*

- **Qt Remote Objects** between shell, apps and our services. Properties replicate, so QML binds to them directly.
- **D-Bus** only where freedesktop interoperability requires it (notifications, portals, MPRIS, Secret Service).

### The QOR network

QOR OS talks to the services Demiurge runs, which stay public in demiurge-chain under its own rules
([QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md)):

| Service | What QOR OS uses it for |
|---------|-------------------------|
| **QOR ID** | One sign-in for the OS and its apps (OAuth 2.1), levels, avatars and XP across every game |
| **Social** | Chat, rooms, friends and World, in the shell and the game overlay |
| **The Demiurge chain** | Provenance for published games and creations; licences, remixes and royalties. A devnet with test value only |
| **The Mesh** | Distributing games and creations peer to peer, with storage proofs |
| **ARQADE's service** | Rankings, arenas and replay checks for ARQADE's games |

**Money:** QOR OS 1.0 moves no money: free items and test CGT only. Amounts are integer embers, never floating point,
and no investment language appears anywhere
([QOR-0006](docs/decisions/QOR-0006-qor-os-1-0-moves-no-money.md)).

---

## 8. App and game platform

### QTOS Kit (`import QTOS.Kit`)

- **Lumen style for Qt Quick Controls 2.** Any app that writes plain `import QtQuick.Controls` gets Lumen
  automatically.
- **Higher-level components:** Sidebar, Inspector, Toolbar, SearchField, Sheet, Popover, Toast, Empty State, adaptive
  layouts.
- Compiled with **qmlsc** ahead of time, linted with `qmllint` in CI.

### Making *every* Qt app native

- **`QPlatformTheme` plugin (`qtos`)**: fonts, palette, icon theme, native dialogs (via portals), reduce-motion,
  scroll physics.
- **Lumen `QStyle`** for Qt Widgets apps (Krita, OBS, Wireshark, VLC, KeePassXC…).

### Games

- **QQ games** are packages of QML scenes, logic files and assets, played by the QQ runtime. Logic runs in a
  confined engine; the game process also runs in the OS sandbox.
- **ARQADE** plays its own games and arenas on QQ's runtime, against ARQADE's service.
- **PC games** run through Steam and Proton inside gamescope (§4), from the Library like any other game.
- **Controllers** work everywhere: XInput, DualShock 4 and DualSense over HID (USB and Bluetooth), and evdev, with
  rumble, light bars and battery, through QTOS.Input.

### App model

- **Manifest:** identity, capabilities/permissions, file handlers, URL schemes, **intents** (actions exposed to Beam
  and the share sheet), Live Activities, widgets.
- **Packaging:** signed `.qpk` packages for first-party and SDK apps and QQ games; **Flatpak** for the existing
  ecosystem (Flathub). Both are sandboxed with bubblewrap and mediated by `qtos-portald`.
- **Store:** signed catalogues and developer-signed packages, verified by platform crypto (stage 1 exists). Free items
  only in 1.0.
- **Widgets:** apps supply *data* and choose from a vetted set of QML widget templates the shell renders.

### Languages

| Language | Where |
|----------|-------|
| **QML / JavaScript** | All UI: shell, apps, widgets, QQ scenes and game logic |
| **C++20** | Prism, scene-graph nodes, the KMS backend, QQ's runtime, Qt integrations, services |
| **Rust** | The QOR core: the vault, the chain client, the Mesh, the Library index; update verification |
| **Python** | Tooling (token generator, test harnesses, the Store's catalogue builder) |
| **GLSL → QSB** | Materials, blur, transitions, QQ surfaces (via Qt Shader Tools) |
| **CMake / Containerfile** | Build and image |

### SDK and tooling

- **Qt Creator** plugin: app and QQ game templates, manifest editor, deploy-to-device, and the **simulator** (the shell
  plus simulation backends in a window on Windows, macOS or Linux).
- **QQ Studio** as the game maker's tool, with an agent over MCP.
- **Qt Design Studio** for designers, connected to the token pipeline.

---

## 9. Apps

### First-party

| App | Notes / key Qt modules |
|-----|------------------------|
| **QQ Studio** | Builds QQ games in 3D and 2D: lit worlds, physics, particles, sound, animation, prefabs, undo, a logic editor, Play and Stop in place; an agent over MCP (Qt Quick 3D, Qt Multimedia) |
| **QQ Player** | Plays a QQ game on its own, natively and in a browser (Qt for WebAssembly) |
| **ARQADE** | The arcade: games, arenas, rankings and the player's Sparks (test value), in 3D (Qt Quick 3D) |
| **QOR Media** | Plays every format FFmpeg reads, views pictures, edits text, reads, marks and fills PDFs (Qt Multimedia, Qt PDF, PDFium) |
| **Files** | Column, grid and gallery views, tags, Quick Look, Rewind integration |
| **Settings** | Search-first, every pane deep-linkable (`qtos://settings/display/night-light`) |
| **Music** | The audio engine's player: library, equaliser, the audio-reactive interface |
| **Terminal** | GPU-rendered QML terminal, Lumen glass |
| **Monitor** | Qt Graphs, per-app energy and frame time |
| **Store** | One front end over `.qpk` + Flatpak, with editorial pages |
| **Browser** | Qt WebEngine (Chromium) |
| **Installer** | Installs the image to disk, with a Lumen theme |

### Adopted Qt apps (curated, themed by the platform plugin)

Kate (text), Okular (documents), Krita (painting), Kdenlive (video editing), OBS Studio (streaming), MuseScore
(notation), Qt Creator (IDE), KeePassXC (passwords), Wireshark, VLC, KDE Connect.

---

## 10. What makes it next-generation

1. **The game and the desktop are one scene.** Games scan out directly at their display's rate, and the overlay,
   notifications and chat are composed by the same scene graph as the desktop when they appear.
2. **Making games is built in.** QQ Studio, its agent and the Player ship with the OS. A game made on QOR OS is a
   folder of QML that diffs line by line and plays in a browser.
3. **One account, one library.** QOR ID carries a person's levels, friends and creations across every game and
   device; the Library holds QQ, ARQADE and Steam games together.
4. **Provenance for creations.** Publishing records who made a work and who holds a licence on the QOR network, so a
   remix can pay its sources once real value exists.
5. **Spatial mode.** Because windows are scene-graph textures, Qt Quick 3D can place them on panels in 3D space; with
   **Qt Quick 3D Xr** the same desktop runs in a headset.
6. **On-device intelligence (opt-in, local-first).** `qtos-assistd` hosts local models; the assistant acts **only
   through declared intents** and asks for confirmation.
7. **Rewind.** A btrfs snapshot timeline for any folder, plus system-level undo for settings changes.
8. **Cross-app shared-element transitions** (`qtos_hero`) and **Trusted UI** users can learn to recognise.

---

## 11. Accessibility and internationalization

- **From the first commit:** `Accessible` attached properties on every Kit component, enforced by `qmllint` rules and
  CI checks. AT-SPI bridge on Linux.
- **Screen reader:** a Qt-native reader (Qt TextToSpeech + AT-SPI over QtDBus). Orca is the interim fallback.
- **Compositor-level aids:** a magnifier, color filters, pointer and focus highlighting. All are shaders.
- **Preferences** honored system-wide: Reduce Motion, Reduce Transparency (Glass → Solid), Increase Contrast, larger
  text with real reflow.
- **Controllers as an accessibility tool:** everything in the shell is reachable with a controller and an on-screen
  keyboard.
- **i18n:** Qt Linguist workflow, HarfBuzz shaping, full RTL mirroring, input methods via fcitx5 or Qt Virtual
  Keyboard; on-device translation for Social.

---

## 12. Performance budgets (targets on the reference laptop)

The reference laptop is a Dell G15 5530: Intel Core i7-13650HX, RTX 4060 Laptop and Intel UHD graphics, a
1920×1080 360 Hz panel.

| Metric | Target |
|--------|--------|
| Power button → greeter | ≤ 5 s (NVMe) |
| Login → interactive desktop | ≤ 700 ms |
| Every connected display | Its own native mode and refresh rate (60–360 Hz tested together), < 1% missed frames |
| Compositor frame at 4K @ 120 Hz (shell animating) | GPU ≤ 3 ms, CPU ≤ 2 ms |
| Pointer or controller input → photon | ≤ 1 frame + scanout |
| Full-screen game | Direct scan-out; zero shell frames while it runs |
| First-party app: launch → first frame | ≤ 300 ms (quick-launch + AOT-compiled QML) |
| Beam: keystroke → updated results | ≤ 16 ms |
| Shell idle memory | ≤ 250 MB RSS |
| Idle wakeups with nothing animating | ~0 (render on demand) |

Budgets are checked by **frame-time CI on real hardware**. A change that blows a budget fails the build. Today's
measurements are in [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

---

## 13. Engineering

### Repository layout (monorepo) *(see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md))*

```text
qor-os/
├─ kit/              QTOS.Kit: Lumen tokens, Theme, Spring, OKLCH, SDF Surface, Glass, Backdrop, icons, fonts
│  └─ style/         Lumen Qt Quick Controls style
├─ input/ audio/     controllers; the audio engine
├─ services/ sim/    service contracts (interfaces/*.rep), QTOS.Services; simulated services, qtos-simd
├─ store/ apps/      the signed Store; first-party apps
├─ prism/
│  ├─ shell/         QTOS.Shell: Rail, Shelf, Beam, Control Center, windows, Spaces, Overview, Lock
│  ├─ host/          qtos-shell: simulation host
│  └─ compositor/    prism: the Wayland compositor (Linux)
├─ platform/         Lumen QStyle, qtos QPA platform theme, widgets gallery
├─ core/             the QOR core, built from demiurge-chain at a pinned commit (Rust, Corrosion)
├─ products/
│  ├─ qq/            QQ: runtime, Studio, Player, agent (carried, Apache-2.0)
│  └─ arqade/        ARQADE's Qt app, games and fixtures (carried, Apache-2.0)
├─ tools/            qtosctl, generators, dev, CI and upstream scripts; qor-media/ (QOR Media, carried)
├─ image/            the Fedora Atomic image (Phase 3)
├─ sdk/              app template
├─ tests/            Qt Test, Qt Quick Test, Python
└─ docs/             architecture, building, development, design specs, decisions
```

Planned additions: `prism/backend-kms/` (Prism's KMS backend, Phase 3) and `prism/protocols/` (`qtos_*` Wayland
extensions).

### Build and toolchain *(decided: [QTOS-0005](docs/decisions/qtos/QTOS-0005-qt-6-12-baseline.md), [QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md))*

- CMake presets + **Qt 6.12** (commercial kit); move to the next LTS when validated.
- `qt_add_qml_module` everywhere, so QML is AOT-compiled with `qmlsc` and type-checked with `qmllint` (zero warnings).
- **Rust** (1.91+) builds the QOR core through Corrosion.
- The carried products keep their own builds (`tools/dev/products.ps1`).
- bootc builds images; `systemd-sysext` handles fast on-device iteration.

### Testing

- **Qt Test / QML TestCase** for units. **Golden-image tests** for every Kit component in light, dark, high-contrast
  and RTL.
- **End-to-end:** scripted shell runs (`shell_e2e`) and a real compositor with real clients (`prism_e2e`).
- **QQ, ARQADE and QOR Media** keep their suites, rendering on the GPU and reading back pixels; ARQADE's games replay
  recorded runs.
- **Fuzzing** for protocol handlers, metadata extractors and update parsing.

### Dev loop from this Windows machine

1. **Simulation mode (native Windows):** the shell, apps and products run directly with Qt for Windows and the
   simulation backends. Most design work happens here.
2. **Nested compositor (WSL2 + WSLg):** the real Prism runs as a Wayland client of WSLg with real Qt apps inside it.
3. **Full image in a VM:** QEMU with 3D acceleration to test boot, greeter and updates.
4. **Reference hardware:** the Dell G15 5530 (RTX 4060 + Intel UHD, 360 Hz panel) with a 60 Hz television, plus RTX
   and Intel desktops: where displays, VRR, drivers and games get tested.

---

## 14. Licensing *(decided: [QTOS-0004](docs/decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md), [QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md), [QOR-0008](docs/decisions/QOR-0008-before-a-public-release.md))*

Several key Qt modules are **GPLv3 or commercial, not LGPL**, including Qt Wayland Compositor, Qt Application Manager,
Qt Virtual Keyboard, Qt Quick 3D, Qt Quick 3D Physics and Qt Graphs. QOR-MATRIX holds a **Qt Enterprise** commercial
license, so:

- QOR OS is developed on the **commercial Qt** kit, including commercial tooling such as `qmlsc`.
- The repository is **private** and the code **proprietary** (`LICENSE`).
- Code carried from demiurge-chain (QQ, ARQADE, QOR Media) remains **Apache-2.0** with its notices.
- Third-party assets are permissively licensed (OFL fonts, ISC/MIT icons) and listed in `THIRD_PARTY_NOTICES.md`.
- **Distributing** Qt inside a free-to-download OS image and SDK needs The Qt Company's written confirmation before
  any public release (QOR-0008).

---

## 15. Precedents to study

| Project | Lesson |
|---------|--------|
| **SteamOS** (Arch, gamescope) | A controller-first gaming OS that runs Windows games through Proton; gamescope as the game's compositor. |
| **Bazzite** (Fedora Atomic, Universal Blue) | Image-based gaming OS with NVIDIA images and rollback: the base QOR OS builds on. |
| **Sailfish OS** (Lipstick on Qt Wayland Compositor) | A shipped, gesture-first, all-Qt mobile OS. Proof that the core idea works. |
| **Liri** | A desktop shell on Qt Wayland Compositor. Study its scope and where it stalled. |
| **Deepin Treeland** (Waylib) | A Qt Quick scene on wlroots: the fallback for the backend. |
| **KDE Plasma 6 / KWin** | The reference for HDR, VRR, color management, XWayland and fractional scaling on Qt 6. |
| **Roblox, Dreams, Core** | Platforms where players become makers; what an engine inside a platform needs. |

Lesson shared by all of them: **scope is the killer.** QOR OS reuses Fedora, gamescope, Proton and KDE Frameworks, and
spends its own effort on Prism, Lumen, QQ and the first-party experience.

---

## 16. Roadmap (phases with exit criteria)

The detail, with every item, is [docs/ROADMAP.md](docs/ROADMAP.md).

| Phase | Goal | Exit |
|-------|------|------|
| **0 · Clear the ground** | One repository, the products carried, the core pinned, one rulebook | Builds and tests in CI on Windows and Linux, products included |
| **1 · One stack** | QQ, ARQADE and QOR Media on Linux inside Prism; controllers; Lumen everywhere; Home and Library | Orb Run plays inside Prism on Linux with a DualSense |
| **2 · Identity and keys** | The vault as a service, one QOR ID sign-in, Social and the Library index in the shell | Sign in once, chat in World, publish Orb Run on the devnet from QQ Studio inside Prism |
| **3 · Boots and plays PC games** | The Fedora Atomic image, Prism's KMS backend, Steam through gamescope, the greeter, updates with rollback | The reference laptop boots QOR OS at 360 Hz on its panel and 60 Hz on a television, and plays a Steam game and Orb Run |
| **4 · The ecosystem** | The public SDK, one catalogue, the Store's developer program, the premium shell features | An outside developer ships a QQ game to QOR OS with only the public SDK |

---

## 17. Top risks

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Prism's KMS backend (several GPUs, per-output refresh, VRR) takes far longer than planned | Blocks booting on gaming laptops | Backend-agnostic shell; the requirement written as one acceptance test; Waylib as a fallback for the backend only |
| Scope: an OS plus seven products for one founder with agents | Never ships | QOR-0007 fixes 1.0; Phases 1 and 3 are the ones that make QOR OS real; reuse Fedora, gamescope, Proton |
| Game compatibility | Players can't run their games | Proton and gamescope; anti-cheat is the publishers' decision, said plainly |
| Keys on an operating system | Loss of people's assets | One source in the QOR core, a vault service outside every app, approvals in the trusted layer, an external review before value |
| Hosting of QOR ID and Social on one provider in one region | People can't sign in | Offline login, then a second region, before QOR ID signs people in to the OS |
| Licensing: Qt for a distributed OS, trademarks, code signing | Legal and product risk | Release gates in QOR-0008, settled before any public image |
| Two-way flow with demiurge-chain | Drift or conflicts | Same paths, `tools/upstream/demiurge.sh`, a cut-over record when QOR OS becomes where the products ship |

---

## 18. Decisions

| Question | Decision | Record |
|----------|----------|--------|
| Decision log and rules | One rulebook (`AGENTS.md`), one log, three series | [QOR-0001](docs/decisions/QOR-0001-one-rulebook-and-one-decision-log.md) |
| Where the code lives | Private `qor-os` from QTOS's history; QQ, ARQADE, QOR Media carried; the network stays public | [QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md) |
| Code that handles keys | The QOR core, from demiurge-chain at a pinned commit, never copied | [QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md) |
| Base image | Fedora Atomic with bootc; NVIDIA open modules; Mesa | [QOR-0004](docs/decisions/QOR-0004-the-base-image-is-fedora-atomic.md) |
| Displays and the compositor's backend | Every display at its own rate on every GPU; Prism's own KMS backend; the gate deferred | [QOR-0005](docs/decisions/QOR-0005-every-display-at-its-own-refresh.md), [QTOS-0003](docs/decisions/qtos/QTOS-0003-compositor-strategy.md) |
| Money | 1.0 moves no money; free items and test CGT | [QOR-0006](docs/decisions/QOR-0006-qor-os-1-0-moves-no-money.md) |
| 1.0 scope | Shell, Home and Library, QQ, ARQADE, QOR Media, Social, the vault, Steam | [QOR-0007](docs/decisions/QOR-0007-what-1-0-ships.md) |
| Public release; naming | Release gates; the code keeps QTOS names until the name is cleared | [QOR-0008](docs/decisions/QOR-0008-before-a-public-release.md) |
| License posture | Commercial Qt Enterprise; private, proprietary repository | [QTOS-0004](docs/decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md) |
| Qt baseline | Qt 6.12 | [QTOS-0005](docs/decisions/qtos/QTOS-0005-qt-6-12-baseline.md) |
| Service IPC | Qt Remote Objects; D-Bus only at freedesktop edges | [QTOS-0006](docs/decisions/qtos/QTOS-0006-service-ipc-qt-remote-objects.md) |
| Lumen controls | Qt Quick Controls style on Qt Quick Templates | [QTOS-0007](docs/decisions/qtos/QTOS-0007-lumen-style-on-quick-templates.md) |
| Development model | Simulation-first on Windows, then nested Prism, then the image | [QTOS-0008](docs/decisions/qtos/QTOS-0008-simulation-first-development.md) |
| Design tokens | OKLCH single source, generated for QML, C++ and docs | [QTOS-0009](docs/decisions/qtos/QTOS-0009-oklch-tokens-single-source.md) |
| Materials and rendering | Shared backdrops, SDF primitives, render on demand | [QTOS-0010](docs/decisions/qtos/QTOS-0010-materials-and-render-on-demand.md) |
| Naming | QOR OS (product); Prism (compositor and shell), Lumen (design language), QQ (engine), ARQADE (arcade), QOR Media; Beam, Shelf, Rail, Stash | This document, QOR-0008 |

---

## 19. First steps *(Phase 0)*

1. ✅ Free the disk space the work needs.
2. ✅ Create `qor-os` from QTOS's full history, and freeze QTOS.
3. ✅ Carry QQ, ARQADE and QOR Media with their history; build and test them here.
4. ✅ Build the QOR core from demiurge-chain at a pinned commit, tested through its bridge.
5. ✅ One rulebook and one decision log, QOR-0001 to QOR-0008.
6. ⏭ CI green on Windows and Linux, with runners shared across the organisation.
7. ⏭ Move the core's pin to Demiurge's L13.2, then start Phase 1: the products inside Prism on Linux.
