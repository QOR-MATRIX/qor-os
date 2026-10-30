# Development guide

Conventions, recipes and debugging techniques for working on QOR OS. Read
[ARCHITECTURE.md](ARCHITECTURE.md) first, and [`AGENTS.md`](../AGENTS.md) for
the rules that bind every change. The products (QQ, ARQADE, QOR Media) keep
the conventions they had in demiurge-chain; see [UPSTREAM.md](UPSTREAM.md)
before changing them.

## 1. Daily loop

```powershell
. .\tools\dev\Enter-QorShell.ps1
cmake --build --preset windows-release            # incremental build (QML is compiled too)
.\tools\dev\run.ps1 -- --unlocked --open files    # run
ctest --preset windows-release                    # test
cmake --build --preset windows-release --target all_qmllint
```

For visual work, `tools\screenshots\capture.ps1 -Scenes desktop,beam -OutDir $env:TEMP\shots`
produces deterministic frames you can compare before and after a change.

## 2. C++ conventions

- C++20, Qt 6.12. Namespaces: `qor` (Kit, Services), `qor::sim`,
  `qor::lumen`, `qor::host`, `qor::color`.
- Strict Qt string hygiene is on: `QT_NO_CAST_FROM_ASCII`,
  `QT_NO_CAST_TO_ASCII`, `QT_NO_URL_CAST_FROM_STRING`, `QT_USE_QSTRINGBUILDER`.
  Use `u"…"_s` (`using namespace Qt::StringLiterals`) and wrap QStringBuilder
  expressions in `QString(…)` when a `QString` is required (for example
  `QJsonValue`).
- QML-exposed types use `QML_ELEMENT` / `QML_SINGLETON` with `FINAL`
  properties and `Q_SIGNALS` / `Q_EMIT`. Singletons shared with C++ use a
  static `instance()` plus `create()` that sets `CppOwnership`.
- Header includes: own header, blank line, project headers, Qt (`<QtModule/QClass>`), std.
- Formatting: `.clang-format`, enforced with clang-format **20.1.8**
  (`requirements-dev.txt`, pre-commit, CI). Do not hand-format.
- Warnings: `/W4` (MSVC) or `-Wall -Wextra -Wpedantic …`. Third-party headers are
  external. `ci-windows` builds with warnings as errors.
- Every file starts with `// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary`.

## 3. QML conventions

- Start delegate-heavy files with `pragma ComponentBehavior: Bound`. Delegates
  declare `required property` for every model role they use.
- Type every function parameter and return value (`function f(x: real): color`).
  Use `var` deliberately where dynamic access is intended (see
  `AppWindow.configureContent`).
- Colors come from `Theme` roles, sizes from `Tokens`, motion from
  `Motion` presets. **Never hard-code a hex color, radius, font size or
  duration in shell or app code.**
- Animate physical motion with `Spring`, not `NumberAnimation`. Small
  cosmetic transitions (hover color) may use short `ColorAnimation` /
  `NumberAnimation`.
- Shader URLs must use `Qt.resolvedUrl("shaders/x.frag.qsb")`. A plain string
  on a component's root object resolves against the *instantiating* file.
- Avoid these names: `layer` (an `Item` group property), `on` (a QML
  contextual keyword), `print` (reserved). Don't reuse an existing property
  name for an `id` (`qmllint` flags it).
- Icons: `Icon { name: "wifi" }` for Lucide names in `kit/icons/icons.txt`. In
  Lumen controls, `icon.name` takes the same names.
- Accessibility: interactive items set `Accessible.role` / `Accessible.name`,
  support keyboard activation (Space/Return) and show `Theme.focusRing` on
  `activeFocus` / `visualFocus`.
- **Never put a `TapHandler` on an overlay above interactive content.** In Qt
  6.12 a `TapHandler`, even with `gesturePolicy: TapHandler.DragThreshold`,
  leaves a *mouse* press accepted (QTBUG-145896). Qt Quick then stops
  offering the press to items underneath. Handlers still get it; items do
  not: Qt Quick Controls, `TextInput`, and Wayland client surfaces in
  Prism. To observe presses from above, use a `PointHandler`, which always
  leaves them unaccepted (see the activation overlay in `AppWindow.qml`).
  `TapHandler` on the item it belongs to, or on a parent, is fine: parents are
  offered events after their children. `shell_e2e` and `prism_e2e` catch
  regressions.
