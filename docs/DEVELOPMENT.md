# Development guide

Conventions, recipes and debugging techniques for working on QTOS. Read
[ARCHITECTURE.md](ARCHITECTURE.md) first.

## 1. Daily loop

```powershell
. .\tools\dev\Enter-QtosShell.ps1
cmake --build --preset windows-release            # incremental build (QML is compiled too)
.\tools\dev\run.ps1 -- --unlocked --open files    # run
ctest --preset windows-release                    # test
cmake --build --preset windows-release --target all_qmllint
```

For visual work, `tools\screenshots\capture.ps1 -Scenes desktop,beam -OutDir $env:TEMP\shots`
produces deterministic frames you can compare before and after a change.

## 2. C++ conventions

- C++20, Qt 6.12. Namespaces: `qtos` (Kit, Services), `qtos::sim`,
  `qtos::lumen`, `qtos::host`, `qtos::color`.
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
- Every file starts with `// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary`.

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
5. Measure with `qtos-shell --benchmark 8` before and after (see
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
`QTOS_GOLDEN_UPDATE=platform`. That writes an override to
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

`qtos-shell --script file.json` and `prism --script file.json` drive the
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
| `{"click": target}`, `{"doubleClick": target}`, `{"move": target}` | Pointer input |
| `{"drag": {"from": target, "to": target \| "by": [dx, dy], "steps": n}}` | Press, move in steps, release |
| `{"key": "Escape"}` | Key press and release (Wayland clients receive evdev scan codes) |
| `{"call": "setSurface", "args": [...]}` | Call a `Desktop` method (`launch`, `setSurface`, `switchSpace`, ...) |
| `{"restore": spec}` | What clicking the window's app on the Shelf does |
| `{"close": spec}` | Ask a window to close (`xdg_toplevel.close` for clients) |
| `{"expect": {"path": ..., "equals": value}}` | Assert on the state below; `"window": spec` scopes the path to a window; `"near": [..] \| "statePath"` with `"scale"` and `"tolerance"` compares rectangles |
| `{"state": file}`, `{"screenshot": file}` | Write the state as JSON or grab the window (paths relative to the script) |
| `{"log": text}` | Print a progress line |

A click or drag aimed at a window part fails if another window covers that
point, so a script cannot silently hit the wrong window.

A **target** is `[x, y]` in window coordinates, or
`{"window": spec, "part": "frame|titlebar|content|close|maximize|minimize", "at": [x, y]}`,
or `{"item": "<objectName>", "at": [x, y]}`. `at` is relative to the part's
top-left corner; negative values count from the right and bottom edges, and
without `at` the target is the centre. A window **spec** is a string matched
against title or app id, or `{"title": ..., "appId": ...}`.

The **state** includes:

- `settings`;
- `workArea`, `overview`, `currentSpace` and `surface`;
- open `popups`, as screen rectangles;
- `windows[]`, with title, app id, focus, maximised, minimised, frame and
  on-screen rectangles. Wayland clients also report their window geometry,
  configured size, Shelf identity and surface item.

Two suites use the runner (label `gpu`):

- **`shell_e2e`** runs `tests/shell/apps.e2e.json` in `qtos-shell` on every
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

### Add an icon

Add the Lucide name to `kit/icons/icons.txt`, then run
`python tools/icons/import_lucide.py <lucide-static package dir>` (download
with `npm pack lucide-static`). CMake picks the list up automatically.

### Add a Kit component

Create `kit/MyComponent.qml` (SPDX header, `import QTOS.Kit`), add it to
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
2. New file: add it to `QTOS_SERVICE_INTERFACES` in `services/CMakeLists.txt`.
3. Implement the simulated source in `sim/src/` (subclass `<Name>SimpleSource`),
   load initial state from the scenario, and `enableRemoting` it in
   `SimServices::start`.
4. Expose the replica: add a member and `Q_PROPERTY` to `qtos::Services`,
   acquire it in `connectTo`, and add a `<Name>ServiceForeign` declaration.
5. Extend `tests/services/tst_services.cpp` and [SERVICES.md](SERVICES.md).
6. Optionally add `qtosctl` commands.

### Add an app

1. Create `apps/MyApp.qml`: root `Item` with `property real topInset` and
   `property var launchArgs`. Lay out below `topInset` (the title bar overlays it).
2. Add it to `apps/CMakeLists.txt` and to `AppRegistry` (`apps`, `pinned`, a
   `Component` property and `component()`).
3. Use `FrostSidebar` for sidebars, Lumen controls for inputs, `Services` for
   system state, and `Intents` for system actions.
4. Add a capture scene in `tools/screenshots/capture.ps1` if it is a showcase.

### Add a shell action (Beam, Terminal, qtosctl)

Add a `case` in `Desktop.runAction()`, an entry in `Beam.catalog()`, and if it
is useful from outside, a `qtosctl` command or an `Intents` action.

## 6. Debugging

| Need | How |
|------|-----|
| QTOS logs | `QT_FORCE_STDERR_LOGGING=1`, `QT_LOGGING_RULES="qtos.*=true"` |
| QML warnings with file and line | Same as above. Warnings print on stderr |
| Graphics backend, adapter, pipeline cache | `QSG_INFO=1` |
| Overdraw and batching | `QSG_VISUALIZE=overdraw` / `batches` / `clip` / `changes` |
| Render timing per frame | `QSG_RENDER_TIMING=1` |
| QML profiler / debugger | Launch config "QML debugger" in `.vscode/launch.json` (`-qmljsdebugger=port:3768,block`), then attach from Qt Creator |
| GPU frame capture | RenderDoc or PIX on `qtos-shell.exe` (D3D11) |
| Service traffic | `qtosctl watch`; `QT_LOGGING_RULES="qt.remoteobjects*=true"` |
| Frame pacing | `qtos-shell --benchmark 8 --benchmark-out bench.json` |

## 7. Tooling reference

| Tool | Purpose |
|------|---------|
| `tools/dev/Enter-QtosShell.ps1` | Load MSVC + Qt + CMake + Ninja into PowerShell |
| `tools/dev/build.ps1` | Configure, build, optionally test or build a target |
| `tools/dev/run.ps1` | Run `qtos-shell` with arguments |
| `tools/screenshots/capture.ps1` | Deterministic doc screenshots (`docs/images/*.webp`) |
| `tools/tokens/generate_tokens.py` | Lumen token generator (`--check` in CI) |
| `tools/icons/import_lucide.py` | Reproducible icon import |
| `tools/ci/check_headers.py` | SPDX header check |
| `tools/ci/install_qt.py` | Unattended commercial Qt install for CI |
| `tools/github/bootstrap.py` | Apply labels, milestones, repository settings and ruleset |
| `qtosctl` | Drive services from the command line ([SERVICES.md](SERVICES.md#qtosctl)) |
