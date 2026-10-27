# Architecture

This document describes the code as it exists today and how it maps onto the
device architecture in [`VISION.md`](../VISION.md). Decisions and their
rationale are recorded in [`decisions/`](decisions/README.md).

## 1. Layers

```text
┌───────────────────────────────────────────────────────────────────────┐
│ Products       QQ (Studio, Player, runtime) · ARQADE · QOR Media      │  carried from
│                each its own process and CMake project                 │  demiurge-chain
├───────────────────────────────────────────────────────────────────────┤
│ Apps           QTOS.Apps        Files · Music · Notes · Terminal · …  │
│ Shell          QTOS.Shell       Desktop · Rail · Shelf · Beam · …     │
│ Platform API   QTOS.Services    typed replicas + Intents              │
│ Design system  QTOS.Kit, Lumen  tokens · Theme · Spring · materials   │
├───────────────────────────────────────────────────────────────────────┤
│ Hosts          qtos-shell (any desktop OS)  │  prism (Wayland, Linux) │
├───────────────────────────────────────────────────────────────────────┤
│ Services       qtos-sim / qtos-simd (simulation)  │  qtos-*d (device) │
│                ── Qt Remote Objects contracts: services/interfaces ── │
│ QOR core       qor::core (Rust, demiurge-chain at a pinned commit)    │
│                ── C bridge: one JSON request, one JSON answer ──      │
└───────────────────────────────────────────────────────────────────────┘
```

The carried products are separate programs today. Phase 1 runs them as
Wayland clients of Prism and moves their theming and controller input onto
QTOS.Kit and QTOS.Input. The QOR core is linked into a test today. Phase 2
makes it the vault service, a process of its own.

**Simulation and device share all QML.** The shell and the apps never talk to
a service implementation directly. They bind to Qt Remote Objects *replicas*.
In simulation the *sources* are the classes in `sim/`; on a device they are the
`qtos-*d` daemons. The host program decides which sources it connects to.

## 2. Build targets

