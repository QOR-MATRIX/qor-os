# ADR-0010: Shared-backdrop materials and render on demand

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

Glass is central to Lumen, but naive blur-per-panel costs a full-screen
render and blur for every panel every frame. A premium shell must also be
quiet when idle, for battery life and thermal headroom.

## Decision

- **One blur per source.** `Backdrop` captures a subtree at ¼ resolution and
  blurs it once. Every `Glass` samples that texture at its exact on-screen
  rectangle (`SceneRect`, updated on `afterAnimating`). Chrome samples the
  scene. In-window Frost samples the wallpaper only, which avoids recursion.
- **SDF primitives** for all rounded and shadowed shapes.
- **Render on demand.** Springs stop on settle. No continuous timers or
  animations without visible change. Expensive shaders are cached in layers
  and animated with transforms (the Aurora wallpaper). Frame statistics are
  measured passively (`FrameStats`).
- Performance is verified with `qtos-shell --benchmark`
  ([PERFORMANCE.md](../../PERFORMANCE.md)).

## Consequences

- 358 fps at 360 Hz on a discrete GPU through the benchmark tour; 137 fps
  average on an integrated GPU, with p95 of 9.1 ms.
- Glass cannot show other chrome behind it (it samples content only). This
  is accepted.
- Region-limited capture is the next optimisation for integrated GPUs.
