# Prism shell specification

The Prism shell (`QOR.Shell`, `prism/shell/`) is the desktop: the lock
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

- **Left**: the QOR OS mark (opens Beam), the active window's title (or
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
<kbd>Alt</kbd>+<kbd>Space</kbd>, the QOR OS mark or the Shelf.

- A scrim plus a 24 px-radius glass panel at 16% of screen height, up to
  860 px wide. A results list on the left (56%) with section headers, and a
  preview pane on the right.
- **Sources**: apps (with ARQADE and QQ Studio under Prism, where they are installed),
  actions (appearance, Overview, media, lock, Control Center, Wi-Fi,
  Bluetooth, Focus, Reduce Motion, Reduce Transparency, living wallpaper,
  test notification, Space 1–3, eight accent presets), settings pages (deep
  links), and files (simulated index).
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
  A client's new window takes the keyboard only once it has drawn, and only if that
  is a theft from nobody: on an empty desktop, from the same client, or if Prism
  started it. Otherwise it asks for attention. It never does on the lock screen or
  over a shell surface ([QOR-0018](../decisions/QOR-0018-the-security-and-quality-bar.md)).
- **Window menu**: right-click the title bar for Minimize, Maximize or
  Restore, Move to Space 1–3, and Close.
- **Wayland clients** (under `prism`): the same frame. Content is a
  `ShellSurfaceItem` below the title bar. Close asks the client
  (`sendClose`), and size changes are sent as xdg configures. A client's
  window opens at the size the client asks for, within the work area: it
  appears once the client has drawn at that size, and flies in from there.
- **The products** (under `prism`): ARQADE, QOR Media, QQ Studio and the QQ
  Player, which Prism starts itself. For ARQADE and QOR Media Prism plans the
  window before the product shows it, at the product's own size where there
  is room. QQ Studio and the QQ Player open at the size they ask for, as any
  client does, and the Player plays one game at a time: asked for another,
  it brings the one playing forward and says it is already open. Prism names
  each product's window and gives it its icon from Prism's own record.
  Closing the window ends the product. A toast says so when a product
  crashes, exits with an error, or never connects.

### Spaces

Three Spaces on a horizontal strip, moved by a `smooth` spring. The
wallpaper parallaxes 3% of the width per Space. Switch with
<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>←</kbd>/<kbd>→</kbd>,
<kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>3</kbd>, Rail dots, the Overview switcher,
or a two-finger horizontal swipe on the empty desktop (touchpad, 140 px
threshold).

Every window, first-party or Wayland client, belongs to one Space and rides
the strip with it. Windows of Spaces out of view are not drawn.

- **Take the active window along**:
  <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd>
  or <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>1</kbd>…<kbd>3</kbd> moves it and
  switches with it. Beam has "Move Window to Space 1–3".
- **Send a window away**: Move to Space in the window menu leaves you where
  you are; the topmost window left on the current Space takes focus.
- **In Overview**, drag a window onto a Space chip. The chip under the
  pointer lights up in the accent colour; dropping anywhere else returns the
  window to the grid.
- **Clients sleep off-screen.** A Wayland client on a Space out of view is
  taken off the output (`wl_surface.leave`) and gets no frame callbacks, so
  a well-behaved client stops drawing until its Space comes back.
  `prism_e2e` checks this with an animating client.

### Overview

<kbd>Ctrl</kbd>+<kbd>↑</kbd>, <kbd>F3</kbd> or the Shelf button.

- Windows of the current Space spring into a centred grid (columns about
  √(n·aspect/1.6), scale ≤ 0.82, last row centred). Thumbnails stay live and
  mipmapped.
- The wallpaper pulls focus (blur 90%) and zooms 3%. The Space switcher
  chips appear at the top.
- Hover outlines a window in the accent color. Click to focus it and exit.
  Click empty space to exit. Drag a window onto a Space chip to move it
  there.

### Home and the Library (`Home.qml`, `HomeShelf.qml`, `HomeCard.qml`, `library.h`)

Guide, or Beam's **Home** and **Library**. Where a controller lands, over the
desktop's frosted glass.

- **Home** has three rows: **Continue** (what was played or opened last, a
  film or song with how far it got), **Games**, and the newest **Media**.
- **The Library** shows everything, chosen by **Everything**, **Games**,
  **ARQADE**, **Made by you** or **Media**.
- Every card's art is drawn from its hue (Kit's `Artwork`) until it has its
  own, with its glyph.
- **Moving:** the d-pad or the arrow keys go from card to card, along a row or
  through the grid, and on Home Up and Down go a row at a time. Up from the
  top reaches the tabs, then Left and Right on them change the page. LB and RB
  go between Home and the Library.
- **Choosing:** South, Return or Space plays a game in the QQ Player or in
  ARQADE, or opens a file in QOR Media, where it stopped. East or Esc closes.
- **Hints** show those buttons with the controller's own glyphs.

The Library (`qor::Library`) finds what it shows until the QOR core's index
(Phase 2):

- **QOR OS's QQ games:** folders that have scenes in `QOR_GAMES`, else in
  `share/qor-os/games` beside Prism and in the data folder.
- **The person's own:** such folders in Documents, two deep, that QQ Studio made or
  they trusted (`qor-os/own-projects.json` in their data folder). A folder that is
  merely there, a download unpacked, is not listed as theirs.
- **ARQADE's eight games**, Ever Rift and Darkveil among them, while ARQADE can start.
- **Video, music and pictures** in the person's folders, three deep.

