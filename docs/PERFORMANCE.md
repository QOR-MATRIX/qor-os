# Performance

QTOS treats frame pacing as a feature. This document records the budgets,
the measured baseline, how to measure, and the techniques that keep the
shell fast.

## Budgets

From [VISION.md §12](../VISION.md#12-performance-budgets-targets-on-the-reference-laptop).
Phase 0 measures what the simulator can measure: frame pacing and idle
behaviour.

| Metric | Target | Phase 0 status |
|--------|--------|----------------|
| Compositor frame, shell animating | GPU ≤ 3 ms, CPU ≤ 2 ms | **2.79 ms average frame interval at 360 Hz** on the RTX 4060 (the frame interval includes vsync wait, so render cost is lower) |
| Idle wakeups with nothing animating | ~0 | Met by design: springs stop on settle, clock wakes once a minute, no continuous animations |
| First-party app launch → first frame | ≤ 300 ms | Apps are in-process QML in simulation; measured in Phase 3 with Qt Application Manager |
| Beam keystroke → results | ≤ 16 ms | Search is synchronous JS over about 60 entries; not yet instrumented |
| Pointer input → photon | ≤ 1 frame + scanout | Input to swap: 5.9 ms p50 on the RTX 4060 at 360 Hz (about 2.1 frames, measured through the Windows compositor); 9.2 ms on Intel UHD. Measured on device in Phase 2 |
| Boot / login budgets | ≤ 5 s / ≤ 700 ms | Phase 2 (image) |

## Baseline (2026-10-23)

`qtos-shell --open files,music,notes --benchmark 8`: an 8-second tour that
cycles Overview → Beam → Control Center → Space 2 → Space 1 every 750 ms.
Window 1539×844 logical at 125% scaling (1924×1055 physical), Direct3D 11,
`windows-release`.

| GPU | Average | p50 | p95 | p99 | Max |
|-----|---------|-----|-----|-----|-----|
| NVIDIA GeForce RTX 4060 Laptop (360 Hz panel) | **2.79 ms · 358 fps** | 2.77 ms | 3.07 ms | 3.57 ms | 15.6 ms |
| Intel UHD Graphics (integrated) | 7.30 ms · 137 fps | 8.20 ms | 9.11 ms | 11.36 ms | 16.5 ms |

The RTX 4060 holds the panel's 360 Hz refresh through the whole tour. The
single max outlier is first-use pipeline creation when Beam opens.

Re-measured on 2026-10-24 with the shared benchmark driver:

- **RTX 4060:** 2.79 ms average, p95 3.06 ms, with 0.3% of frames missing a
  360 Hz refresh.
- **Intel UHD:** bimodal between runs. Two runs in five reproduce the
  baseline (avg 7.2 ms, p95 9.1 ms); the others sit at p95 about 11.1 ms,
  which looks like a GPU power state. Compare the better of several runs.

### Input-to-swap latency (2026-10-24)

`inputToSwap` times a pointer move over the Shelf. The clock starts when the
event is posted and stops at the swap of the first frame synchronised after
the event was handled. Display scan-out and input-device latency are not
included.

| GPU | p50 | p95 | Max |
|-----|-----|-----|-----|
| NVIDIA GeForce RTX 4060 Laptop (360 Hz) | **5.9 ms** | 6.3 ms | 6.5 ms |
| Intel UHD Graphics (360 Hz panel) | 9.2 ms | 18 ms | 54 ms |

### Prism nested under WSLg (2026-10-24, indicative)

`prism --benchmark 8` on the same laptop, inside WSLg. That means Ubuntu
26.04, OpenGL on Mesa d3d12, a 60 Hz output paced by WSLg's Weston, and a
1600×1000 window. These numbers describe the nested development setup, not
the device.

| Scenario | Average | p95 | Missed frames | Input to swap p50 / p95 |
|----------|---------|-----|---------------|-------------------------|
| Shell tour | 17.1 ms (60 Hz) | 17.8 ms | 0.2% | 26.4 / 33.9 ms |
| Shell tour + `weston-terminal` + Qt Widgets client | 18.2 ms | 21.8 ms | 1.1% | 26.8 / 34.8 ms |

### Optimisation history

| Change | Intel UHD p95 | Intel UHD avg |
|--------|---------------|---------------|
| Initial shell | 31.3 ms | 14.4 ms |
| Wallpaper zoom and parallax as transforms of a cached layer (not shader uniforms) | **9.1 ms** | **7.3 ms** |

## How to measure

```powershell
.\tools\dev\run.ps1 -- --open files,music,notes --benchmark 8 --benchmark-out bench.json
.\tools\dev\run.ps1 -- --gpu low-power --open files,music,notes --benchmark 8   # integrated GPU
```

```bash
prism --benchmark 8 --benchmark-out prism.json     # Linux: every output of the compositor
```

The tour runs for the given number of seconds; then the driver takes 40
latency samples over about 6 seconds. The driver is `prism/shell/shellbenchmark.cpp`, shared by
`qtos-shell` and `prism`.

| Field | Meaning |
|-------|---------|
| `tool` | `qtos-shell` or `prism` |
| `frames`, `averageMs`, `averageFps`, `p50Ms`, `p95Ms`, `p99Ms`, `maxMs` | Frame intervals of the first output during the tour |
| `refreshHz`, `missedFrames`, `missedPercent` | The output's refresh rate, and intervals longer than 1.5 refresh periods |
| `graphicsApi`, `size`, `devicePixelRatio`, `screen`, `seconds` | Context |
| `outputs[]` | The same frame fields for every output (one window per `WaylandOutput` under Prism) |
| `inputToSwap` | `samples`, `posted`, `averageMs`, `p50Ms`, `p95Ms`, `p99Ms`, `maxMs` (see above) |

Intervals come from `QQuickWindow::frameSwapped` through `qtos::FrameStats`,
which never requests frames itself. While the tour is measured, the benchmark
keeps every output drawing (each swap asks for the next), so an output with
nothing animating is measured at its refresh rate rather than by its idle
gaps; the latency phase runs on a still scene. Gaps over 200 ms count as idle
and are excluded. Keeping outputs busy left the single-output numbers above
unchanged (2.78 ms average, 3.05 ms p95 and 5.9 ms input to swap on the
RTX 4060).

### Compositor decision gate

[ADR-0003](adr/0003-compositor-strategy.md) keeps Qt Wayland Compositor
unless the bare-metal path misses multi-output frame-pacing or latency
targets. The gate's data is `prism --benchmark` on the reference hardware:

- DRM/KMS (`QT_QPA_PLATFORM=eglfs` with the KMS integration, later Prism's
  own backend);
- two outputs at different refresh rates;
- Wayland clients running.

Proposed pass criteria, to be ratified with the measurement in ADR-0011:

- under 1% missed frames on each output at its own refresh rate;
- input to swap within two refresh periods at p95.

The product target stays at one frame plus scan-out (VISION §12). The nested
numbers above validate the tooling only.

#### Running the decision gate

Prism drives several outputs (`--outputs` nested, every screen on eglfs), and
`tools/dev/prism-gate.sh` runs the whole gate in one command. The machine
under test needs neither a Qt kit nor a build: `tools/dev/prism-gate-bundle.sh`
packs Prism, the test client, the Qt runtime they use and the gate script
into one archive (about 100 MB) that runs on a stock Ubuntu desktop, a live
USB session included. Build the bundle on the Ubuntu release it targets,
against that release's ISO manifest:

```bash
tools/dev/prism-gate-bundle.sh --build-dir ~/qtos-build/linux-release     --manifest ubuntu-26.04.1-desktop-amd64.manifest --out qtos-gate.tar.gz
```

On the machine under test, booted into Ubuntu (a live USB session is enough)
with two displays at different refresh rates on one GPU:

1. Unpack the bundle, and join the groups for the displays and input:
   `sudo usermod -aG video,render,input $USER`. A live session's user needs
   a password for the console: `sudo passwd $USER`.
2. `qtos-gate/prism-gate.sh --probe` lists the GPUs and the displays on
   each. Prism drives one GPU, so both displays must be on it; on a laptop,
   the panel is usually on the integrated GPU and some video ports are wired
   to the discrete one.
3. `qtos-gate/prism-gate.sh --try` runs a trial inside the desktop session,
   as two windows: it checks the bundle on this machine (its numbers are not
   gate data).
4. Switch to a text console (<kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>F3</kbd>),
   log in, and run `qtos-gate/prism-gate.sh`.

It picks the GPU with both displays (`--device` overrides), starts Prism on
Qt's eglfs KMS integration (atomic modesetting, one output per display, the
built-in panel first), opens an animating Wayland client on each output,
runs the benchmark, and checks the criteria above. It writes `report.json`,
`summary.md` (machine, GPU, outputs and their refresh rates, per-output
results, input to swap, and the verdict) and the logs into a `gate-<time>`
folder, with a `.tgz` of it to upload; a live session forgets them at power
off. It exits 0 when every criterion passes. <kbd>Ctrl</kbd>+<kbd>C</kbd>
stops a run, and a watchdog ends one that hangs.

