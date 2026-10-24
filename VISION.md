# QTOS: Vision & Architecture

> A next-generation operating system where every pixel a person sees is drawn by Qt 6.
>
> **Status:** living vision. Phase 0 is implemented (see [README](README.md) and
> [docs/ROADMAP.md](docs/ROADMAP.md)). Decisions are recorded as ADRs in
> [docs/adr/](docs/adr/). Sections marked *(decided)* link to the ADR.

---

## 0. The pitch in six lines

1. **Linux kernel underneath, Qt everywhere above it.** A Qt 6 compositor, shell, services, SDK and app suite on a small, immutable, verified base.
2. **The shell is the compositor.** Windows are live items in one Qt Quick scene graph, so every window can be animated, blurred, morphed, tiled or put in 3D space with QML and shaders. This is the main thing that sets QTOS apart.
3. **Lumen**, a design language built on physically inspired materials, spring motion, continuous-curvature shapes, perceptual color and HDR-aware highlights.
4. **Every Qt app looks native on day one.** A platform theme plugin and a matching QStyle give Lumen to thousands of existing Qt apps (KDE, OBS, Krita, Wireshark, VLC, etc.).
5. **Secure and boring underneath.** Signed boot chain, read-only verified root, A/B updates with automatic rollback, sandboxed apps, and system prompts drawn by the compositor so apps can't fake them.
6. **One shell for every form factor.** Desktop, tablet, handheld, kiosk and XR all come from one adaptive QML codebase, which Qt's embedded heritage makes realistic.

---

## 1. Principles

| # | Principle | What it means in practice |
|---|-----------|---------------------------|
| P1 | **The Qt Line** | Everything user-facing is Qt. Below the line, only *headless* infrastructure is allowed, and only behind a Qt service API. |
| P2 | **Motion is meaning** | Every transition has spatial continuity: things come from somewhere and go somewhere. All animations can be interrupted and keep their velocity when retargeted. |
| P3 | **Direct manipulation** | Gestures track 1:1 with fingers or trackpad, then hand velocity to springs. Nothing waits for a fixed-duration animation to finish. |
| P4 | **Render on demand** | Zero frames, zero wakeups when nothing changes. A premium feel also means good battery life. |
| P5 | **Trust is visible** | Camera, mic, screen capture, location and permission prompts live in a compositor-owned layer that apps can't draw over. |
| P6 | **Local-first** | No account required. Cloud features use open protocols (CalDAV/CardDAV/JMAP/WebDAV). Intelligence features run on the device. |
| P7 | **Leverage, don't rewrite** | Reuse the mature Qt ecosystem (KDE Frameworks, Qt Application Manager, Calamares, KDE Connect) wherever it doesn't compromise the experience. |

---

## 2. The stack at a glance

```text
┌────────────────────────────────────────────────────────────────────┐
│ APPS        First-party QML apps · adopted Qt apps · Flatpaks      │
│ PLATFORM    QTOS Kit · Lumen QQC2 style · QStyle · platform theme  │  100% Qt
│ SHELL       Prism: compositor + shell in ONE Qt Quick scene graph  │
│ SERVICES    qtos-*d daemons (Qt Core, C++ / Rust via CXX-Qt)       │
│             internal: Qt Remote Objects · external: D-Bus          │
╞═══════════════════════════ THE QT LINE ════════════════════════════╡
│ SUBSTRATE   systemd + logind · PipeWire/WirePlumber · Network-     │  headless,
│             Manager · BlueZ · UPower · fwupd · CUPS · Mesa ·       │  wrapped by
│             libinput · HarfBuzz/FreeType/fontconfig                │  Qt services
│ KERNEL      Linux LTS: DRM/KMS · evdev · Landlock · KVM · btrfs    │
│ BOOT        UEFI Secure Boot · signed UKI · TPM2 · A/B slots       │
└────────────────────────────────────────────────────────────────────┘
```

**Why not "Qt all the way down"?** Qt is not a kernel and should not be one. (Qt for MCUs targets bare-metal microcontrollers, not desktop-class hardware.) The Linux kernel provides drivers for real hardware, and that is not something to recreate. The aim is a uniform *experience*, not purity for its own sake.

---

## 3. Foundation: image, boot, updates, security

### Image and build

