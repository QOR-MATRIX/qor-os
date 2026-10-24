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
