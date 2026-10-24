# apps — QTOS.Apps

First-party apps, hosted by the shell inside Prism windows. Apps depend on
QTOS.Kit, the Lumen style and QTOS.Services, **never** on the shell.

| App | File | Highlights |
|-----|------|------------|
| Files | `FilesApp.qml` | Locations and tags in a Frost sidebar, grid and list views, live filter, inspector; Beam deep links (`folder`, `select`) |
| Music | `MusicApp.qml` | Now-playing hero with colour-matched artwork glow, seek, transport, volume, live queue (media service) |
| Notes | `NotesApp.qml` | Note list and editor, persisted with `QtCore.Settings` |
| Terminal | `TerminalApp.qml` | GPU-rendered console with history; commands wired to services (`theme`, `accent`, `wifi`, `play`, `notify`, `open`, `lock`, `qtfetch`) |
| Monitor | `MonitorApp.qml` | Qt Graphs CPU/GPU chart, stat tiles, live frame time from `FrameStats` |
| Settings | `SettingsApp.qml` | Appearance, Display, Sound, Wi-Fi, Bluetooth, Notifications, Accessibility, About; deep links (`page`) |

Shared: `AppRegistry.qml` (catalogue, pinned order, settings pages, sender
identities, components), `FrostSidebar.qml`, `FileIndex.js` (simulated
index, also used by Beam).

App contract: a root item with `property real topInset` (title bar height)
and `property var launchArgs`. See
[DEVELOPMENT.md → Add an app](../docs/DEVELOPMENT.md#add-an-app).