- **Yocto Project + `meta-qt6`** (the same foundation as Qt's own Boot2Qt) and our own `meta-qtos` distro layer. Reproducible builds, a full SBOM, and control over every package.
- **Root filesystem:** read-only EROFS image protected by **dm-verity**. User data lives on **btrfs** (zstd compression, snapshots for Rewind; see §10).
- **Encryption:** LUKS2 full-disk encryption, unlocked by TPM2 + PIN (`systemd-cryptenroll`), with a recovery key generated at setup.
- **Developer overlay:** `systemd-sysext` images let a developer drop a freshly built compositor or service onto a running device without reflashing.

### Boot

- UEFI Secure Boot → signed **Unified Kernel Image** → measured boot into TPM2 PCRs.
- **Flicker-free boot:** firmware logo (BGRT) → early splash that reuses the BGRT logo → Prism takes over **without a modeset** (same mode, same framebuffer contents) → greeter. The handoff should feel like a single continuous animation.
  - Early splash: Plymouth with a matching theme in the initramfs (a Qt runtime there is ~20–30 MB). Option for later: a minimal static Qt Quick splash on `eglfs`.

### Updates

- **OS:** A/B slots via **RAUC** (mature in the Yocto world). Signed bundles, delta updates, boot counting with automatic rollback if the new slot fails to boot.
- **Firmware:** `fwupd` (LVFS) shown in the same Software Update UI.
- **Apps:** updated independently of the OS (see §8).

### Security model

- **Wayland isolation by default:** no global key snooping and no screen scraping without the portal.
- **App sandbox:** bubblewrap namespaces + seccomp + Landlock. Permissions declared in the manifest and granted at runtime through portals.
- **Trusted UI layer:** permission prompts, password prompts and privacy indicators are drawn by Prism in a layer clients can never cover. A subtle "trusted" visual cue (for example a specular edge that only system UI can render) makes spoofing obvious.
- **Memory-safe edges:** network-facing and parser-heavy services (update verification, secrets, metadata extraction) are written in **Rust via CXX-Qt** where practical.

---

## 4. Graphics and the compositor: **Prism**

### The defining idea

In **Qt Wayland Compositor**, each client surface is a `QQuickItem` (`ShellSurfaceItem`). The shell, the wallpaper and every app window share one scene graph, rendered through QRhi (Vulkan / OpenGL). That makes all of these straightforward:

- Window open/close morphs from the exact icon or thumbnail that launched it
- Live backdrop blur behind *any* window or shell surface
- Overview / Spaces zoom with live window contents, at full frame rate
- Window "stacks" (tabs at compositor level for any app)
- Mapping windows onto 3D panels in Qt Quick 3D (Spatial mode, §10)
- A compositor-level magnifier, color filters and accessibility effects implemented as shaders

```qml
// prism/Main.qml (sketch)
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

### The big decision: what drives the hardware

| Option | What it is | Pros | Cons |
|--------|-----------|------|------|
| **A. Qt Wayland Compositor + custom KMS backend** *(recommended)* | Pure Qt. We write a `qtos-kms` QPA/output backend: atomic KMS, per-output frame scheduling, overlay planes, dmabuf scanout | All Qt and maintained by The Qt Company; stable APIs; full design control | We must build HDR metadata, VRR, multi-GPU, hotplug, direct scanout and fractional scaling ourselves. **This is the hardest component in QTOS.** |
| B. Waylib / qwlroots (as in Deepin's Treeland) | Qt Quick scene on top of the wlroots backend | Mature DRM backend, XWayland, many protocols for free | wlroots is below the Qt Line and its API changes every release; depends on a smaller upstream |
| C. KWin (Qt 6) + our shell as clients/effects | Most mature Qt-based compositor | HDR, VRR, color management and XWayland already work | Shell and windows are no longer one scene; effects are limited to KWin's effect API, and KWin's architecture caps the design ceiling |

**Recommendation:** prototype on **A** running *nested* (Qt Wayland Compositor runs inside any existing Wayland/X11 session, including WSLg on Windows) so shell design starts immediately. Keep window-management logic in a backend-agnostic layer (QML models + C++ interfaces). At the end of Phase 1, hold a **decision gate**: if the bare-metal KMS backend isn't meeting frame-pacing and multi-monitor targets, switch to **B** and keep the shell.

### Protocols

Qt Wayland Compositor ships xdg-shell, xdg-decoration, linux-dmabuf, viewporter, presentation-time, text-input and more. We add the rest in C++ with `QWaylandCompositorExtensionTemplate` + `qtwaylandscanner`:

- Standard: fractional-scale, cursor-shape, tearing-control, color-management, idle-inhibit, session-lock (internal), screencopy (for portals), xdg-activation, pointer-constraints/relative-pointer (games)
- QTOS extensions (`qtos_*`), designed to be upstreamable:
  - `qtos_material`: an app requests backdrop blur/vibrancy regions (precedent: KDE's `org_kde_kwin_blur`)
  - `qtos_hero`: an app tags a rect as a shared element so Prism can do cross-app morph transitions
  - `qtos_titlebar`: content extends under a unified title bar while Prism still owns the controls
  - `qtos_activity`: Live Activities (§6) published by apps

### Legacy X11

There's no built-in X window manager in Qt Wayland Compositor. **Stance:** QTOS is Wayland-native. X11 apps run in **rootful Xwayland** shown as a single "Legacy" window: cheap, isolated and honest. A full XWM is a later, optional project (or it comes for free with option B).

### Rendering techniques that make it feel premium

- **SDF "Surface" primitive** (custom `QSGMaterial`): one shader renders continuous-curvature (squircle) shape, fill, hairline border, inner specular highlight and analytic soft shadow in a single batched draw. Crisp at any scale and cheap.
- **Dual-Kawase backdrop blur** as a `QSGRenderNode`: downsampled, cached when the backdrop is static, invalidated by damage.
- **Text:** `Text.CurveRendering` (Qt 6.7+) for text that scales during zoom animations; native rendering for static body text.
- **Zero-copy:** client dmabufs imported directly; full-screen clients get direct scanout, and overlay planes are used for video and cursor.
- **Frame pacing:** present-timed rendering per output, VRR on supported panels, and pipeline caches warmed at install time (`QQuickGraphicsConfiguration` pipeline cache) so the first animation never stutters.
- **HDR:** QRhi already supports HDR swapchain formats. On bare KMS, Prism sets `HDR_OUTPUT_METADATA` and colorspace properties itself. SDR UI is composited at reference white, and only accents and media use HDR headroom, subtly.

---

## 5. Lumen: the design language

### Materials

| Material | Use | Recipe |
|----------|-----|--------|
| **Glass** | Shelf, Rail, Beam, Control Center, popovers | Dual-Kawase blur + saturation lift + 2% blue-noise grain (prevents banding) + tint from tokens |
| **Frost** | Sidebars, inspectors inside apps | Lighter blur of the *window's own* backdrop via `qtos_material` |
| **Solid** | Content areas, documents | Opaque and calm; content comes first |
| **Lens** | Trusted system UI only | Glass + a slowly moving specular rim that apps cannot reproduce (see P5) |

```qml
// kit/Lumen/Glass.qml (prototype path; production uses the C++ blur node)
import QtQuick
import QtQuick.Effects

Item {
    id: root
    required property Item backdrop
    property real radius: Tokens.radius.l

    ShaderEffectSource {
        id: behind
        sourceItem: root.backdrop
        sourceRect: root.mapToItem(root.backdrop, 0, 0, root.width, root.height)
        visible: false
    }
    MultiEffect {
        anchors.fill: parent
        source: behind
        blurEnabled: true; blur: 1.0; blurMax: 48
        saturation: 0.35; brightness: 0.04
        maskEnabled: true; maskSource: shape
    }
    Surface { id: shape; anchors.fill: parent; radius: root.radius; layer.enabled: true }
}
```

### Depth

Five elevation levels. Each level is a *pair* of SDF shadows (a tight contact shadow plus a wide ambient one) and a stronger backdrop blur. Focused windows sit one level above unfocused ones, which gives a subtle but readable sense of where focus is.

### Motion

- **Springs instead of durations.** A `Lumen.Spring` type in C++ (ticked from the scene graph's animation driver) with `response` / `damping` parameters. Retargeting mid-flight keeps velocity, and gestures write velocity into it directly.

```qml
Shelf {
    id: shelf
    y: root.height - height + reveal.value
    Spring {
        id: reveal
        target: shelf.revealed ? 0 : shelf.height + 12
        preset: Motion.snappy          // response 0.28 s, damping 0.90
    }
}
```

- **Presets:** `snappy` (controls, hover), `smooth` (windows, sheets), `bouncy` (playful, rare: drops onto Shelf/Stash).
- **Choreography:** staggered entries (about 18 ms per item), a shared-axis rule for navigation (forward = slide left + fade, back = reverse), and container transforms for anything that opens from a visible element.
- **Reduce Motion** remaps every spring to a short crossfade through one global token, so no per-component handling is needed.

### Shape, type, color, iconography, sound

- **Shape:** continuous-curvature corners everywhere (SDF squircle). Radii scale with element size, and nested radii follow `inner = outer − padding`.
- **Type:** **Inter** (variable, with optical sizing) for UI, **JetBrains Mono** for code. Qt 6.7 variable font axes let weight animate on press/focus. Type scale for desktop: 11 / 12 / 13 / 15 / 17 / 22 / 28 / 34.
- **Color:** tokens defined in **OKLCH** and generated into sRGB and Display-P3. Accent color is derived from the wallpaper with perceptual contrast checks (APCA). Light ↔ dark transitions interpolate in OKLCH, so there is no muddy midpoint.
- **Icons:** one vector icon family drawn on a 24-unit grid, converted with **Qt VectorImage** (`svgtoqml`) to native QML shapes. State changes are animated symbol transitions (Qt Lottie for the expressive ones).
- **Sound:** a small, quiet palette of UI sounds placed spatially with **Qt Spatial Audio** (a notification sounds slightly from where its banner appears). Off by default for clicks, on for meaningful events.
- **Grid:** 4 px base unit. Spacing scale 4 / 8 / 12 / 16 / 24 / 32 / 48.

### One source of truth for tokens

```json
{
  "color": {
    "accent":  { "oklch": [0.68, 0.17, "wallpaper.dominantHue"] },
    "surface": { "light": { "oklch": [0.98, 0.005, 260], "alpha": 0.72 },
                 "dark":  { "oklch": [0.22, 0.012, 260], "alpha": 0.64 } }
  },
  "radius": { "s": 6, "m": 10, "l": 16, "xl": 24, "shape": "squircle" },
  "motion": {
    "snappy": { "response": 0.28, "damping": 0.90 },
    "smooth": { "response": 0.42, "damping": 1.00 },
    "bouncy": { "response": 0.50, "damping": 0.72 }
  }
}
```

A generator (Python) emits a `Lumen/Tokens.qml` singleton, a `QPalette` + metrics for the Widgets QStyle, CSS custom properties for WebEngine content, and the design docs. Designers work in **Qt Design Studio**, using the Figma bridge to bring Figma designs over, against the same tokens.

### Signature moments (the demo reel)

1. **Unlock:** the wallpaper's depth-of-field pulls from blurred to sharp as you authenticate, and windows rise into place from where they were.
2. **Launch:** an app window grows out of its Shelf icon on a `smooth` spring, gaining sharpness as it grows.
3. **Overview:** a three-finger swipe up tracks your fingers 1:1. Windows fly to a grid, and the wallpaper recedes with a slight parallax.
4. **Beam:** results stream in with staggered springs. Arrowing through them morphs the preview pane between file, contact and action previews.
5. **Theme change:** light/dark sweeps across the screen as a soft radial wave of OKLCH interpolation.
6. **Hero handoff:** a photo double-clicked in Files expands smoothly into the Photos window (`qtos_hero`).

---

## 6. The shell experience

| Surface | Concept |
|---------|---------|
| **Rail** (top) | Left: app name + optional global menu (DBusMenu, so Qt/KDE apps work). Center: **Live Activities** capsule (media, timers, calls, screen recording, downloads, navigation) that expands into rich controls. Right: status indicators, which open Control Center. |
| **Shelf** (bottom, floating) | Pinned + running apps, live window previews on hover, progress rings and badges on icons. Hides when a window touches it and reappears with a nudge at the edge. |
| **Stash** (end of Shelf) | A drag-and-drop pocket: drop files, text or images in, switch Spaces or apps, then drag them out again. Persists until cleared. |
| **Beam** (Super / Ctrl+Space) | Universal command bar: apps, files, settings (deep links), contacts, window switching, clipboard history, emoji, calculator/units/currency, and **app actions** (intents). Quick Look preview pane. Optional natural-language mode (§10). |
| **Spaces** | A horizontal strip of workspaces with three-finger swipe between them. Each Space remembers its layout and can tint the wallpaper. Named Spaces become **Projects** that relaunch their apps and restore state. |
| **Windows** | Floating by default with **Magnetic** snapping: dragging to an edge reveals layout zones (halves, thirds, quarters, custom). Full keyboard tiling. Drop one window onto another's title bar to make a **Stack** (tabs for any app). Window controls appear as a compact pill on hover. |
| **Overview** | Semantic zoom: windows → Spaces → **Timeline** (recent files, apps and activity by time). One continuous pinch gesture moves through the levels. |
| **Control Center** | Modular tiles: Wi-Fi/Bluetooth with inline device lists, per-app volume and output picker (PipeWire), brightness/Night Light, Focus mode, Quick Share, screen recording, accessibility shortcuts. Users can rearrange it. |
| **Notifications** | Fully compatible with `org.freedesktop.Notifications`. Grouped by app and thread, with inline reply and actions. **Focus** modes filter by person and app. Optional on-device digest. |
| **Greeter & Lock** | `greetd` + Prism in greeter mode as an unprivileged user (SDDM, which is Qt/QML, is the fallback). The lock screen is compositor-owned, so a crashing client can never reveal the session. |
| **Quick Look** | Spacebar preview anywhere: images, video (Qt Multimedia), PDF (Qt PDF), code (KSyntaxHighlighting), **3D models** (Qt Quick 3D runtime asset loading). |
| **Screenshots & recording** | Built into Prism: pixel-exact capture, window capture with shadow, region capture, recording with system/mic audio. |

**Gestures (trackpad and touch):** 3-finger up = Overview · 3-finger left/right = Spaces · 3-finger down = App Exposé · pinch = semantic zoom · edge swipes on touch devices · all tracked 1:1 with spring handoff.

---

## 7. System services

All first-party daemons are **Qt Core** processes (C++20, or Rust via CXX-Qt). Each is socket-activated, sandboxed by systemd, and costs nothing while idle.

| Service | Responsibility | Built on |
|---------|----------------|----------|
| `qtos-sessiond` | Session lifecycle, autostart, app state save/restore, Projects | Qt SCXML state machine |
| `qtos-settingsd` | Typed, schema-validated settings with change notifications; per-app scoping | SQLite + Qt Remote Objects |
| `qtos-notifyd` | Notifications, Focus rules, history | freedesktop D-Bus API + QtRO to the shell |
| `qtos-indexd` | File search and metadata | KFileMetaData extractors + SQLite FTS5 (+ optional embeddings) |
| `qtos-netd` | Wi-Fi, Ethernet, VPN, cellular | NetworkManagerQt, ModemManagerQt |
| `qtos-btd` | Bluetooth pairing, audio devices, battery levels | BluezQt |
| `qtos-audiod` | Per-app volume, routing, device switching | PipeWire / WirePlumber |
| `qtos-powerd` | Battery health, power profiles, inhibitors, lid and suspend policy | UPower, power-profiles-daemon, logind |
| `qtos-portald` | `xdg-desktop-portal` backend: file chooser, screenshot, screencast, print, permissions, settings | UI rendered by Prism in the trusted layer |
| `qtos-updated` | OS (RAUC), firmware (fwupd), apps (Flatpak/qpk) | Rust core for signature verification |
| `qtos-secretd` | Secret Service API (passwords, keys), unlocked at login, TPM-bound | Rust via CXX-Qt |
| `qtos-accountsd` | Online accounts (OAuth2), CalDAV/CardDAV/JMAP/WebDAV | Qt NetworkAuth |
| `qtos-sensord` | Auto-rotate, ambient light → auto-brightness, lid angle | Qt Sensors, iio-sensor-proxy |
| `qtos-locationd` | Location with per-app permission | Qt Positioning |
| `qtos-backupd` | btrfs snapshots, Rewind, external backup | btrfs + Qt Core |
| `qtos-assistd` *(optional)* | On-device models (§10) | Qt gRPC API around a local inference runtime |

### IPC: two mechanisms, not five

- **Qt Remote Objects** between shell, apps and our services. Properties replicate, so QML binds to them directly (`network.wifiEnabled`).
- **D-Bus** only where freedesktop interoperability requires it (notifications, portals, MPRIS, Secret Service, StatusNotifierItem).
- (Qt gRPC only for `qtos-assistd` and future companion devices.)

**Define every API once.** Write service interfaces in **QFace IDL** and let **Qt Interface Framework** generate the QML-facing frontend, a QtRO server stub, *and* a JSON-driven **simulation backend**. That last part means the entire shell runs on Windows or macOS with fake Wi-Fi networks, fake batteries and fake notifications, which makes design iteration much faster.

```text
// services/network/network.qface
module qtos.network 1.0

interface Network {
    bool wifiEnabled;
    readonly string activeSsid;
    readonly list<AccessPoint> accessPoints;
    void connect(string ssid, string passphrase);
    signal connectionFailed(string reason);
}
```

---

## 8. App platform

### QTOS Kit (`import QTOS.Kit`)

- **Lumen style for Qt Quick Controls 2.** Any app that writes plain `import QtQuick.Controls` gets Lumen automatically.
- **Higher-level components:** Sidebar, Inspector, Toolbar (unified title bar), SearchField with tokens, Sheet, Popover, Toast, Empty State, adaptive Gallery/List, Settings-page builder, and **adaptive layouts** with Desktop / Tablet / Handheld breakpoints.
- Compiled with **qmlsc / qmltc** ahead of time, linted with `qmllint` in CI.

### Making *every* Qt app native

- **`QPlatformTheme` plugin (`qtos`)**: fonts, palette, icon theme, cursor theme, native dialogs (via portals), reduce-motion, scroll physics, global menu. Every Qt 5/6 app picks it up automatically.
- **Lumen `QStyle`** for Qt Widgets apps (Krita, OBS, Wireshark, VLC, KeePassXC…). They don't get glass, but they get correct palette, metrics, radii, focus rings and typography.
- **Kirigami theme plugin** so KDE's QML apps adopt Lumen tokens.

### App model

- **Manifest:** identity, capabilities/permissions, file handlers, URL schemes, **intents** (actions exposed to Beam, Flows and the share sheet), Live Activities, widgets.
- **Lifecycle and packaging:** **Qt Application Manager** handles multi-process lifecycle, signed package installation, intents, and *quick-launch* (pre-warmed QML engines, so first-party apps open almost instantly).
- **Two package channels, one sandbox:**
  - `.qpk`: signed Qt Application Manager packages for first-party and QTOS SDK apps
  - **Flatpak** with the KDE/Qt 6 runtime for the existing ecosystem (Flathub)
  - Both are sandboxed with bubblewrap and mediated by `qtos-portald`
- **Widgets:** apps supply *data* (a timeline of JSON entries) and choose from a vetted set of QML widget templates the shell renders. Widgets stay safe and cheap, and they always look like Lumen.

### Languages

| Language | Where |
|----------|-------|
| **QML / JavaScript** | All UI: shell, apps, widgets |
| **C++20** | Prism, scene-graph nodes, KMS backend, Qt integrations, performance-critical services |
| **Rust (CXX-Qt)** | Security-critical and parser-heavy services, update verification, secrets |
| **Python (PySide6)** | Tooling (token generator, test harnesses), automation scripting for power users |
| **GLSL → QSB** | Materials, blur, transitions (via Qt Shader Tools) |
| **BitBake / CMake** | Image and build |

### SDK and tooling

- **Qt Creator** plugin: QTOS app templates, manifest editor, deploy-to-device, and a **simulator** (the shell plus simulation backends running in a window on Windows/macOS/Linux).
- **Qt Design Studio** for designers, connected to the token pipeline.
- **Flows:** a visual automation app built on intents (trigger → actions), scriptable in JS or Python.

---

## 9. Apps

### First-party (Qt Quick + QTOS Kit)

| App | Notes / key Qt modules |
|-----|------------------------|
| **Files** | Column, grid and gallery views, tags, Quick Look, network locations via KIO, Rewind integration |
| **Settings** | Search-first, every pane deep-linkable (`qtos://settings/display/night-light`) |
| **Browser** | Qt WebEngine (Chromium): vertical tabs in a Frost sidebar, per-site permissions through the trusted layer |
| **Terminal** | GPU-rendered QML terminal (libvterm core), split panes, Lumen glass |
| **Photos** | Library, editing (shader pipeline), face/object search via `qtos-assistd` if enabled |
| **Media** | Music and video, Qt Multimedia (FFmpeg backend, VA-API hardware decode), MPRIS |
| **Mail / Calendar / Contacts** | JMAP + IMAP/CalDAV/CardDAV, with a Rust core via CXX-Qt |
| **Messages** | Matrix via libQuotient (precedent: NeoChat) |
| **Maps** | Qt Location + MapLibre plugin |
| **Notes · Clock · Weather · Calculator · Camera** | Small apps that show off QTOS Kit; Camera uses Qt Multimedia |
| **Store** | One front end over `.qpk` + Flatpak, with editorial pages |
| **System Monitor** | Qt Graphs (2D/3D), per-app energy impact |
| **Installer** | **Calamares** (Qt/QML) with a Lumen theme |

### Adopted Qt apps (curated, themed by the platform plugin)

Kate / KWrite (text), Okular (documents), Konsole (alt terminal), Krita (painting), Kdenlive (video editing), digiKam (photo pro), OBS Studio (streaming), MuseScore (notation), FreeCAD (CAD), Qt Creator (IDE), LibreOffice with the **Qt 6 VCL backend** (office), KeePassXC (passwords), qBittorrent, Wireshark, VLC, Calibre, Strawberry (music library), KDE Connect (phone continuity).

---

## 10. What makes it next-generation

1. **One shell, every form factor.** The same QML shell adapts to Desktop, Tablet (touch, Qt Virtual Keyboard, larger targets), Handheld (convergent phone), Kiosk/Embedded (Qt Application Manager's automotive heritage), and **Spatial**.
2. **Spatial mode.** Because windows are scene-graph textures, Qt Quick 3D can place them on panels in 3D space. With **Qt Quick 3D Xr** (OpenXR), the same desktop runs in a headset. On flat screens, a 2.5D depth Overview reuses the same scene.
3. **On-device intelligence (opt-in, local-first).** `qtos-assistd` hosts local models for semantic file search, live captions and dictation (Whisper-class models), notification digests, and natural-language commands. The assistant can act **only through declared intents**, shows a preview of what it will do, and asks for confirmation, so it never gets free-form control of the system.
4. **Rewind.** A btrfs snapshot timeline for any folder ("show this folder as it was on Tuesday") plus system-level undo for settings changes.
5. **Continuity.** KDE Connect protocol support: phone notifications, shared clipboard, file drop, SMS, remote input, "find my phone".
6. **Cross-app shared-element transitions** (`qtos_hero`), something no mainstream desktop OS currently offers.
7. **Trusted UI** that users can learn to recognize, plus privacy indicators drawn by the compositor.
8. **Projects:** named, restorable working contexts (apps, window layout, open documents).

---

## 11. Accessibility and internationalization

- **From the first commit:** `Accessible` attached properties on every Kit component, enforced by `qmllint` rules and CI checks. AT-SPI bridge on Linux.
- **Screen reader:** a Qt-native reader (Qt TextToSpeech + AT-SPI over QtDBus) designed around Beam-style navigation. Orca is the interim fallback.
- **Compositor-level aids:** a full-screen and lens magnifier, color filters (deuteranopia/protanopia/tritanopia/grayscale), and pointer and focus highlighting. All are shaders, so they cost almost nothing.
- **Preferences** honored system-wide through the platform theme: Reduce Motion, Reduce Transparency (Glass → Solid), Increase Contrast, larger text with real reflow, sticky/slow/bounce keys, and voice control (built on `qtos-assistd`).
- **i18n:** Qt Linguist workflow, HarfBuzz shaping, full RTL mirroring (`LayoutMirroring`), and input methods via fcitx5 (Qt frontend) or Qt Virtual Keyboard on touch.

---

## 12. Performance budgets (targets on the reference laptop)

| Metric | Target |
|--------|--------|
| Power button → greeter | ≤ 5 s (NVMe) |
| Login → interactive desktop | ≤ 700 ms |
| Compositor frame at 4K @ 120 Hz (shell animating) | GPU ≤ 3 ms, CPU ≤ 2 ms |
| Pointer input → photon | ≤ 1 frame + scanout |
| First-party app: launch → first frame | ≤ 300 ms (quick-launch + AOT-compiled QML) |
| Beam: keystroke → updated results | ≤ 16 ms |
| Shell idle memory | ≤ 250 MB RSS |
| Idle wakeups with nothing animating | ~0 (render on demand) |

Budgets are checked by **frame-time CI on real hardware** using the Qt Quick Profiler, `QSG_RENDER_TIMING`, RenderDoc and Tracy. A change that blows a budget fails the build.

---

## 13. Engineering

### Repository layout (monorepo) *(implemented; see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md))*

```text
QTOS/
├─ kit/              QTOS.Kit: Lumen tokens, Theme, Spring, OKLCH, SDF Surface, Glass, Backdrop, icons, fonts
│  └─ style/         Lumen Qt Quick Controls style
├─ services/         service contracts (interfaces/*.rep) + QTOS.Services (replicas, Intents)
├─ sim/              simulated services, scenarios, qtos-simd
├─ prism/
│  ├─ shell/         QTOS.Shell: Rail, Shelf, Beam, Control Center, windows, Spaces, Overview, Lock
│  ├─ host/          qtos-shell: simulation host
│  └─ compositor/    prism: Qt Wayland Compositor host (Linux)
├─ apps/             first-party apps (QTOS.Apps)
├─ platform/         Lumen QStyle, qtos QPA platform theme, widgets gallery
├─ image/            Yocto: meta-qtos layer, kas config (Phase 2)
├─ sdk/              app template
├─ tools/            qtosctl, token generator, icon importer, screenshots, dev and CI scripts
├─ tests/            Qt Test, Qt Quick Test, Python
└─ docs/             architecture, building, development, design specs, ADRs
```

Planned additions: `prism/backend-kms/` (atomic KMS, Phase 2) and
`prism/protocols/` (`qtos_*` Wayland extensions, Phase 1+).

### Build and toolchain *(decided: [ADR-0005](docs/adr/0005-qt-6-12-baseline.md))*

- CMake presets + **Qt 6.12** (commercial kit); move to the next LTS when validated.
- `qt_add_qml_module` everywhere, so QML is AOT-compiled with `qmlsc` and type-checked with `qmllint` (zero warnings).
- Yocto builds images. `systemd-sysext` handles fast on-device iteration.

### Testing

- **Qt Test / QML TestCase** for units. **Golden-image tests** for every Kit component in light, dark, high-contrast and RTL (`grabToImage` + perceptual diff).
- **Squish** for end-to-end shell and app flows.
- **Fuzzing** (libFuzzer / cargo-fuzz) for protocol handlers, metadata extractors and update parsing.
- Static analysis: clang-tidy, `qmllint`, and Axivion if commercial tooling is available.

### Dev loop from this Windows machine

1. **Simulation mode (native Windows):** shell and apps run directly with Qt for Windows and the simulation backends, with fake windows standing in for clients. This is where most design work happens.
2. **Nested compositor (WSL2 + WSLg):** the real Prism runs as a Wayland client of WSLg with real Qt apps inside it.
3. **Full image in a VM:** QEMU or VMware with 3D acceleration (virtio-gpu / svga via Mesa) to test boot, greeter and updates.
4. **Reference hardware:** one well-supported Linux laptop (for example Framework 13, AMD) plus one touch device. That's where HDR, VRR and power get tested.

---

## 14. Licensing *(decided: [ADR-0004](docs/adr/0004-commercial-qt-and-proprietary-repository.md))*

Several key Qt modules are **GPLv3 or commercial, not LGPL**, including Qt
Wayland Compositor, Qt Application Manager, Qt Virtual Keyboard, Qt Quick 3D
and Qt Graphs. QOR-MATRIX holds a **Qt Enterprise** commercial license, so:

- QTOS is developed and shipped on the **commercial Qt** kit, including
  commercial tooling such as the Qt Quick Compiler extensions (`qmlsc`).
- The repository is **private** and the code **proprietary** (`LICENSE`).
- Third-party assets are permissively licensed (OFL fonts, ISC/MIT icons)
  and listed in `THIRD_PARTY_NOTICES.md`.
- Third-party apps that use only LGPL Qt modules can be proprietary
  regardless, which matters for the app ecosystem.

---

## 15. Precedents to study

| Project | Lesson |
|---------|--------|
| **Sailfish OS** (Lipstick on Qt Wayland Compositor) | A shipped, gesture-first, all-Qt mobile OS. Proof that the core idea works. |
| **Liri** | A desktop shell on Qt Wayland Compositor and the closest precedent to this plan. Study its scope and where it stalled. |
| **Deepin Treeland** (Waylib) | A Qt Quick scene on wlroots. This is option B in practice. |
| **KDE Plasma 6 / KWin** | The reference for HDR, VRR, color management, XWayland and fractional scaling on Qt 6. |
| **Qt Application Manager reference UIs** (e.g. Neptune 3) | Multi-process app lifecycle with a QML System UI. |
| **Lomiri / Ubuntu Touch, AsteroidOS, Plasma Mobile** | Convergence, touch and very-low-memory Qt shells. |

Lesson shared by all of them: **scope is the killer.** QTOS reuses the KDE Frameworks and freedesktop infrastructure and spends its own effort on Prism, Lumen and the first-party experience.

---

## 16. Roadmap (phases with exit criteria)

| Phase | Goal | Exit criteria |
|-------|------|---------------|
| **0 · Foundations** | Lumen tokens, SDF Surface, Glass, Spring, Kit basics; shell in simulation mode | Shell runs on Windows with simulated services; tests and `qmllint` green (**done, v0.1.0**). Golden-image tests move to Phase 1 |
| **1 · Nested Prism** | Real compositor nested in WSLg/Linux: Spaces, Shelf, Rail, Beam, Overview, decorations | Daily-drivable *nested* session for an hour with real Qt apps, 120 fps animations. **Decision gate: compositor backend A vs B.** |
| **2 · Bootable alpha** | Yocto image, KMS backend, greeter, lock, Settings/Files/Terminal/Browser, platform theme | Boots on the VM + reference laptop; flicker-free boot; Widgets apps themed |
| **3 · Platform** | Services complete, portals, sandbox, Flatpak + `.qpk`, Store, A/B updates + rollback, notifications, Control Center | An update and a forced rollback both succeed unattended; 3rd-party Flatpaks run sandboxed |
| **4 · Premium** | HDR, VRR, fractional scaling, gestures everywhere, sound, Quick Look, Stacks, Projects, hero transitions, accessibility pass | All §12 budgets met in CI; screen-reader walkthrough of every shell surface |
| **5 · Ecosystem** | SDK + simulator, intents/Flows, Continuity, optional `qtos-assistd`, Spatial preview | External developer ships an app using only the public SDK |

---

## 17. Top risks

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Bare-metal KMS backend (multi-monitor, VRR, HDR, multi-GPU) takes far longer than planned | Blocks bootable product | Backend-agnostic `prism/core`; decision gate after Phase 1; Waylib fallback |
| Scope explosion (the Liri trap) | Never ships | Reuse KDE Frameworks + freedesktop daemons; first-party apps limited to the ones that define the experience |
| Widgets apps look "second-class" next to QML apps | Inconsistent feel | Strong QStyle + platform theme early (Phase 2), curated adopted-app list |
| GPU driver variance (blur and HDR on weaker iGPUs) | Jank on cheap hardware | Quality tiers chosen automatically from measured frame time; Reduce Transparency path |
| Licensing surprises | Legal and product risk | Decided: commercial Qt Enterprise ([ADR-0004](docs/adr/0004-commercial-qt-and-proprietary-repository.md)); audit third-party additions in `THIRD_PARTY_NOTICES.md` |
| X11 app expectations | User friction | Rootful Xwayland "Legacy" window now; real XWM as an optional later project |

---

## 18. Decisions

| Question | Decision | Record |
|----------|----------|--------|
| Compositor path | Qt Wayland Compositor (option A) with a decision gate after Phase 1; Waylib as fallback | [ADR-0003](docs/adr/0003-compositor-strategy.md) |
| License posture | Commercial Qt Enterprise; private, proprietary repository | [ADR-0004](docs/adr/0004-commercial-qt-and-proprietary-repository.md) |
| Qt baseline | Qt 6.12 | [ADR-0005](docs/adr/0005-qt-6-12-baseline.md) |
| Service IPC | Qt Remote Objects; D-Bus only at freedesktop edges | [ADR-0006](docs/adr/0006-service-ipc-qt-remote-objects.md) |
| Lumen controls | Qt Quick Controls style on Qt Quick Templates; StyleKit evaluated later | [ADR-0007](docs/adr/0007-lumen-style-on-quick-templates.md) |
| Development model | Simulation-first on Windows, then nested Prism, then the image | [ADR-0008](docs/adr/0008-simulation-first-development.md) |
| Design tokens | OKLCH single source, generated for QML, C++ and docs | [ADR-0009](docs/adr/0009-oklch-tokens-single-source.md) |
| Materials and rendering | Shared backdrops, SDF primitives, render on demand | [ADR-0010](docs/adr/0010-materials-and-render-on-demand.md) |
| Primary form factor for v1 | Desktop and laptop; touch and tablet in Phase 4 | [ROADMAP](docs/ROADMAP.md) |
| On-device AI | Phase 5 opt-in, not a launch dependency | [ROADMAP](docs/ROADMAP.md) |
| Naming | QTOS (product), Prism (compositor and shell), Lumen (design language), Beam, Shelf, Rail, Stash | This document |

Still open: **reference hardware** for Phase 2. The current development
machine (RTX 4060 laptop with a 360 Hz panel and an Intel iGPU) is the
performance baseline in [docs/PERFORMANCE.md](docs/PERFORMANCE.md).

---

## 19. First steps *(done in Phase 0)*

1. ✅ Scaffold the monorepo with CMake presets, Qt 6.12 and `qt_add_qml_module` targets.
2. ✅ Lumen **token generator** and the core primitives: **SDF Surface**, **Glass** (shared backdrops), **Spring**.
3. ✅ **Simulated shell** on Windows: lock, Rail, Shelf, Beam, Control Center, notifications, windows, Spaces, Overview, plus six apps and services over Qt Remote Objects.
4. ⏭ Port the shell into **nested Prism** under WSLg and Linux with real Wayland clients (Phase 1; `prism` target exists).
5. ⏭ Write the Phase 1 decision-gate benchmark (multi-output frame pacing, input latency) *before* writing the KMS backend. `qtos-shell --benchmark` is the starting point.
