# Controllers

QOR OS can be driven entirely with a game controller. Keyboard and mouse stay
the default: when a controller connects, QOR OS asks whether to switch to the
**controller interface** (or switches by itself, if you chose that), and it
returns to keyboard and mouse when the last controller disconnects.
Keyboard and mouse keep working throughout.

| Piece | Where | What it does |
|-------|-------|--------------|
| Controller backends and `GamepadManager` | [`input/src/`](../input/src/) (`qor_gamepads`, in `qor_input`) | Find controllers, read them, present one standard layout; Qt Core and Gui only, so QQ links it too ([below](#games)) |
| `Gamepads` | [`input/qml/gamepads.h`](../input/qml/gamepads.h) (`QOR.Input`) | The connected controllers and their live state, for QML |
| `ControllerNavigator` | [`input/qml/navigator.h`](../input/qml/navigator.h) | Turns a controller into pointer, scroll and key input, and shell actions |
| `ControllerGlyph` | [`input/qml/ControllerGlyph.qml`](../input/qml/ControllerGlyph.qml) | A button as printed on the controller in use |
| Prompt, cursor, hints, keyboard | [`prism/shell/`](../prism/shell/) `Controller*.qml` | The controller interface in the shell |
| Settings › Controllers | [`apps/SettingsApp.qml`](../apps/SettingsApp.qml) | Preferences, connected controllers, a live tester |
| `qor-gamepads` | [`tools/gamepads/`](../tools/gamepads/) | Lists controllers and streams their state |

## Supported controllers

Every controller is presented in one standard layout (the one SDL, the Linux
gamepad specification and the W3C Gamepad API share): South, East, West,
North, two shoulders, two analogue triggers, Back, Start, Guide, two sticks
with clicks, and the D-pad. Glyphs follow the controller: A B X Y for Xbox,
✕ ○ □ △ for PlayStation, B A Y X for Nintendo.

| Controller | Windows | Linux |
|------------|---------|-------|
| Xbox 360, Xbox One, Xbox Series (USB, wireless adapter, Bluetooth) | XInput, with the Guide button, battery and rumble | `xpad` / `hid-microsoft` through evdev, with rumble |
| SteelSeries Stratus Duo (and other XInput pads: 8BitDo in X-input mode, PowerA, PDP, Steam Input's virtual pads) | XInput | `xpad` through evdev |
| DualShock 4 (both revisions, wireless adapter) | Raw HID over USB and Bluetooth: rumble, light bar, battery | `hid-playstation` through evdev, with rumble |
| DualSense, DualSense Edge | Raw HID over USB and Bluetooth: rumble, light bar, battery | `hid-playstation` through evdev |
| Stratus Duo in Android (Bluetooth) mode, other HID game pads | Generic HID, best effort (Android-style and DirectInput-style layouts) | Any pad the kernel exposes with gamepad buttons |
| Nintendo Switch Pro | Generic HID (Bluetooth) | `hid-nintendo` through evdev |

No third-party libraries are involved. On Windows, QOR OS loads XInput at run
time (including `XInputGetStateEx`, for the Guide button) and reads HID
devices with the Windows HID and SetupAPI functions; arrivals and removals
come from Configuration Manager notifications. On Linux it reads
`/dev/input/event*` directly (access comes from logind's `uaccess` rule for
game pads) and follows hotplug through the directory.

**Light bar.** PlayStation controllers glow in the accent colour (Settings ›
Controllers › Light bar; Windows). **Battery** is shown where the controller
or its Linux driver reports it.

**The same controller twice.** Steam Input and some remappers present a
PlayStation controller as an Xbox one as well. Every controller's input is
combined before the interface acts on it, so a press never counts twice.

### Details

- Sticks have a radial dead zone (16%) with the live range rescaled from
  zero; triggers have a 4% dead zone and count as buttons past half way
  (with hysteresis).
- A press and release between two polls (8 ms) is never lost: the HID
  readers latch presses.
- Bluetooth PlayStation controllers are switched to full reports once (a
  feature-report read) and their output reports carry the CRC-32 they need.
- Report layouts follow the Linux kernel's `hid-playstation` driver.
- The Linux `xpad` driver reports the Xbox X and Y buttons as `BTN_X` and
  `BTN_Y`, which the gamepad specification names north and west. QOR OS swaps
  them for Xbox controllers.

## The controller interface

| Input | Action |
|-------|--------|
| Left stick | Moves a cursor (accelerating; Settings › Controllers › Cursor speed) |
| South (A, ✕) | Click; after D-pad moves, activate what is focused |
| West (X, □) | Right-click |
| East (B, ○) | Back (Escape): closes Beam, Control Center, Overview, menus |
| North (Y, △) | Beam |
| Start (Menu, Options) | Control Center |
| Back (View, Share) | Overview |
| Guide (Xbox, PS) | Home, and again closes it; on the lock screen, unlock |
| D-pad | Arrow keys; Tab and Shift+Tab where nothing takes arrows |
| Right stick | Scroll |
| LB / RB (L1 / R1) | Previous / next Space; on Home, Home / the Library |
| LT / RT (L2 / R2) | Volume down / up (held: repeats) |
| Left stick click | On-screen keyboard |
| Right stick click | Play or pause |

Pointer, wheel and key events enter through `QWindowSystemInterface`, as a
real device's do, so every surface, app and (in Prism) every Wayland client
responds without knowing about controllers. Key strokes press their
modifier keys around the key, as a keyboard does: Prism's compositor
rebuilds key events from the keys it has seen go down. A real mouse moving
takes over from the controller's cursor at once.

**Home and the Library** ([SHELL.md](design/SHELL.md#home-and-the-library-homeqml-homeshelfqml-homecardqml-libraryh))
are moved through by the d-pad, card by card, and South plays a game or opens
a file.

**Hints.** A glass bar at the bottom left shows what the buttons do right
now, with the glyphs of the controller in use. It fades after a few seconds
without input.

**On-screen keyboard.** It opens when a text field takes focus (for example
Beam's), and with the left stick's click. D-pad or left stick choose a key;
South types it; West deletes; North types a space; Start presses Enter;
LB and RB move the text cursor; LT is Shift; East closes the keyboard.

**Vibration.** A light tap on selections and shell actions (Settings ›
Controllers › Vibration).

### Games

A game reads the controller itself, through the same backends: QQ's `Input`
(`products/qq/runtime/input.cpp`) takes every controller `GamepadManager`
reports, combined, as its gamepad. That is the QQ Player, QQ Studio while it
plays, and ARQADE's games, which run on QQ's runtime. Its sticks arrive with
QOR.Input's dead zone already out, so QQ does not take out its own as well,
and Guide is never a game's. A controller is heard only by the application
in front: one pressed in the shell or another window moves nothing in a game
behind it.

A game's actions (`InputAction`) bind the pad's buttons by name, and the left
stick's four ways too: `LS Up`, `LS Down`, `LS Left` and `LS Right`, each held
while the stick leans past half way toward it, so a diagonal holds two. A game
that moves a way at a time, as ARQADE's rewarded games do so their runs replay
exactly, takes the stick as well as the d-pad that way (Ever Rift and Darkveil do).

Under Prism, a product whose record says it plays with the controller
(ARQADE and the QQ Player) has it while its window has the focus and nothing
of the shell is open:

- the controller interface steps aside, with its cursor and hints, so a press
  is the game's alone;
- Guide brings the shell back: Overview, where East returns to the game;
- a controller connected then is the game's, and nothing is asked.

QQ Studio is not such a product, since it is mostly edited with keyboard and
mouse: while it plays with the controller interface on, both answer.

**ARQADE** takes the controller because its games read it themselves. Were
the controller interface to answer as well, each d-pad press would also
arrive as an arrow key, and East as Esc, which puts a game away. Its home
answers the controller through QQ's `Input` too, with actions it defines
only while the home shows: a game hears every action pressed while it plays
and keeps it in its run.

| Input | On ARQADE's home |
|-------|------------------|
| D-pad, arrow keys | The keyboard's focus to the nearest card, button or tab that way, its ring showing; held, it repeats. The page scrolls to keep the focus in view |
| South (A, ✕), Return | Plays the card focused, or presses the button, as the button comes up: the game it starts never finds it held, or takes it as its own first press. Space presses too |
| Back (View, Share) in a game | Puts the game away, as Esc does; the focus is on its card again. ARQADE reads it from the controller service itself (`src/controllerback.h`), so the game never hears it |

### Applications

Every other application keeps the controller interface. Prism gives its
window the d-pad as the arrow keys, South (once the d-pad has moved) as
Return, and East as Esc, so an application answers the controller by
answering the keyboard. QOR Media does:

- on what is open (the picture, the player, the pages), the arrows keep their
  own meaning: next picture, seeking, scrolling;
- Return takes the keyboard to the controls: the player's Play, or the top
  bar's way back;
- on a control, the arrows go to the next control, or back with Up and Left,
  as Tab and Shift+Tab do, and Return or Space presses it. The volume keeps
  its own Left and Right;
- Esc from a control the keys reached comes back to what is open. A second Esc
  goes back to the Library.

In the editor, Return types a new line, so its bar is reached by Tab.

### Preferences

| Setting | `Settings` property | Default |
|---------|---------------------|---------|
| Controller interface | `controllerInterface` (not remembered) | off |
| Switch automatically | `controllerAutoSwitch` | off: ask |
| Cursor speed | `controllerCursorSpeed` | 50% |
| Vibration | `controllerVibration` | on |
| Light bar | `controllerLightbar` | on |

## Tools and tests

```bash
qor-gamepads                  # list controllers, then stream their state
qor-gamepads --seconds 5
qor-gamepads --rumble         # a short vibration on each
qor-gamepads --light ff6b00   # light bar colour (PlayStation)
```

`QOR_GAMEPADS=virtual` keeps only virtual controllers, and `QOR_GAMEPADS=off`
none. Every suite runs with virtual ones, so a controller on the machine never
joins in: QOR OS's own through `tests/CMakeLists.txt`, QQ's and ARQADE's built
with `products/qq/tests/virtualpads.cpp`. `--screenshot`, `--benchmark` and
`--script` runs set it too. `QT_LOGGING_RULES="qor.input.*=true"` logs what the
backends find; a scan's list of every HID device is off unless that asks for
it.

Shell scripts drive a virtual controller:

```json
{ "gamepad": { "connect": "playstation", "name": "DualShock 4" } },
{ "gamepad": { "tap": "south" } },
{ "gamepad": { "stick": [0.8, 0], "ms": 400 } },
{ "gamepad": { "press": "rt" } }, { "gamepad": { "release": "rt" } },
{ "gamepad": { "disconnect": true } }
```

Buttons: `south east west north lb rb lt rt back start guide ls rs up down
left right`. The state has `controller` (`gamepads`, `prompt`, `keyboard`,
`cursorVisible`, `cursor`).

- `tst_input`:
  - DualShock 4 and DualSense reports (USB, basic and full Bluetooth), with
    battery;
  - CRC-32 and output reports;
  - the XInput, evdev and generic HID mappings;
  - dead zones;
  - the manager's edges and merging of duplicate controllers;
  - the navigator driving a real Qt Quick window: actions, cursor, clicks,
    typing, the keyboard's routing, and a real mouse taking over.
- `shell_e2e`:
  - the prompt (B: not now; A: switch);
  - Beam with the on-screen keyboard;
  - closing with B, Control Center, Spaces on the shoulders and the cursor;
  - leaving the interface when the controller disconnects.
- [`tools/dev/uinput-gamepad.py`](../tools/dev/uinput-gamepad.py) creates a
  kernel game pad with uinput (root), to exercise the Linux backend and
  hotplug without hardware.