- `all_qmllint` must stay at zero warnings.

## 4. Performance rules

1. **Render on demand.** No `Timer`, `FrameAnimation` or infinite animation
   that runs while nothing visible changes. Gate them with `running: visible && …`.
2. **Glass via `Backdrop` only.** Never add per-item `MultiEffect` blurs.
   Sample `Materials.scene` (chrome) or `Materials.desktop` (in-window Frost).
3. **Cache expensive shaders** in a layer and animate them with transforms,
   not uniforms (see `Wallpaper.qml`).
4. **Prefer SDF `Surface`** over `Rectangle` + effects for anything rounded,
   stroked or shadowed.
5. Measure with `qor-shell --benchmark 8` before and after (see
   [PERFORMANCE.md](PERFORMANCE.md)) and include the numbers in the PR.

## 5. Recipes

### Change a design token

1. Edit `kit/tokens/lumen.tokens.json`.
2. `python tools/tokens/generate_tokens.py` regenerates `kit/Tokens.qml`,
   `platform/common/lumentokens.h` and `docs/design/tokens.md`.
3. `python -m unittest discover -s tests/tools` checks the contrast
   guarantees (text on accent at every preset hue, and the high-contrast
   targets).
4. Rebuild, run the golden-image tests (below), and update the references if
   the change is intended.

### Add a color role

Add it to `color.roles` in the JSON, regenerate, then expose it in
`kit/Theme.qml` (`readonly property color myRole: resolve(Tokens.color.myRole)`).
Add it to `tests/kit/tst_kit.qml` if it is a core role. If text sits on it or
it is text, add `highContrast` values and a target in
`tests/tools/test_generate_tokens.py`.

### Golden-image tests

`tst_golden` renders the scenes in `tests/golden/scenes/` (Lumen controls,
Kit components, and wallpaper with backdrop and glass) in five variants:

- `dark`
- `light`
- `dark-hc`
- `light-hc` (Increase Contrast)
- `dark-rtl` (right to left)

It compares each render with `tests/golden/references/<scene>-<variant>.png`.
The comparison is perceptual: a pixel differs when its OKLab distance exceeds
0.03 (about 1.5 just-noticeable differences). A case fails when more than 0.3%
of its pixels differ.

That absorbs GPU, driver and graphics-API variation:

- Intel UHD and an RTX 4060 (Direct3D 11) differ by at most 0.023, in the
  backdrop blur.
- Linux (OpenGL on Mesa, WSLg) differs visibly from the Windows references on
  at most 0.005% of pixels, a few antialiased glyph edges.

So one reference set serves every platform. It still catches real changes: a
0.125 lightness shift in `textPrimary` fails every affected case.

```powershell
ctest --preset ci-windows -L gpu                                  # run (opens brief windows)
cmake --build --preset ci-windows --target update_golden          # rewrite the shared references
```

If a platform must render a case differently on purpose, run the test with
`QOR_GOLDEN_UPDATE=platform`. That writes an override to
`tests/golden/references/<os>/`, which takes precedence on that OS.

When a case fails, `build/<preset>/tests/golden-out/` holds `<case>-actual.png`
and `<case>-diff.png`, with visible differences in red. CI uploads them as the
`golden-diff-windows` artifact. To accept an intended change:

1. Inspect the diff images.
2. Run `update_golden`.
3. Review the new references in the pull request's image diff.
4. Commit them with the change.

To add a scene, put a QML file with a root `property bool rtl` in
`tests/golden/scenes/`, add its name to `kScenes` in `tst_golden.cpp`, then
run `update_golden`. A case without a reference is skipped with a message
rather than failed.

### Shell scripts and end-to-end tests

`qor-shell --script file.json` and `prism --script file.json` drive the
shell with synthetic input, then exit with 0 (every step passed) or 4 (a
step failed, with the reason on stderr). Input enters through
`QWindowSystemInterface`, the way a device's (and `QTest`'s) does. It
therefore takes the same path as a person's: shell handlers, controls, and
under Prism on to Wayland clients. The runner is
`prism/shell/shellscript.cpp`; scripts use logical coordinates at any display
scale.

