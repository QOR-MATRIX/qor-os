# prism — shell, simulation host and compositor

| Path | Contents |
|------|----------|
| `shell/` | **QTOS.Shell**: `Main` (simulator window), `Desktop` (orchestration, layering, shortcuts, actions), `WindowLayer` + `AppWindow` (window management), `Rail`, `LiveActivity`, `MediaPopover`, `Shelf` + `ShelfButton`, `Beam` + `search.js`, `ControlCenter`, `NotificationStack` + `NotificationCard`, `LockScreen`, `SpaceSwitcher` |
| `host/` | **qtos-shell**: CLI options, embedded simulator, screenshot, `--benchmark`, GPU preference (`gpupreference.cpp`) |
| `compositor/` | **prism** (Linux, experimental): `Compositor.qml` hosts the same `Desktop` in a Qt Wayland Compositor scene; xdg-shell, server- and client-side decorations, text input |

Behaviour spec: [docs/design/SHELL.md](../docs/design/SHELL.md). Compositor
strategy: [ADR-0003](../docs/adr/0003-compositor-strategy.md).

Run the compositor nested on Linux:

```bash
cmake --preset linux-release && cmake --build --preset linux-release
./build/linux-release/prism/compositor/prism --unlocked --socket wayland-qtos &
WAYLAND_DISPLAY=wayland-qtos QT_QPA_PLATFORM=wayland some-qt-app
```

Without `--socket`, Prism takes the first free socket in `$XDG_RUNTIME_DIR`
(`wayland-1` inside most sessions) and prints it at start.

## End-to-end tests

`tools/dev/prism-e2e.py --build-dir <build>` runs a nested Prism with real
clients and drives it through `prism --script`. The ctest name is
`prism_e2e`, and it needs a display. It checks 24 window-management
behaviours, from decorations to popups and Shelf identity. See
[DEVELOPMENT.md](../docs/DEVELOPMENT.md#shell-scripts-and-end-to-end-tests).

## Wayland protocols

| Protocol | Provided by | Notes |
|----------|-------------|-------|
| `xdg_wm_base` (xdg-shell), `xdg_wm_dialog_v1` | Qt (`XdgShell`) | Toplevels and popups |
| `zxdg_decoration_manager_v1` | Qt (`XdgDecorationManagerV1`) | Server-side decorations preferred |
| `wp_viewporter` | Qt (always on) | Used with fractional scale |
| `wp_fractional_scale_manager_v1` | Prism (`extensions.cpp`) | Preferred scale = the output window's device pixel ratio |
| `wp_cursor_shape_manager_v1` (v2) | Prism (`extensions.cpp`) | Named cursors become Qt cursors over the requesting client's surface; tablet tools accepted, not shown yet |
| `wp_presentation` | Qt (`PresentationTime`), fed by Prism's `PresentationFeedback` | Frames reported at swap time (`CLOCK_MONOTONIC`) |
| `zwp_idle_inhibit_manager_v1` | Qt (`IdleInhibitManagerV1`) | Inhibitors are recorded on surfaces, for the Phase 3 idle and lock policy |
| `zwp_text_input_manager_v2`/`v3`, `qt_text_input_method_manager_v1` | Qt | Input methods |

Prism's own protocols are generated from the XML in `compositor/protocols/`
(wayland-protocols 1.47). To verify them on the wire, run:

```bash
tools/dev/prism-protocol-check.sh ~/qtos-build/linux-release
```

The script starts a nested Prism with real clients and checks three things:
the advertised globals, xdg-decoration and fractional-scale `preferred_scale`
with a Qt client, and wp_presentation feedback with `weston-presentation-shm`.
It prints one line per check and exits with the number of failures.

## WSLg

Prism runs nested under WSLg on Windows 11; see
[BUILDING.md](../docs/BUILDING.md#linux-on-wsl2-windows-workstation) for the
toolchain and Qt kit. Specifics:

- Prism's own window is a Wayland client of WSLg's Weston (`WAYLAND_DISPLAY=wayland-0`).
- WSLg's Mesa (d3d12) lacks `EGL_WL_bind_wayland_display`, so Prism logs a
  hardware-integration warning and clients use shared-memory buffers. Mesa's
  `libEGL`/`ZINK` warnings at start are expected.
- Qt 6 tries Wayland before X11, so pointing `WAYLAND_DISPLAY` at Prism is
  enough to open a Qt app in it. `QT_QPA_PLATFORM=wayland` just makes the
  choice explicit. WSLg also exports `DISPLAY`, so non-Qt toolkits may still
  choose X11, which shows them outside Prism.
- Weston's demo clients (`apt install weston`: `weston-terminal`,
  `weston-simple-egl`, …) are handy clients with client-side decorations.
- `--screenshot <file> --delay <ms>` saves the output and exits, for
  automated checks.
