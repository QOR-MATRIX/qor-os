# prism — shell, simulation host and compositor

| Path | Contents |
|------|----------|
| `shell/` | **QTOS.Shell**: `Main` (simulator window), `Desktop` (orchestration, layering, shortcuts, actions), `WindowLayer` + `AppWindow` (window management), `Rail`, `LiveActivity`, `MediaPopover`, `Shelf` + `ShelfButton`, `Beam` + `search.js`, `ControlCenter`, `NotificationStack` + `NotificationCard`, `LockScreen`, `SpaceSwitcher` |
| `host/` | **qtos-shell**: CLI options, embedded simulator, screenshot, `--benchmark`, GPU preference (`gpupreference.cpp`) |
| `compositor/` | **prism** (Linux, experimental): `Compositor.qml` hosts the same `Desktop` in a Qt Wayland Compositor scene; xdg-shell, server-side decorations, text input |

Behaviour spec: [docs/design/SHELL.md](../docs/design/SHELL.md). Compositor
strategy: [ADR-0003](../docs/adr/0003-compositor-strategy.md).

Run the compositor nested on Linux:

```bash
cmake --preset linux-release && cmake --build --preset linux-release
./build/linux-release/prism --unlocked &
WAYLAND_DISPLAY=wayland-1 QT_QPA_PLATFORM=wayland some-qt-app
```

Prism takes the next free socket in `$XDG_RUNTIME_DIR` (`wayland-0`,
`wayland-1`, …). Inside an existing Wayland session that is usually
`wayland-1`; check with `ls $XDG_RUNTIME_DIR/wayland-*`.