```json
{ "steps": [
  { "call": "launch", "args": ["settings", { "page": "accessibility" }] },
  { "waitFor": { "windows": 1, "timeout": 10000 } },
  { "click": { "item": "reduceMotionSwitch" } },
  { "expect": { "path": "settings.reduceMotion", "equals": true } },
  { "drag": { "from": { "window": "Settings", "part": "titlebar", "at": [140, 20] }, "to": [3, 400] } },
  { "expect": { "window": "Settings", "path": "frameRect", "near": "workArea", "scale": [1, 1, 0.5, 1] } }
] }
```

| Step | Effect |
|------|--------|
| `{"wait": ms}` | Pause |
| `{"waitFor": {"windows": n \| "window": spec \| "gone": spec \| "popups": n, "timeout": ms}}` | Wait for a condition; fail on timeout |
| `{"click": target}`, `{"doubleClick": target}`, `{"rightClick": target}`, `{"move": target}` | Pointer input |
| `{"drag": {"from": target, "to": target \| "by": [dx, dy], "steps": n}}` | Press, move in steps, release |
| `{"scroll": {"at": target, "notches": n}}` | Mouse wheel; positive `n` scrolls down |
| `{"gesture": {"fingers": 3, "by": [dx, dy], "steps": n, "ms": 16, "at": target}}` | A touchpad swipe, as Qt's native gestures. In parts: `{"begin": {"fingers": 3, "at": target}}`, `{"move": [dx, dy], "steps": n, "ms": 16}`, `{"end": true, "after": ms}` (`after` holds the fingers still before they lift) |
| `{"touch": {"fingers": n, "from": target, "by": [dx, dy], "steps": n, "ms": 16, "spread": 60}}` | A touchscreen drag: the fingers land one by one in a row, move together and lift together |
| `{"key": "Escape"}`, `{"key": "Ctrl+Alt+Right"}` | Key press and release. Modifier keys go down first and up last, as on a keyboard, which Prism needs. Wayland clients receive evdev scan codes |
| `{"gamepad": {"connect": family \| "tap" \| "press" \| "release": button \| "stick": [x, y], "ms": n \| "disconnect": true}}` | A virtual game controller ([CONTROLLERS.md](CONTROLLERS.md)) |
| `{"call": "setSurface", "args": [...]}` | Call a `Desktop` method (`launch`, `setSurface`, `switchSpace`, ...) |
| `{"restore": spec}` | What clicking the window's app on the Shelf does |
| `{"close": spec}` | Ask a window to close (`xdg_toplevel.close` for clients) |
| `{"expect": {"path": ..., "equals": value}}` | Assert on the state below; `"window": spec` scopes the path to a window; in an array a path step is an index or `length` (`windows.length` counts closing windows too); `"near": [..] \| number \| "statePath"` with `"scale"` and `"tolerance"` compares rectangles and numbers; `"contains": "text"` checks a string holds another |
| `{"state": file}`, `{"screenshot": file}` | Write the state as JSON or grab the window (paths relative to the script); `"display": n` grabs another display under Prism |
| `{"log": text}` | Print a progress line |

A click or drag aimed at a window part fails if another window covers that
point, so a script cannot silently hit the wrong window.

Gesture and touch steps run on a virtual clock that advances by `ms` per
step, so the velocity a swipe ends with is the scripted one, however busy the
machine is.

While a script runs, only its own input and the controller interface's
reach the window (`qor::input::InjectedInput`). Real pointer and key input
is dropped: a person's mouse crossing the window, or a host compositor's
pointer over a nested Prism, would otherwise cancel scripted drags or fire
shortcuts. Running tests on a machine in use is safe.

A **target** is `[x, y]` in window coordinates, or
`{"window": spec, "part": "frame|titlebar|content|close|maximize|minimize", "at": [x, y]}`,
or `{"item": "<objectName>", "at": [x, y]}`, or `{"display": n, "at": [x, y]}`
for a point on another display (Prism with `--outputs`; windows and items are
found on whichever display shows them). `at` is relative to the part's
top-left corner; negative values count from the right and bottom edges, and
without `at` the target is the centre. A window **spec** is a string matched
against title or app id, or `{"title": ..., "appId": ...}`.