| Target | Kind | Module URI | Source | Purpose |
|--------|------|------------|--------|---------|
| `qtos_kit` | static lib + QML plugin | `QTOS.Kit` | `kit/` | Tokens, Theme, Motion, Materials, Spring, Oklch, SceneRect, FrameStats, Surface, Glass, Backdrop, RoundMask, Wallpaper, Icon, AppIcon, Artwork, QtosMark, Spectrum, Sounds, building blocks |
| `qtos_lumen_style` | static lib + QML plugin | `Lumen` | `kit/style/` | Qt Quick Controls 2 style (24 controls, Basic fallback) |
| `qtos_input` | static lib + QML plugin | `QTOS.Input` | `input/` | Game controllers: XInput, HID and evdev backends, `Gamepads`, `ControllerNavigator`, `ControllerGlyph` ([CONTROLLERS.md](CONTROLLERS.md)) |
| `qtos_audio` | static lib | — | `audio/` | The audio engine: real-time output, QTOS Sessions synth, file decoding, EQ, limiter, analysis ([AUDIO.md](AUDIO.md)) |
| `qtos_services` | static lib + QML plugin | `QTOS.Services` | `services/` | `Services` singleton (replicas), `Intents`, `Reactive`, QML type info for replicas |
| `qtos_sim` | static lib | — | `sim/` | Simulated sources (audio and media on `qtos_audio`) + scenario loader + QtRO host |
| `qtos_apps` | static lib + QML plugin | `QTOS.Apps` | `apps/` | `AppRegistry` and the six apps |
| `qtos_shell` | static lib + QML plugin | `QTOS.Shell` | `prism/shell/` | `Main`, `Desktop` and every shell surface |
| `qtos-shell` | executable | — | `prism/host/` | Simulation host: window, sim services, CLI, screenshot, benchmark, GPU preference |
| `prism` | executable | `QTOS.Prism` | `prism/compositor/` | Wayland compositor (Linux, needs Qt Wayland Compositor) |
| `qtos-simd` | executable | — | `sim/simd/` | Services in a separate process |
| `qtosctl` | executable | — | `tools/qtosctl/` | CLI client of the services |
| `qtos-gamepads` | executable | — | `tools/gamepads/` | Lists game controllers and streams their state |
| `qtos_lumen_widgets_style` | plugin (`styles`) | — | `platform/style/` | Lumen `QStyle` for Qt Widgets |
| `qtos_platformtheme` | plugin (`platformthemes`) | — | `platform/theme/` | `qtos` QPA platform theme |
| `lumen-widgets-gallery` | executable | — | `platform/gallery/` | QStyle preview |
| `qor_core` (`qor::core`) | static lib (Rust) + header | — | `core/` | The QOR core from demiurge-chain at `QOR_CORE_COMMIT`, built by Corrosion; `qor_core.h` is its C bridge ([§10](#10-the-qor-core)) |
| `tst_*` | tests | — | `tests/` | See [§9](#9-testing) |

The carried products are not targets of this build. Each is its own CMake
project, built by its own script (`tools/dev/products.ps1`), exactly as
upstream ([§11](#11-carried-products)).

All QML modules are static libraries linked into the executables and loaded
with `Q_IMPORT_QML_PLUGIN`. Their QML is compiled ahead of time by the QML
script compiler (`qmlsc`) and lands in a single import directory
(`build/<preset>/qml`) for `qmllint` and `qmlls`.

### Module dependency graph

```text
QTOS.Shell ──► QTOS.Apps ──► QTOS.Services
     │              │              
     └──────────────┴──► QTOS.Kit ◄── Lumen (QQC2 style)
```

`QTOS.Kit` depends on nothing from QTOS. Apps depend on Kit and Services,
never on the Shell. The Shell depends on everything.

## 3. Rendering model

### One scene graph

The desktop is a single Qt Quick scene. Windows (`AppWindow`) are items. In
simulation their content is an app component; under `prism` it is a Wayland
`ShellSurfaceItem`. Because windows are items, Overview, Spaces, minimise and
snapping are just item geometry driven by springs.

### Layering inside `Desktop`

```text
content        Wallpaper · focus-pull overlay · WindowLayer      ← sceneBackdrop source
chrome         SpaceSwitcher · Rail · Shelf · NotificationStack · MediaPopover · ControlCenter · Beam
LockScreen
display sim    Night Light tint · brightness dimmer
```

### Materials: shared backdrops

`Backdrop` renders its `sourceItem` once per change into a texture at
`downsample` (0.25) resolution, blurs it with `MultiEffect`, and exposes the
result as a texture provider. There are two:

| Backdrop | Source | Used by |
|----------|--------|---------|
| `wallpaperBackdrop` (`Materials.desktop`) | wallpaper only | Frost inside windows (`FrostSidebar`, Terminal), lock focus-pull |
| `sceneBackdrop` (`Materials.scene`) | wallpaper + windows | Shell chrome Glass (Rail, Shelf, Beam, Control Center, banners) |

Each `Glass` samples the shared texture at its exact on-screen rectangle.
`SceneRect` re-maps that rectangle on `QQuickWindow::afterAnimating`, once per
rendered frame and never on its own, so any number of glass panels costs one
blur, and only when their source changes. Frost samples the wallpaper-only
backdrop, so a window never samples itself (no recursion).

### Primitives are signed distance fields

`Surface`, `Glass` and `RoundMask` draw continuous-curvature rounded
rectangles (an Lᵖ-norm corner, exponent 4) analytically in one fragment shader
pass, including anti-aliasing, a hairline with specular falloff, an inner
sheen and a soft shadow. There are no textures or nine-patches, and they are
resolution-independent.

### Render on demand

Nothing in the shell requests frames continuously. Springs tick only while
moving (`QAbstractAnimation` driver stops on settle). The clock wakes once per
minute. The Aurora wallpaper is a cached layer (zoom and parallax are item
transforms of that texture). `FrameStats` measures frames passively. When the
desktop is idle, the render loop is idle.

Shaders live in `kit/shaders/*.frag` and are compiled to `.qsb` (SPIR-V, GLSL,
HLSL, MSL) by `qt_add_shaders`, so the same code runs on Direct3D, Vulkan,
Metal and OpenGL through QRhi.

## 4. Motion

`Spring` (C++) is the only animation primitive for motion that should feel
physical. It integrates the closed-form solution of a damped harmonic
oscillator (under-, critically- and over-damped cases), so it is exact at any
frame rate. Retargeting keeps velocity, and gestures can set velocity
directly. `Kit.reducedMotion` makes every spring jump to its target. Presets
(`snappy`, `smooth`, `bouncy`, `gentle`) come from the tokens. See
[design/LUMEN.md](design/LUMEN.md#motion).

## 5. Color and theming

Tokens define every color role in OKLCH for light and dark appearance
(`followAccent` roles take the accent hue as an offset). `Theme` resolves them
through `Oklch.themed()`, interpolating in OKLab by `Theme.darkness`, a spring
between 0 and 1. The accent hue is also a spring that takes the short way
around the hue circle. Every color in the system therefore morphs
continuously.

The shell binds `Theme.dark`, `accentHue`, `reduceMotion`,
`reduceTransparency` and `livingWallpaper` to the settings service, so
`qtosctl theme light` restyles the running system.

The same tokens are generated into `platform/common/lumentokens.h` for the
Widgets `QStyle` and the QPA platform theme, and into `docs/design/tokens.md`.

## 6. Services and IPC

```text
                   services/interfaces/*.rep   (repc)
               ┌───────────────┴────────────────┐
         *SimpleSource (sim/)              *Replica (services/, tools/qtosctl)
               │                                │
   QRemoteObjectHost  ◄── local:qtos-services ──►  QRemoteObjectNode
   (qtos-shell embedded, or qtos-simd)          (Services singleton, qtosctl, tests)
```

- Endpoint: `local:qtos-services` (Windows and macOS named pipe or socket),
  `localabstract:qtos-services` (Linux abstract socket). If the default is
  taken, the host falls back to `…-<pid>`.
- `Services::connectTo()` acquires all replicas and waits until each has its
  initial state before QML loads, so the UI never flashes default values.
- Replicas are registered for QML with `QML_FOREIGN` (`NetworkService`,
  `PowerService` and so on). `cmake/QtosRepc.cmake` generates them so AUTOMOC
  emits metatypes, which gives `qmllint` and `qmlsc` full type information.
- Apps request system actions through `Intents` (`app.open`, `session.lock`,
  `shell.overview`, `shell.beam`, `shell.space`), which only the shell
  fulfils.

Contracts are documented in [SERVICES.md](SERVICES.md).

## 7. Window management

`WindowLayer` owns the windows (`AppWindow` instances on a horizontal strip,
one screen-width per Space).

- **Open**: the window flies out of its Shelf icon (`origin`), driven by a
  `presence` spring. Close shrinks in place. Minimise flies back to the Shelf.
- **Move and resize**: `DragHandler`s write geometry and `snap()` the springs
  so tracking is 1:1. On release, `snapTargetFor()` applies magnetic halves or
  maximise.
- **Overview**: `relayout()` computes a grid and sets `overviewRect`/`overviewScale`.
  Springs animate there. Window layers enable mipmapping while scaled.
- **Spaces**: `spaceOffset` (a spring) slides the strip, and the wallpaper
  parallax follows.
- **Focus**: z-order counter, focused windows get elevation 5, others 3.

Under `prism`, `WindowLayer.surfaceDelegate` is a `ShellSurfaceItem` component
and `adopt()` wraps each xdg-toplevel in the same `AppWindow`.

## 8. Hosts

### `qtos-shell` (simulation)

1. Applies the GPU preference: high-performance by default via DXGI on Windows ([PERFORMANCE.md](PERFORMANCE.md#gpu-selection)).
2. `Kit::initialize()` registers Inter and JetBrains Mono and sets the application font.
3. Selects the `Lumen` Quick Controls style.
4. Starts `SimServices` in-process (or connects to an external host) and calls `Services::connectTo()`.
5. Loads `QTOS.Shell/Main` with an `options` map from the command line.
6. Optionally runs the screenshot timer or the `--benchmark` tour.

### `prism` (device, experimental)

`Compositor.qml` creates a `WaylandCompositor` with one `WaylandOutput` whose
window hosts the same `Desktop`. It plugs a `ShellSurfaceItem` delegate into
the window layer and implements xdg-shell, xdg-decoration (server-side),
text-input and Qt's text-input-method. It drives one output per display
(`--outputs N` nested; every screen under eglfs). It runs nested today. Prism's
own KMS backend is Phase 3
([QOR-0005](decisions/QOR-0005-every-display-at-its-own-refresh.md),
[QTOS-0003](decisions/qtos/QTOS-0003-compositor-strategy.md)).

## 9. Testing

| Suite | Framework | Covers |
|-------|-----------|--------|
| `tst_spring` | Qt Test | spring physics: damping regimes, settling, velocity preservation, presets, reduced motion, stability |
| `tst_oklch` | Qt Test | OKLCH/OKLab conversions, gamut mapping, themed tokens, perceptual mixing |
| `tst_services` | Qt Test | end-to-end QtRO: state replication, replica→source writes, remote slots and signals, Wi-Fi and media behaviour |
| `tst_audio` | Qt Test | the audio engine: rendering, sessions, decoding, EQ, limiter, analysis |
| `tst_input` | Qt Test | controller reports, CRCs, mappings, dead zones, the manager and the navigator |
| `tst_store` | Qt Test | catalogue and package signatures, installation, tampering refused |
| `tst_qorcore` | Qt Test | the QOR core through its C bridge: version, units, exact amounts, refusals ([§10](#10-the-qor-core)) |
| `store_widgets_qmllint` | qmllint | the Store's widgets against the built modules |
| `tst_shell` | Qt Quick Test | Beam fuzzy ranking, calculator (including injection rejection), section grouping |
| `tst_kit_qml` | Qt Quick Test (GPU) | tokens, theme roles, typography, springs in QML, Oklch singleton |
| `tst_golden` | Qt Test (GPU) | every Kit component in every variant against reference images |
| `shell_e2e` | scripted shell (GPU) | whole flows with synthetic input, controllers included |
| `prism_e2e` | Python + real clients (GPU, Linux) | the compositor with real Wayland clients: windows, Spaces, displays |
| `tools_generate_tokens` | Python unittest | color math, generated outputs current |

`all_qmllint` must report zero warnings. CI excludes the `gpu` label on hosted
runners. The carried products' suites are listed in [§11](#11-carried-products).

## 10. The QOR core

The QOR core is the Rust library that holds the keys and talks to the QOR
network. Its source is `tools/qor-launcher/core/` in demiurge-chain, and
QOR OS never copies it
([QOR-0003](decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)).

- `core/CMakeLists.txt` fetches demiurge-chain at `QOR_CORE_COMMIT` (or uses
  `QOR_CORE_SOURCE_DIR`), and Corrosion builds the `qor-launcher-core` crate as
  a static library from its `Cargo.lock`.
- The bridge is three C functions:
  - `qor_core_version()`;
  - `qor_core_call(request)`, which takes `{"command": …, "args": {…}}` and
    returns `{"ok": …}` or `{"error": {"kind", "message"}}`;
  - `qor_core_free(answer)`.

  Amounts cross as decimal strings of embers, and no command answers with a key.
- At the current pin the bridge answers `core_version`, `token_info`,
  `parse_amount` and `format_sparks`. It grows upstream as Phase 2 needs it.
- `QOR_BUILD_CORE` is on where `cargo` is found, and the CI presets require it.

## 11. Carried products

QQ, ARQADE and QOR Media come from demiurge-chain with their history, at the
same paths as upstream ([UPSTREAM.md](UPSTREAM.md)). Each is its own CMake
project, built and tested by its own `build.ps1`, which
`tools/dev/products.ps1` calls.

| Product | Path | Targets | Suites |
|---------|------|---------|--------|
| **QQ** | `products/qq/` | `qqruntime` (QML module `QQ`), `qq-studio` (`QQ.Studio`), `qq-player` (`QQ.Player`), `qq-mcp`, `qqagent` | `tst_world`, `tst_scene`, `tst_studio`, `tst_play`, `tst_player`, `tst_agent`, `tst_lockdown`, `tst_publish` |
| **ARQADE** | `products/arqade/qt/` (+ `games/`, `tests/fixtures/`) | `arqade` (QML module `ARQADE`), built on QQ's runtime from `../../qq/runtime` | `tst_arqadelink`, `tst_arqadeserver`, `tst_interface`, `tst_games` |
| **QOR Media** | `tools/qor-media/` | `qor-media` (QML module `QOR.Media`), with PDFium | `tst_medialink`, `tst_files`, `tst_textfile`, `tst_playback`, `tst_look`, `tst_reader`, `tst_pdfpages`, `tst_pdfmarks`, `tst_pdfforms` |

Their suites render on the GPU in real windows and drive them with input. The
look tests read `docs/design/tokens.json`, Demiurge's one look. Today they
build on Windows. The Linux build, running under Prism, Lumen theming and
QTOS.Input are Phase 1.

## 12. Repository conventions

- Generated files are committed and checked for staleness: `kit/Tokens.qml`,
  `platform/common/lumentokens.h` and `docs/design/tokens.md` (from
  `tools/tokens/generate_tokens.py`).
- Every source file carries `SPDX-License-Identifier: LicenseRef-QTOS-Proprietary`,
  except the carried code, which keeps its Apache-2.0 notices and its own
  formatting.
- Naming: C++ in namespace `qtos` (`qtos::sim`, `qtos::lumen`, `qtos::host`),
  QML modules under `QTOS.*` except the style module `Lumen`. The code keeps
  these names until the product name is cleared
  ([QOR-0008](decisions/QOR-0008-before-a-public-release.md)).
