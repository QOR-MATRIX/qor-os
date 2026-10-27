# Building and running QOR OS

QOR OS has two kinds of build:

- **The OS**: the CMake presets below. They build the Kit, services, shell,
  apps, compositor, tools, and the QOR core from demiurge-chain at its pinned
  commit.
- **The carried products**: QQ, ARQADE and QOR Media, each built by its own
  script, exactly as upstream ([§6](#6-the-carried-products-qq-arqade-qor-media)).

## 1. Prerequisites

### Windows (primary development platform)

| Tool | Version | Notes |
|------|---------|-------|
| Qt | **6.12.0**, kit `msvc2022_64` | Commercial install via Qt Maintenance Tool (see module list below) |
| Visual Studio 2022 or Build Tools | 17.14+ | Workload "Desktop development with C++" (MSVC v143, Windows SDK) |
| CMake | 3.27+ | Bundled with Qt: `C:\Qt\Tools\CMake_64` |
| Ninja | any recent | Bundled with Qt: `C:\Qt\Tools\Ninja` |
| Python | 3.10+ | Token generator, tests, tooling |
| Git | 2.40+ | |
| Rust | 1.91+ (1.98.1 in CI), from [rustup](https://rustup.rs) | The QOR core (`core/`), built by `cargo` through Corrosion |

### Required Qt modules

Install these for the `MSVC 2022 64-bit` kit in the Qt Maintenance Tool
(`C:\Qt\MaintenanceTool.exe` → Add or remove components → Qt 6.12.0):

| Component | Installer package | Needed for |
|-----------|-------------------|------------|
| Qt 6.12.0 MSVC 2022 64-bit | `qt.qt6.6120.win64_msvc2022_64` | Core, Gui, Qml, Quick, Quick Controls, Quick Effects, Quick Shapes, Svg, Test, Widgets |
| Qt Shader Tools | `qt.qt6.6120.addons.qtshadertools` | `qt_add_shaders` (`.frag` → `.qsb`) |
| Qt Remote Objects | `qt.qt6.6120.addons.qtremoteobjects` | Service IPC, `repc` |
| Qt Graphs | `qt.qt6.6120.addons.qtgraphs` | Monitor app |
| Qt Image Formats | `qt.qt6.6120.addons.qtimageformats` | WebP screenshots |
| Qt Quick Compiler extensions | `qt.qt6.6120.addons.qtqmlscriptcompiler` | `qmlsc` ahead-of-time QML compilation (commercial) |
| Qt Task Tree (TP) | `qt.qt6.6120.addons.qttasktree` | Dependency of Qt's own QML asset-downloader plugin; without it CMake prints a warning at configure time |

The carried products also need these, from the same kit:

- **Qt Multimedia**;
- **Qt Quick 3D**, with **Qt Quick 3D Physics**;
- **Qt Spatial Audio**;
- **Qt PDF**;
- **Qt Network**.

The QQ Player's browser build needs Qt for WebAssembly (multithreaded).
Recommended for upcoming phases (see [QT-MODULES.md](QT-MODULES.md)): Qt Quick
Timeline, Qt Lottie, Qt Virtual Keyboard, Qt WebEngine, Qt Positioning, Qt
TextToSpeech.

`cmake/QorLocateQt.cmake` finds Qt automatically, in this order:
`Qt6_DIR`/`CMAKE_PREFIX_PATH` from your IDE kit, the `QOR_QT_DIR`
environment variable, then `C:\Qt\6.12.0\msvc2022_64`.

### Linux

Qt 6.12.0 `gcc_64` with the same modules (plus **Qt Wayland Compositor** to
build `prism`), GCC 13+ or Clang 17+, CMake 3.27+, Ninja, and the
OpenGL/EGL/xkbcommon development packages:

```bash
sudo apt-get install ninja-build libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev \
  libfontconfig1-dev libfreetype-dev libwayland-dev libpulse0 libssl-dev libdbus-1-dev pkg-config
```

`libpulse0` is needed by Qt Multimedia (the audio engine); desktops running
PipeWire are used through it automatically. `libssl-dev` (OpenSSL 3's
libcrypto) verifies Store signatures; Windows uses CNG instead.
`libdbus-1-dev` is for the QOR core, whose keychain uses the Secret Service.
Rust comes from rustup.

### Linux on WSL2 (Windows workstation)

WSL2 with WSLg is the quickest way to work on the Linux-only parts (`prism`,
the platform plugins) from a Windows machine. It is verified on Ubuntu 26.04
with GCC 15 and CMake 4.2.

```bash
# 1. Build dependencies, plus Weston's demo clients for testing Prism:
sudo apt-get install build-essential cmake ninja-build git python3 pkg-config \
  libgl1-mesa-dev libegl1-mesa-dev libgles-dev libdrm-dev libgbm-dev libvulkan-dev \
  libxkbcommon-dev libxkbcommon-x11-dev libfontconfig1-dev libfreetype-dev libdbus-1-3 \
  libwayland-dev wayland-protocols libinput-dev libxcb-cursor0 libpulse0 libssl-dev weston \
  libdbus-1-dev
#    and Rust, for the QOR core:
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.98.1

# 2. The commercial Qt kit, unattended. This reuses the Qt Account sign-in
#    from Windows. Copy it once and keep it private:
install -D -m 600 "/mnt/c/Users/<you>/AppData/Roaming/Qt/qtaccount.ini" ~/.local/share/Qt/qtaccount.ini
python3 /mnt/x/path/to/qor-os/tools/ci/install_qt.py --platform linux --root ~/Qt

# 3. Build the Windows checkout from WSL into the Linux filesystem (fast I/O).
#    Turn .qmlls.ini generation off so the Windows editor setup is untouched:
export PATH=$HOME/.cargo/bin:$PATH
cmake -S /mnt/x/path/to/qor-os -B ~/qor-build/linux-release --preset linux-release \
  -DQT_QML_GENERATE_QMLLS_INI=OFF
cmake --build ~/qor-build/linux-release
ctest --test-dir ~/qor-build/linux-release        # GPU suites run on WSLg too
```

On Linux the executables sit in their target directories, for example
`prism/qor-shell`, `prism/compositor/prism` and `platform/lumen-widgets-gallery`.

## 2. Configure and build

The Ninja presets need the MSVC environment. `tools\dev\Enter-QorShell.ps1`
loads it (via `vswhere` and `vcvars64.bat`) together with Qt's `bin`, CMake and
Ninja:

```powershell
. .\tools\dev\Enter-QorShell.ps1          # dot-source once per terminal
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

Or in one step: `.\tools\dev\build.ps1 [-Preset windows-debug] [-Test] [-Target all_qmllint] [-Fresh]`.

| Preset | Generator | Build type | Use |
|--------|-----------|------------|-----|
| `windows-release` | Ninja | RelWithDebInfo | Day-to-day work, benchmarks, screenshots |
| `windows-debug` | Ninja | Debug | Debugging (Qt debug DLLs) |
| `windows-vs` | Visual Studio 17 2022 | multi-config | Works without a developer shell |
| `linux-release`, `linux-debug` | Ninja | | Linux |
| `ci-windows`, `ci-linux` | Ninja | RelWithDebInfo | CI. Both treat warnings as errors and require the QOR core |

Build options (`-D<option>=ON|OFF`):

| Option | Default | Effect |
|--------|---------|--------|
| `QOR_BUILD_TESTS` | ON | Tests in `tests/` |
| `QOR_BUILD_TOOLS` | ON | `qorctl`, `qor-simd` |
| `QOR_BUILD_PLATFORM` | ON | QStyle plugin, platform theme, widgets gallery |
| `QOR_BUILD_COMPOSITOR` | ON on Linux | `prism` (skipped with a warning if Qt Wayland Compositor is missing) |
| `QOR_WARNINGS_AS_ERRORS` | OFF | `/WX` or `-Werror` |
| `QOR_QT_PINNED_VERSION` | 6.12.0 | Version used by Qt auto-location |
| `QOR_BUILD_CORE` | ON where `cargo` is found | The QOR core and `tst_qorcore` ([UPSTREAM.md](UPSTREAM.md)) |
| `QOR_CORE_SOURCE_DIR` | empty | A local demiurge-chain checkout to build the core from instead: to change the core and QOR OS together, or to try a commit before the pin moves to it |

The pin (`QOR_CORE_COMMIT`) and the core's Cargo features
(`QOR_CORE_FEATURES`, `desktop`) are set in `core/CMakeLists.txt` and written
on every configure, so they are not options. A build folder configured at an
older pin follows a new one and says so ("the pin moved to …").

The first configure with the core clones demiurge-chain at the pin and
Corrosion v0.6.1 into the build tree, and the first build compiles the core's
Rust dependencies. Both need network access and take a few minutes; with
`desktop`, the first build of the core takes about 3–4 minutes on the
workstation. Later builds are incremental.

Outputs (Windows): executables and test binaries in `build/<preset>/`, QML
modules in `build/<preset>/qml/`, plugins in `build/<preset>/plugins/`.

### IDEs

- **Qt Creator**: open `CMakeLists.txt`, choose the *Qt 6.12.0 MSVC2022 64bit* kit. Presets are detected.
- **VS Code**: open the folder and accept the recommended extensions (Qt
  extension pack, CMake Tools, clangd). Presets, launch configurations and
  tasks are in `.vscode/`.
- **Visual Studio**: use the `windows-vs` preset, or open the folder (CMake presets are supported).

## 3. Run

```powershell
.\tools\dev\run.ps1 [-Preset windows-release] -- <qor-shell options>
```

`qor-shell` options:

| Option | Description |
|--------|-------------|
| `--unlocked` | Start on the desktop instead of the lock screen |
| `--open <apps>` | Comma-separated apps to open after unlock: `files`, `music`, `notes`, `terminal`, `monitor`, `settings`. `app/page` opens a page and `app/page/section` scrolls to a section, for example `settings/sound/reactive` |
| `--play`, `--seek <seconds>` | Start the music, at a position |
| `--surface <name>` | Show `beam`, `control-center`, `overview` or `media` after start |
| `--query <text>` | Initial Beam query (with `--surface beam`) |
| `--theme light\|dark` | Force appearance (writes to the settings service) |
| `--accent <hue>` | Force accent hue in OKLCH degrees |
| `--contrast standard\|high` | Force Increase Contrast off or on |
| `--size WxH` | Window size in logical pixels (clamped to the screen) |
| `--fullscreen` | Start full screen (<kbd>F11</kbd> toggles) |
| `--services embedded\|external` | Host the simulator in-process (default) or connect to `qor-simd` |
| `--services-url <url>` | Services endpoint |
| `--scenario <file>` | Simulation scenario JSON (default: built-in `sim/scenarios/default.json`) |
| `--ephemeral` | Do not persist preferences (implied by `--screenshot` and `--benchmark`) |
| `--gpu high-performance\|low-power\|default` | GPU preference (default `high-performance`) |
| `--screenshot <file> [--delay ms]` | Save a frame (`.webp`, `.png`, `.jpg`) and exit |
| `--benchmark <seconds> [--benchmark-out file]` | Scripted animation tour, prints frame-pacing JSON, exits |
| `--script <file>` | Drive the shell with a JSON script and exit ([DEVELOPMENT.md](DEVELOPMENT.md)) |

`--screenshot`, `--benchmark` and `--script` render audio silently (the
engine's null output; see [AUDIO.md](AUDIO.md)), see only virtual game
controllers (`QOR_GAMEPADS=virtual`; see [CONTROLLERS.md](CONTROLLERS.md)),
leave the touchpad alone (`QOR_TOUCHPAD=off`; see
[SHELL.md](design/SHELL.md#gestures)), and install from the Store into a
temporary folder (`QOR_STORE_DIR`; see [STORE.md](STORE.md)).
`QOR_STORE_URL` points the Store at another signed catalogue.

Persistent preferences (appearance, accent, motion, transparency, living
wallpaper, audio and the audio-reactive interface) are stored with `QSettings`
under `QOR-MATRIX/QTOS Shell` and Notes under the `notes` category.

### Separate service process

```powershell
.\build\windows-release\qor-simd.exe --ephemeral            # terminal 1
.\tools\dev\run.ps1 -- --services external --unlocked       # terminal 2
.\build\windows-release\qorctl.exe watch                    # terminal 3
```

### Prism, the Wayland compositor (Linux)

`prism` hosts the same shell as a real Wayland compositor. It runs nested
inside any Wayland or X11 session, including WSLg:

```bash
B=~/qor-build/linux-release
$B/prism/compositor/prism --unlocked --socket wayland-qor &
WAYLAND_DISPLAY=wayland-qor weston-terminal &                  # client-side decorations
WAYLAND_DISPLAY=wayland-qor QT_QPA_PLATFORM=wayland \
  $B/platform/lumen-widgets-gallery &                           # Qt app, Prism decorations
```

![Prism nested in WSLg with weston-terminal and a Qt Widgets client](images/prism-nested.webp)

| Option | Description |
|--------|-------------|
| `--unlocked` | Start on the desktop instead of the lock screen |
| `--socket <name>` | Wayland socket for clients (default: the first free `wayland-N`, printed at start) |
| `--size WxH` | Output window size |
| `--outputs N` | Drive N outputs (up to 4), side by side, each a window of `--size`. On bare metal (`QT_QPA_PLATFORM=eglfs`) every connected screen is an output instead |
| `--spread-clients` | Open each new client on the output with the fewest windows |
| `--services embedded\|external` | As for `qor-shell` |
| `--screenshot <file> [--delay ms]` | Save the output and exit (used for automated checks) |
| `--benchmark <seconds> [--benchmark-out file]` | Shell tour plus input latency; per-output JSON report, then exit ([PERFORMANCE.md](PERFORMANCE.md)) |

How clients are handled:

- **Decorations.** Clients that support `xdg-decoration` (all Qt apps) get
  Prism's title bar and controls. Clients that draw their own (Weston demos,
  GTK) get none, so nothing is drawn twice.
- **Window geometry.** A client's window geometry is aligned with the frame,
  and client-drawn shadows are clipped, never scaled.
- **Size.** If a client picks its own size (a preferred size on first map, or
  whole character cells in a terminal), the frame adopts it.
- **Window state.** Focus, maximise and minimise follow `xdg_toplevel`
  requests and states.
- **Moves.** A move started from a client's own title bar uses the same
  springs and edge snapping as Prism's.

Under WSLg, Mesa's `libEGL` warnings and "no EGL_WL_bind_wayland_display" are
expected. Clients fall back to shared-memory buffers, which is fine for
development; on device, GPU buffer sharing is part of Phase 3.

## 4. Tests and lint

```powershell
ctest --preset windows-release                       # all suites
ctest --preset windows-release -LE gpu               # skip GPU-dependent suites
cmake --build --preset windows-release --target all_qmllint
python tools\tokens\generate_tokens.py --check
python tools\ci\check_headers.py
.\.venv\Scripts\pre-commit run --all-files
```

## 5. Install and deploy

```powershell
cmake --install build\windows-release --prefix dist\qor-os
```

The install step runs Qt's deployment script (`qt_generate_deploy_qml_app_script`,
which calls `windeployqt` on Windows) to produce a self-contained folder with
the required Qt libraries, plugins and QML modules. The result runs on a
machine without Qt installed (verified with an empty `PATH`).

Because the QOR OS QML modules are static and embedded in the executable,
`qor-shell` sets `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` to `kit/`, `apps/`
and `services/` so the deployment scan finds their imports (for example
`QtCore` and `QtGraphs`). Add new module directories there.

## 6. The carried products (QQ, ARQADE, QOR Media)

QQ, ARQADE and QOR Media come from demiurge-chain at the same paths as
upstream ([UPSTREAM.md](UPSTREAM.md)). Each keeps its own CMake project and
`build.ps1`, which configures with MSVC 2022 and Ninja, builds, and runs every
suite. `tools/dev/products.ps1` runs them all:

```powershell
.\tools\dev\products.ps1                          # build\products\{qq,arqade,media}
.\tools\dev\products.ps1 -Product qq -Attempts 1  # one product, no retry
```

| Product | Script | Programs | Notes |
|---------|--------|----------|-------|
| QQ | `products/qq/build.ps1` | `qq-studio.exe`, `qq-player.exe`, `qq-mcp.exe` | `-Run` opens QQ Studio; `-Web` builds the Player for the browser (Qt for WebAssembly and Emscripten) |
| ARQADE | `products/arqade/qt/build.ps1` | `arqade.exe` | Builds QQ's runtime from `products/qq/runtime`; carries the games from `products/arqade/games` |
| QOR Media | `tools/qor-media/build.ps1` | `qor-media.exe` | Fetches the pinned PDFium release once (checked by SHA-256) into `%LOCALAPPDATA%\qor-media-pdfium` |

Their suites open real windows on the GPU and drive them with the keyboard
and mouse. Run them in a desktop session, and leave the desktop alone while
they run: a window that loses the keyboard to another application fails a
test. `products.ps1` tries a failing product once more. The look tests read
`docs/design/tokens.json`.

The samples are QQ scenes you can open in QQ Studio, for example
`products\qq\samples\orb-run`. Today the products build on Windows. Linux, and
running them inside Prism, are Phase 1.

## 7. Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Cannot open include file: 'rep_…_replica.h'` | Reconfigure; generated headers live in the build tree (`services/`, `tools/qorctl/`) |
| Configure can't find Qt | Set `QOR_QT_DIR` to the kit (`C:\Qt\6.12.0\msvc2022_64`) or pass `-DCMAKE_PREFIX_PATH=…` |
| `cl` not found with Ninja presets | Dot-source `tools\dev\Enter-QorShell.ps1`, or use the `windows-vs` preset |
| No console output from `qor-shell` | It is a GUI-subsystem app. Set `QT_FORCE_STDERR_LOGGING=1` and redirect stderr. Use `QT_LOGGING_RULES="qor.*=true"` for QOR OS categories |
| Test output missing in `ctest` | Tests set `QT_FORCE_STDERR_LOGGING=1`; run the binary with `-o -,txt` to see QtTest output directly |
| Low frame rate on a laptop | Check the log line `GPU: …`. Use `--gpu high-performance`, or force an adapter with `QT_D3D_ADAPTER_INDEX` (`QSG_INFO=1` lists adapters) |
| Shader or graphics issues | `QSG_INFO=1` prints the RHI backend and adapter; `QSG_RHI_BACKEND=vulkan\|opengl\|d3d12` switches backends |
| Services did not initialise in time | Another QOR OS instance holds `local:qor-services`. The host falls back to a PID-suffixed endpoint; pass `--services-url` to `qorctl` |
| `QOR core: No QOR core at …` or Corrosion cannot find `cargo` | Install Rust from rustup (1.91+), open a new terminal, and reconfigure; or turn the core off with `-DQOR_BUILD_CORE=OFF` |
| The core's configure cannot fetch demiurge-chain | It needs network access to GitHub once per build tree; or point `QOR_CORE_SOURCE_DIR` at a local checkout |
| A carried product's suite fails on a busy desktop | Leave the desktop alone while the suites run, or run `products.ps1` again; a failure that repeats is real |