The **state** includes:

- `settings`, including the audio-reactive and controller preferences;
- `controller`: virtual and real controllers connected, whether the connect
  prompt and on-screen keyboard are open, and the controller cursor;
- `audio` (volume, mute, output, equaliser, interface sounds, `active`) and
  `media` (playing, title, position, duration, queue index);
- `workArea`, `overview`, `currentSpace` and `surface`; `spaceOffset` (where
  the strip is, in Spaces) and `overviewProgress`; `displays` (under Prism,
  the number of outputs);
- `gesture`: the system gesture under way (`active`, `axis`, `source`,
  `fingers`, `translation`);
- `store`: whether the catalogue is `ready`, its number of `items`, and what
  is `installed` (`"id@version,…"`); `editingWidgets`, and `desktopWidgets`
  (`"id@col,row;…"`);
- `frames`: frames the shell has drawn (a client inside Prism gets frame
  callbacks only when Prism draws);
- open `popups`, as screen rectangles;
- `windows[]`, with title, app id, focus, maximised, minimised, frame and
  on-screen rectangles, Space, `display`, `visible`, `commits` (client
  commits) and `overviewMix` (how far into its Overview place).
  Wayland clients also report their window geometry, configured size, Shelf
  identity and surface item (with `onOutput`);
- `products`: each product Prism hosts, by id, `available` and `running`.
- `library`: the ids of the Library's `games`, how many `media` it holds,
  the id of what was played or opened last (`recent`), whether it is
  `scanning`, and the `page` Home shows.

Two suites use the runner (label `gpu`), and a third needs no display:

- **`shell_e2e`** runs `tests/shell/apps.e2e.json` in `qor-shell` on every
  platform. A Lumen `Switch` receives a click, and maximise, snap, minimise,
  Shelf restore, Overview and close work for a first-party app.
- **`prism_e2e`** (Linux) runs `tools/dev/prism-e2e.py`. It starts a nested
  Prism with `prism-test-client` (a Qt client with a menu at a known spot,
  an app id, a desktop entry and size limits) and `weston-terminal`, which
  draws its own decorations. It then checks 31 behaviours:
  - decorations and app identity;
  - maximise, restore and snap;
  - resizing from any edge, and client-initiated moves and resizes;
  - minimise and Shelf restore;
  - size limits;
  - xdg_popup placement and dismissal;
  - focus-stealing prevention (xdg-activation);
  - Overview and close.

  Run it with `--keep` to keep the states and screenshots.
- **`prism_products`** (Linux) runs `tests/prism/products.e2e.json` through
  `tools/dev/prism-products.py`. Prism runs on the offscreen platform, draws
  in software in a runtime folder of its own, and shows nothing, so the suite
  needs neither a display nor a graphics card. Stand-ins for ARQADE and QOR
  Media (`prism-test-arqade`, `prism-test-media`) speak the products' side of
  the host protocol; those for QQ Studio and the QQ Player
  (`prism-test-qq-studio`, `prism-test-qq-player`) read their arguments as
  those do and show at a size of their own. Prism starts them and the suite
  checks:
  - each window's place and size, and that it is named from Prism's record;
  - that a window opens at the size its client asks for, fitted to the work
    area when it asks for more;
  - that the Motion setting reaches ARQADE while it runs;
  - that opening ARQADE again brings back its window;
  - that the QQ Player plays one game at a time;
  - that closing a window ends its product.

### Run a product inside Prism