It reads them on a worker thread when the shell starts and when Home opens,
at most every ten seconds. It keeps what was played and where QOR Media last
stopped (its `position` reports) in `library-history.json`, in the shell's
data folder.

### Themes and widgets

Themes and widgets come from the Store ([STORE.md](../STORE.md)).

- **Themes** set the accent, a preferred appearance, a tint for every
  neutral surface (hue and chroma, sprung like the accent) and the Aurora
  wallpaper's seed and intensity. Choose one in Settings › Appearance, the
  Store or Beam.
- **Widgets** (`WidgetLayer.qml`) sit on glass cards over the wallpaper and
  under the windows, the same on every Space. They use a grid of 152 px
  cells from the work area's top left, clear of notifications. They fade out
  for Overview and the lock screen.
- **Editing widgets**: right-click the desktop (or press and hold on a
  touchscreen) and choose Edit Widgets, or use Beam. The windows step aside
  and the gallery (`WidgetGallery.qml`) replaces the Shelf, showing live
  previews of installed widgets. Click one to add it, drag a widget to a free
  cell, × removes one; Escape or Done finishes.
- **The desktop menu** also offers Change Theme… and Get Themes and
  Widgets….

### Displays

Prism drives every display: nested, `prism --outputs N` opens N windows of
the host; on bare metal (eglfs/KMS) each connected screen is an output, side
by side from left to right.

- The **main display** has the whole shell: Rail, Shelf, notifications,
  Beam, Control Center and widgets. The others (`DisplayDesktop.qml`) show the
  wallpaper and their own windows.
- **Spaces, Overview and the lock screen span all displays.** Switching Space
  slides every display together. Overview arranges each display's windows on
  that display, labelled "Display 2" and so on.
- **Moving windows**: the window menu's Move to Display N;
  <kbd>Meta</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd>; or drag the
  title bar off a display's edge and let go, and the window lands on the next
  display under the pointer.
- **One active window** across all displays; keyboard shortcuts act on it,
  from whichever display has the keyboard. New clients open on the main
  display (`--spread-clients` spreads them for benchmarks).
- In-window frost samples the wallpaper of the display the window is on
  (`Materials.desktopFor(Window.window)`), and Wayland clients are on the
  output of the display that shows them, so they get its frame callbacks.

### Gestures

Swipes with three or more fingers, on a touchpad or a touchscreen, belong to
the shell. It takes them for the whole window before any app or Wayland
client sees them (`SystemGestures` in `QOR.Input`).

| Swipe | Effect |
|-------|--------|
| Left / right | Next / previous Space. The desktop follows the fingers and resists past the first and last Space. On release it settles on the nearest Space, and a flick carries on to the next |
| Up | Opens Overview as far as the fingers go (300 px for all the way). Windows sit between their places and the grid. Release past halfway, or flick, to open |
| Down | Closes Overview the same way |

A swipe picks its axis after 10 px and keeps it. On release the springs carry
on with the fingers' velocity, measured over the last 100 ms, so fingers
that stop before lifting do not fling. Two-finger touchpad swipes on the
empty desktop still switch Spaces, and two fingers on a touchscreen stay with
the app.

Where swipes come from:

- **Touchpads in nested Prism**: the host compositor's pointer gestures
  (`zwp_pointer_gestures_v1`, from GNOME, KDE, Weston and others), which Qt
  delivers as native gestures.
- **Touchpads in the Windows simulator**: Windows keeps multi-finger
  touchpad gestures for itself, so QOR OS reads precision touchpads through Raw
  Input. Set Settings › Bluetooth & devices › Touchpad › Three-finger gestures
  › Swipes to Nothing, or Windows acts on the same swipe. `QOR_TOUCHPAD=off`
  turns the reader off; automated runs do.
- **Touchscreens, everywhere**: three fingers moving together past 16 px. The
  apps under them get a touch cancel.
- **Bare metal**: Qt's libinput backend reports no gestures. Prism's KMS
  backend will read them from libinput (Phase 3).

## Keyboard reference

| Shortcut | Action |
|----------|--------|
| <kbd>Ctrl</kbd>+<kbd>Space</kbd> · <kbd>Alt</kbd>+<kbd>Space</kbd> | Beam |
| <kbd>Ctrl</kbd>+<kbd>↑</kbd> · <kbd>F3</kbd> | Overview |
| <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>←</kbd>/<kbd>→</kbd> | Previous / next Space |
| <kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>3</kbd> | Go to Space |
| <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd> | Take the active window to the previous / next Space |
| <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>1</kbd>…<kbd>3</kbd> | Take the active window to a Space |
| <kbd>Meta</kbd>+<kbd>Shift</kbd>+<kbd>←</kbd>/<kbd>→</kbd> | Move the active window to the display on the left / right (Prism) |
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
`runAction(id, args)`, `adoptSurface(shellSurface, toplevel, product)`,
`openInMedia(path, kind)`, `playQQGame(scene)` and `windowLayer`. `qor-shell --benchmark` drives `setSurface` and
`switchSpace` through these.

## Signature moments: status

| Moment (VISION §5) | Status |
|--------------------|--------|
| Unlock focus pull | Implemented |
| Launch from Shelf icon | Implemented |
| Overview with live windows | Implemented (keyboard, Shelf and three-finger swipes that follow the fingers) |
| Beam streaming results with preview | Implemented |
| Appearance change through OKLab | Implemented (uniform morph. Radial sweep is Phase 4) |
| Hero handoff between apps | Phase 4 (`qor_hero` protocol) |
