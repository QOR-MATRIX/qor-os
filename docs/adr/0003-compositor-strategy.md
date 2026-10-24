# ADR-0003: Compositor strategy — Qt Wayland Compositor with a decision gate

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

QTOS's defining idea is that **the shell is the compositor**: windows are
items in the same Qt Quick scene graph as the shell, so materials, springs
and Overview apply to real clients. The options considered:

| Option | Pros | Cons |
|--------|------|------|
| A. Qt Wayland Compositor + own KMS backend | All Qt, maintained by The Qt Company, stable API | We must build atomic KMS, VRR, HDR metadata, multi-GPU, hotplug, direct scanout |
| B. Waylib/qwlroots (Deepin Treeland) | Mature DRM backend, XWayland | wlroots is below the Qt Line; API churn; smaller upstream |
| C. KWin + shell as clients | Most mature on Qt 6 | Shell and windows are no longer one scene; limits the design |

## Decision

Build on **Option A**. Keep window management backend-agnostic:
`WindowLayer`/`AppWindow` in QTOS.Shell take a pluggable `surfaceDelegate`, so
the same shell runs in the simulator and in `prism`. Hold a **decision gate
at the end of Phase 1**. If the bare-metal path does not meet multi-output
frame-pacing and latency targets, switch to Option B and keep the shell.

X11 apps run in a rootful Xwayland window until a full XWM is justified.

## Consequences

- `prism` (experimental) already hosts the shell with xdg-shell and
  server-side decorations.
- The KMS backend is the hardest single component and is budgeted explicitly
  in Phase 2.
- The decision gate benchmark is a Phase 1 deliverable.

## Status of the gate (2026-10-24)

- **Tooling is ready.** `prism --benchmark` reports frame pacing per output,
  with missed frames judged at each output's own refresh rate, and
  input-to-swap latency. See
  [PERFORMANCE.md](../PERFORMANCE.md#compositor-decision-gate).
- **Nested data exists.** Prism runs nested under WSLg with real Wayland
  clients, at 60 Hz with 0.2–1.1% missed frames.
- **The decision needs bare metal.** That means a DRM/KMS run with two
  outputs at different refresh rates on the reference hardware. It is
  recorded as ADR-0011 together with that measurement. Until then, Option A
  stands.