Prism starts the products itself and lays out their windows
([ARCHITECTURE.md](ARCHITECTURE.md#hosted-by-prism)). It finds each where
`QOR_ARQADE`, `QOR_MEDIA`, `QOR_QQ_STUDIO` or `QOR_QQ_PLAYER` points, else
beside its own program. On Linux, after `tools/dev/products.sh` has built
them:

```bash
export QOR_ARQADE=$PWD/build/products-linux/arqade/arqade
export QOR_MEDIA=$PWD/build/products-linux/media/qor-media
export QOR_QQ_STUDIO=$PWD/build/products-linux/qq/studio/qq-studio
export QOR_QQ_PLAYER=$PWD/build/products-linux/qq/player/qq-player
export QOR_GAMES=$PWD/products/qq/samples   # the QQ games Home and the Library offer
./build/linux-release/prism/compositor/prism --unlocked
```

ARQADE and QQ Studio are then in Beam. Home and the Library (Guide, or Beam's
Home and Library) offer every game and the person's media: QQ games from
`QOR_GAMES`, else `share/qor-os/games` beside Prism and in the data folder;
the person's own QQ projects in Documents; ARQADE's six; and video, music and
pictures. A game plays in the QQ Player (`Desktop.playQQGame(scene)`) or in
ARQADE (`Desktop.playInArqade(game)`), and QOR Media opens the files
(`Desktop.openInMedia(path, kind)`), where they stopped when the Library
knows.

### Add an icon

Add the Lucide name to `kit/icons/icons.txt`, then run
`python tools/icons/import_lucide.py <lucide-static package dir>` (download
with `npm pack lucide-static`). CMake picks the list up automatically.

### Add a Kit component

Create `kit/MyComponent.qml` (SPDX header, `import QOR.Kit`), add it to
`QML_FILES` in `kit/CMakeLists.txt`, use tokens and Theme only, and add a
Quick Test if it has logic.

### Add a Lumen control

Create `kit/style/<Control>.qml` deriving from `QtQuick.Templates`
(`T.<Control>`), mirroring the structure of Qt's Basic style
(`C:\Qt\6.12.0\msvc2022_64\qml\QtQuick\Controls\Basic`). Add it to
`kit/style/CMakeLists.txt`. Unimplemented controls fall back to Basic.

### Add or change a system service

1. Edit or add `services/interfaces/<name>.rep`. Use `READWRITE` for
   user-settable properties and `SOURCEONLYSETTER` for service-owned state.
   Slots return `void` unless a reply is essential.
2. New file: add it to `QOR_SERVICE_INTERFACES` in `services/CMakeLists.txt`.
3. Implement the simulated source in `sim/src/` (subclass `<Name>SimpleSource`),
   load initial state from the scenario, and `enableRemoting` it in
   `SimServices::start`.
4. Expose the replica: add a member and `Q_PROPERTY` to `qor::Services`,
   acquire it in `connectTo`, and add a `<Name>ServiceForeign` declaration.
5. Extend `tests/services/tst_services.cpp` and [SERVICES.md](SERVICES.md).
6. Optionally add `qorctl` commands.

### Add an app

1. Create `apps/MyApp.qml`: root `Item` with `property real topInset` and
   `property var launchArgs`. Lay out below `topInset` (the title bar overlays it).
2. Add it to `apps/CMakeLists.txt` and to `AppRegistry` (`apps`, `pinned`, a
   `Component` property and `component()`).
3. Use `FrostSidebar` for sidebars, Lumen controls for inputs, `Services` for
   system state, and `Intents` for system actions.
4. Add a capture scene in `tools/screenshots/capture.ps1` if it is a showcase.

### Add a shell action (Beam, Terminal, qorctl)

Add a `case` in `Desktop.runAction()`, an entry in `Beam.catalog()`, and if it
is useful from outside, a `qorctl` command or an `Intents` action.

### Change a product

Make the change here. A file from demiurge-chain (listed in
`LICENSES/from-demiurge-chain.txt`) gets a `Changed for QOR OS:` comment near
its top (Apache-2.0 §4(b)); a new file gets QOR OS's SPDX header. Keep the
product's own formatting, and run `tools/dev/products.ps1` and
`tools/dev/products.sh`, since a change for one platform must keep the other
as it was. A change to ARQADE's rules is also made upstream, with its fixture
([UPSTREAM.md](UPSTREAM.md#changing-a-games-rules)).

### Pull ARQADE's fixtures and the one look from upstream

```bash
tools/upstream/demiurge.sh status          # last upstream commits, whether ARQADE's rules match, the core's pin
tools/upstream/demiurge.sh pull            # merge demiurge-chain main's fixtures and copy its tokens.json
pwsh tools/dev/products.ps1                # build and test the products against them
tools/dev/products.sh                      # and on Linux
```

Update the state tables in [UPSTREAM.md](UPSTREAM.md) in the same change.

### Use the QOR core

Link `qor::core` and call the bridge (`qor_core.h`): one JSON request in, one
JSON answer out, freed with `qor_core_free`. Amounts are decimal strings of
embers. `tests/core/tst_qorcore.cpp` shows the pattern. A new command is added
upstream in demiurge-chain with its tests, then the pin moves
([UPSTREAM.md](UPSTREAM.md#moving-the-cores-pin)). To work on both at once,
configure with `-DQOR_CORE_SOURCE_DIR=<a demiurge-chain checkout>`.

### Add a decision

Copy [`decisions/TEMPLATE.md`](decisions/TEMPLATE.md) to the next
`QOR-NNNN-<title>.md`, index it in [`decisions/README.md`](decisions/README.md),
and mark any record it supersedes or amends.

## 6. Debugging

| Need | How |
|------|-----|
| QOR OS logs | `QT_FORCE_STDERR_LOGGING=1`, `QT_LOGGING_RULES="qor.*=true"` |
| QML warnings with file and line | Same as above. Warnings print on stderr |
| Graphics backend, adapter, pipeline cache | `QSG_INFO=1` |
| Overdraw and batching | `QSG_VISUALIZE=overdraw` / `batches` / `clip` / `changes` |
| Render timing per frame | `QSG_RENDER_TIMING=1` |
| QML profiler / debugger | Launch config "QML debugger" in `.vscode/launch.json` (`-qmljsdebugger=port:3768,block`), then attach from Qt Creator |
| GPU frame capture | RenderDoc or PIX on `qor-shell.exe` (D3D11) |
| Service traffic | `qorctl watch`; `QT_LOGGING_RULES="qt.remoteobjects*=true"` |
| Frame pacing | `qor-shell --benchmark 8 --benchmark-out bench.json` |

## 7. Tooling reference

| Tool | Purpose |
|------|---------|
| `tools/dev/Enter-QorShell.ps1` | Load MSVC + Qt + CMake + Ninja into PowerShell |
| `tools/dev/build.ps1` | Configure, build, optionally test or build a target |
| `tools/dev/run.ps1` | Run `qor-shell` with arguments |
| `tools/screenshots/capture.ps1` | Deterministic doc screenshots (`docs/images/*.webp`) |
| `tools/tokens/generate_tokens.py` | Lumen token generator (`--check` in CI) |
| `tools/icons/import_lucide.py` | Reproducible icon import |
| `tools/ci/check_headers.py` | SPDX header check |
| `tools/ci/install_qt.py` | Unattended commercial Qt install for CI |
| `tools/github/bootstrap.py` | Apply labels, milestones, repository settings and ruleset |
| `tools/dev/products.ps1` | Build and test QQ, ARQADE and QOR Media with their own scripts |
| `tools/dev/products.sh` | Build and test QQ, ARQADE and QOR Media on Linux, on a display nobody sees and, under WSL, fenced from the GPU ([BUILDING.md](BUILDING.md#on-linux)) |
| `tools/dev/products-display.qml` | That display: a compositor on Qt's offscreen platform, for the products' suites |
| `tools/dev/secret-service.py` | That run's keyring: an in-memory Secret Service on the run's own session bus ([QOR-0011](decisions/QOR-0011-the-creators-provider-key-in-the-secret-service.md)) |
| `tools/dev/prism-products.py` | Prism hosting stand-ins for ARQADE, QOR Media, QQ Studio and the QQ Player, offscreen and in software (`prism_products`) |
| `tools/upstream/demiurge.sh` | Pull ARQADE's fixtures and the one look from demiurge-chain, check ARQADE's rules match it, write patches for upstream |
| `tools/ci/runners.ps1` | Move the workstation's two CI runners to the organisation or a repository, and start them |
| `qorctl` | Drive services from the command line ([SERVICES.md](SERVICES.md#qorctl)) |
