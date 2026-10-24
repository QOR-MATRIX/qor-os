# Building and running QTOS

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

Recommended for upcoming phases (see [QT-MODULES.md](QT-MODULES.md)): Qt
Multimedia, Qt Quick 3D, Qt Quick Timeline, Qt Lottie, Qt Virtual Keyboard,
Qt WebEngine, Qt Spatial Audio, Qt Positioning, Qt TextToSpeech.

`cmake/QtosLocateQt.cmake` finds Qt automatically, in this order:
`Qt6_DIR`/`CMAKE_PREFIX_PATH` from your IDE kit, the `QTOS_QT_DIR`
environment variable, then `C:\Qt\6.12.0\msvc2022_64`.

### Linux

Qt 6.12.0 `gcc_64` with the same modules (plus **Qt Wayland Compositor** to
build `prism`), GCC 13+ or Clang 17+, CMake 3.27+, Ninja, and the
OpenGL/EGL/xkbcommon development packages:

```bash
sudo apt-get install ninja-build libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev \
  libfontconfig1-dev libfreetype-dev libwayland-dev
```

## 2. Configure and build

The Ninja presets need the MSVC environment. `tools\dev\Enter-QtosShell.ps1`
loads it (via `vswhere` and `vcvars64.bat`) together with Qt's `bin`, CMake and
Ninja:

```powershell
. .\tools\dev\Enter-QtosShell.ps1          # dot-source once per terminal
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
| `ci-windows`, `ci-linux` | Ninja | RelWithDebInfo | CI. `ci-windows` treats warnings as errors |

Build options (`-D<option>=ON|OFF`):

| Option | Default | Effect |
|--------|---------|--------|
| `QTOS_BUILD_TESTS` | ON | Tests in `tests/` |
| `QTOS_BUILD_TOOLS` | ON | `qtosctl`, `qtos-simd` |
| `QTOS_BUILD_PLATFORM` | ON | QStyle plugin, platform theme, widgets gallery |
| `QTOS_BUILD_COMPOSITOR` | ON on Linux | `prism` (skipped with a warning if Qt Wayland Compositor is missing) |
| `QTOS_WARNINGS_AS_ERRORS` | OFF | `/WX` or `-Werror` |
| `QTOS_QT_PINNED_VERSION` | 6.12.0 | Version used by Qt auto-location |

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
.\tools\dev\run.ps1 [-Preset windows-release] -- <qtos-shell options>
```

`qtos-shell` options:

| Option | Description |
|--------|-------------|
| `--unlocked` | Start on the desktop instead of the lock screen |
| `--open <apps>` | Comma-separated apps to open after unlock: `files`, `music`, `notes`, `terminal`, `monitor`, `settings` |
| `--surface <name>` | Show `beam`, `control-center`, `overview` or `media` after start |
| `--query <text>` | Initial Beam query (with `--surface beam`) |
| `--theme light\|dark` | Force appearance (writes to the settings service) |
| `--accent <hue>` | Force accent hue in OKLCH degrees |
| `--size WxH` | Window size in logical pixels (clamped to the screen) |
| `--fullscreen` | Start full screen (<kbd>F11</kbd> toggles) |
| `--services embedded\|external` | Host the simulator in-process (default) or connect to `qtos-simd` |
| `--services-url <url>` | Services endpoint |
| `--scenario <file>` | Simulation scenario JSON (default: built-in `sim/scenarios/default.json`) |
| `--ephemeral` | Do not persist preferences (implied by `--screenshot` and `--benchmark`) |
| `--gpu high-performance\|low-power\|default` | GPU preference (default `high-performance`) |
| `--screenshot <file> [--delay ms]` | Save a frame (`.webp`, `.png`, `.jpg`) and exit |
| `--benchmark <seconds> [--benchmark-out file]` | Scripted animation tour, prints frame-pacing JSON, exits |

Persistent preferences (appearance, accent, motion, transparency, living
wallpaper) are stored with `QSettings` under `QOR-MATRIX/QTOS Shell` and Notes
under the `notes` category.

### Separate service process

```powershell
.\build\windows-release\qtos-simd.exe --ephemeral            # terminal 1
.\tools\dev\run.ps1 -- --services external --unlocked       # terminal 2
.\build\windows-release\qtosctl.exe watch                    # terminal 3
```

## 4. Tests and lint

```powershell
ctest --preset windows-release                       # all suites
ctest --preset windows-release -LE gpu               # skip GPU-dependent suites
cmake --build --preset windows-release --target all_qmllint
python tools\tokens\generate_tokens.py --check
python tools\ci\check_headers.py
pre-commit run --all-files
```

## 5. Install and deploy

```powershell
cmake --install build\windows-release --prefix dist\qtos
```

The install step runs Qt's deployment script (`qt_generate_deploy_qml_app_script`,
which calls `windeployqt` on Windows) to produce a self-contained folder with
the required Qt libraries, plugins and QML modules.

## 6. Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Cannot open include file: 'rep_…_replica.h'` | Reconfigure; generated headers live in the build tree (`services/`, `tools/qtosctl/`) |
| Configure can't find Qt | Set `QTOS_QT_DIR` to the kit (`C:\Qt\6.12.0\msvc2022_64`) or pass `-DCMAKE_PREFIX_PATH=…` |
| `cl` not found with Ninja presets | Dot-source `tools\dev\Enter-QtosShell.ps1`, or use the `windows-vs` preset |
| No console output from `qtos-shell` | It is a GUI-subsystem app. Set `QT_FORCE_STDERR_LOGGING=1` and redirect stderr. Use `QT_LOGGING_RULES="qtos.*=true"` for QTOS categories |
| Test output missing in `ctest` | Tests set `QT_FORCE_STDERR_LOGGING=1`; run the binary with `-o -,txt` to see QtTest output directly |
| Low frame rate on a laptop | Check the log line `GPU: …`. Use `--gpu high-performance`, or force an adapter with `QT_D3D_ADAPTER_INDEX` (`QSG_INFO=1` lists adapters) |
| Shader or graphics issues | `QSG_INFO=1` prints the RHI backend and adapter; `QSG_RHI_BACKEND=vulkan\|opengl\|d3d12` switches backends |
| Services did not initialise in time | Another QTOS instance holds `local:qtos-services`. The host falls back to a PID-suffixed endpoint; pass `--services-url` to `qtosctl` |