`--mode eDP-1=1920x1080@60` picks a display's mode, to separate frame pacing
from raw GPU throughput on a very fast panel; `--no-atomic` falls back to
legacy modesetting if a driver rejects atomic commits. With a Qt kit and a
`linux-release` build on the machine, `--build-dir` runs without a bundle.
`summary.md` is the data ADR-0011 records.

The **Monitor** app shows the same probe live: frame time and fps while the
shell animates, and **Idle · Rendering on demand** when nothing changes.

For pull requests that touch the shell or Kit, run the benchmark on the same
machine before and after and paste both results.

## GPU selection

On hybrid-graphics Windows laptops Qt's Direct3D backends use adapter 0,
usually the integrated GPU. `qtos-shell` asks DXGI for the adapter matching
`--gpu` (default `high-performance`) and sets `QT_D3D_ADAPTER_INDEX`
accordingly before the first window is created (`prism/host/gpupreference.cpp`).
An explicit `QT_D3D_ADAPTER_INDEX` in the environment always wins. The chosen
adapter is logged as `GPU: …`.

On Linux, GPU selection belongs to the platform (PRIME offload / `DRI_PRIME`);
on a QTOS device, Prism owns the KMS device.

## Techniques

| Technique | Where | Why |
|-----------|-------|-----|
| Shared blurred backdrops | `kit/Backdrop.qml`, `kit/Glass.qml` | One downsampled (¼) blur per source per change, shared by every glass panel |
| Exact backdrop mapping without wakeups | `kit/src/scenerect.cpp` | Re-maps on `afterAnimating` (only when frames happen) |
| SDF primitives | `kit/shaders/surface.frag`, `glass.frag`, `roundmask.frag` | One pass for shape, AA, hairline, sheen and shadow; no textures |
| Cached wallpaper | `kit/Wallpaper.qml` | Noise field evaluated only on hue, appearance or time change; zoom and parallax are transforms |
| Closed-form springs that stop | `kit/src/spring.cpp` | Exact at any frame rate; zero cost at rest |
| Mipmapped window layers only while scaled | `prism/shell/AppWindow.qml` | Crisp Overview thumbnails without permanent mip generation |
| Native text rendering | `prism/host/main.cpp` | Crisp static text; layers keep animated text smooth |
| AOT-compiled QML | `qt_add_qml_module` + `qmlsc` | No QML parsing at startup; typed bindings run as C++ |
| Pipeline cache | Qt Quick | Repeat runs avoid shader pipeline creation |

## Known costs and next steps

- On integrated GPUs the remaining p95 is dominated by re-capturing the scene
  backdrop while windows move. Planned work: capture only the region under
  visible glass, and skip the capture when no glass overlaps changed content.
- `Reduce Transparency` removes the glass sampling cost entirely.
- Phase 2 adds direct scanout for full-screen clients and overlay planes for
  video and cursor in the KMS backend.
