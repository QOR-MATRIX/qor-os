# Prism shell specification

The Prism shell (`QTOS.Shell`, `prism/shell/`) is the desktop: the lock
screen, the Rail, the Shelf, Beam, Control Center, notifications, windows,
Spaces and Overview. This document specifies behaviour as implemented. The
visual language is in [LUMEN.md](LUMEN.md).

## Surfaces

### Lock screen (`LockScreen.qml`)

- Shown at start (unless `--unlocked`) and on lock. Drawn above all windows,
  so clients can never cover it.
- Shows the date (`title3`), a curve-rendered `hero` clock that updates every
  second while visible, the latest three notifications on glass, the user
  avatar (gradient, initials), the name, and a glass password field.
- **Unlock** (Enter or the arrow button) runs the focus pull on the `gentle`
  spring: the blurred wallpaper sharpens, wallpaper zoom relaxes from 1.06,
  windows fade in from 96.5% scale, the Rail drops in from above and the
  Shelf rises from below.
- Simulation: any password unlocks (labelled on screen).

### Rail (`Rail.qml`)

A floating glass bar, 34 px high with 8 px margins and radius 13.

- **Left**: the QTOS mark (opens Beam), the active window's title (or
  "Desktop"), and Space dots (the current Space is a 14 px pill; tap to switch).
- **Centre**: the **Live Activity** capsule (`LiveActivity.qml`). It shows
  media artwork, title, artist and a live equaliser. Its width springs to the
  content. Tap it for the Now Playing popover (`MediaPopover.qml`) with
  transport and seek.
- **Right**: the status cluster: Focus moon (when on), Wi-Fi bars (pulses
  while connecting), Bluetooth (connected variant), volume level, battery
  percentage and icon (critical color at 15% or below), and date and time
  (locale formats, updated on minute boundaries). Tap it for Control Center.

### Shelf (`Shelf.qml`, `ShelfButton.qml`)

A floating glass dock, radius 24, centred 12 px above the bottom edge.

- Order: Beam, divider, pinned apps then running unpinned apps, divider, Overview.
- **Magnification**: each icon scales by `1 + 0.24·p²`, where `p` falls off
  linearly over 120 px from the pointer, on `snappy` springs, anchored at the bottom.
- **Labels**: a raised tooltip above the hovered icon.
- **Running indicator**: a 4 px dot under the icon.
- **Launch hop**: launching a non-running app kicks the icon's spring with
  velocity −460 px/s (`bouncy`).
- The Shelf exposes `originFor(appId)` so windows open from, and minimise
  to, their icon.

### Beam (`Beam.qml`, `search.js`)

The universal command bar. <kbd>Ctrl</kbd>+<kbd>Space</kbd>,
<kbd>Alt</kbd>+<kbd>Space</kbd>, the QTOS mark or the Shelf.

- A scrim plus a 24 px-radius glass panel at 16% of screen height, up to
  860 px wide. A results list on the left (56%) with section headers, and a
  preview pane on the right.
- **Sources**: apps, actions (appearance, Overview, media, lock, Control
  Center, Wi-Fi, Bluetooth, Focus, Reduce Motion, Reduce Transparency, living
  wallpaper, test notification, Space 1–3, eight accent presets), settings
  pages (deep links), and files (simulated index).
- **Ranking**: fuzzy subsequence matching with exact, prefix and word-start
  bonuses, consecutive-run bonuses and length penalties. Sections are
  weighted (Apps 1.15, Actions 1.0, Settings 0.95, Files 0.9), kept
  contiguous, and ordered by their best hit. At most 12 results.
- **Calculator**: a safe shunting-yard evaluator (no `eval`):
  `+ − × ÷ * / % ^`, parentheses, unary minus, `sqrt sin cos tan log ln abs
  round floor ceil`, `pi e tau`. Enter copies the result.
- **Keys**: <kbd>↑</kbd>/<kbd>↓</kbd> move, <kbd>Enter</kbd> activates,
  <kbd>Esc</kbd> closes. Hovering selects.
- Results enter staggered by 18 ms on `snappy` springs.

### Control Center (`ControlCenter.qml`)

A 368 px glass panel anchored under the status cluster. It grows from the
top-right on `snappy`.

- **Connectivity tiles**: Wi-Fi and Bluetooth. The badge toggles the radio,
  and the chevron expands an inline list of networks or devices (tap to join
  or connect; checkmarks; lock for secured; battery for devices).
- **Quick toggles**: Focus, Dark/Light, Night Light, Living wallpaper.
- **Sliders**: Display brightness (20–100%) and Sound volume (tap the speaker to mute), with the output device shown.
- **Now Playing** tile with transport.
- **Accent swatches** (8 presets).
- **Footer**: battery and time remaining, Settings, Lock.

### Notifications (`NotificationStack.qml`, `NotificationCard.qml`)

- Banners stack top-right under the Rail (up to 4), slide in on `smooth`
  springs, stay 6.5 s, pause while hovered, and offer a close button on hover.
  Tapping opens the sending app (if it is an app).
- Banners are suppressed in Focus, on the lock screen and while Control Center
  is open. History keeps the last 30, and the lock screen shows the latest 3.
- Sender identity (name, icon, hue) comes from `AppRegistry.identity()`.

### Windows (`WindowLayer.qml`, `AppWindow.qml`)

- **Frame**: an opaque `Surface` with elevation 5 (focused) or 3, radius 14,
  and a crisp hairline above the content. Content is clipped by `RoundMask`.
- **Title bar**: 40 px, drawn by Prism over the app's unified toolbar. Title
  centred (dims when unfocused). Minimise, maximise and close at the right;
  close hovers in `critical`.
- **Open**: a cascade around the work-area centre. The window flies out of
  its Shelf icon (scale 6%→100%, `smooth`).
- **Close**: shrinks to 94% and fades (`snappy`).
- **Minimise**: flies into the Shelf icon. Launching the app again restores it.
- **Move**: drag the title bar; tracking is 1:1. Dragging a maximised window
  restores its size under the pointer.
- **Snap**: release at the left or right edge for a half, or above the work
  area to maximise. An accent-tinted preview slab shows the target while
  dragging.
- **Resize**: right edge, bottom edge or bottom-right corner (min 420×300).
- **Maximise**: double-click the title or press the button. Animates to the
  work area and back.
- **Focus**: any press activates the window (z-order raise, focus to content).
- **Wayland clients** (under `prism`): the same frame. Content is a
  `ShellSurfaceItem` below the title bar. Close asks the client
  (`sendClose`), and size changes are sent as xdg configures.

### Spaces

Three Spaces on a horizontal strip, moved by a `smooth` spring. The
wallpaper parallaxes 3% of the width per Space. Switch with
<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>←</kbd>/<kbd>→</kbd>,
<kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>3</kbd>, Rail dots, the Overview switcher,
or a two-finger horizontal swipe on the empty desktop (touchpad, 140 px
threshold).

### Overview

<kbd>Ctrl</kbd>+<kbd>↑</kbd>, <kbd>F3</kbd> or the Shelf button.

- Windows of the current Space spring into a centred grid (columns about
  √(n·aspect/1.6), scale ≤ 0.82, last row centred). Thumbnails stay live and
  mipmapped.
- The wallpaper pulls focus (blur 90%) and zooms 3%. The Space switcher
  chips appear at the top.
- Hover outlines a window in the accent color. Click to focus it and exit.
  Click empty space to exit.

## Keyboard reference

| Shortcut | Action |
|----------|--------|
| <kbd>Ctrl</kbd>+<kbd>Space</kbd> · <kbd>Alt</kbd>+<kbd>Space</kbd> | Beam |
| <kbd>Ctrl</kbd>+<kbd>↑</kbd> · <kbd>F3</kbd> | Overview |
| <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>←</kbd>/<kbd>→</kbd> | Previous / next Space |
| <kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>3</kbd> | Go to Space |
| <kbd>Ctrl</kbd>+<kbd>W</kbd> | Close window |
| <kbd>Ctrl</kbd>+<kbd>M</kbd> | Minimise window |
| <kbd>Ctrl</kbd>+<kbd>,</kbd> | Settings |
| <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>L</kbd> | Lock |
| <kbd>Esc</kbd> | Close Beam, Control Center, Overview or popovers |
| <kbd>F11</kbd> | Toggle full screen (simulator window) |

Shortcuts avoid the Windows key in simulation because the host OS owns it.
On a device, <kbd>Super</kbd> bindings are added in Prism.

## Shell API (for hosts and automation)

`Desktop` exposes `setSurface(name)`, `toggleSurface(name)`,
`launch(appId, args)`, `lock()`, `unlock()`, `switchSpace(index)`,
`runAction(id, args)`, `adoptSurface(shellSurface, toplevel)` and
`windowLayer`. `qtos-shell --benchmark` drives `setSurface` and
`switchSpace` through these.

## Signature moments: status

| Moment (VISION §5) | Status |
|--------------------|--------|
| Unlock focus pull | Implemented |
| Launch from Shelf icon | Implemented |
| Overview with live windows | Implemented (keyboard and Shelf. Three-finger gesture under Prism, Phase 1) |
| Beam streaming results with preview | Implemented |
| Appearance change through OKLab | Implemented (uniform morph. Radial sweep is Phase 4) |
| Hero handoff between apps | Phase 4 (`qtos_hero` protocol) |
