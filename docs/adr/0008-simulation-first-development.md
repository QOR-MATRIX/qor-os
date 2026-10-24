# ADR-0008: Simulation-first development on Windows

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

The primary workstation runs Windows 11 with a commercial Qt install. A
Wayland compositor and the real service daemons only run on Linux, but most
shell and design work does not depend on them.

## Decision

Develop the shell in **simulation**:

- `qtos-shell` hosts the same QML as `prism`, with in-process windows for
  first-party apps.
- `qtos-sim` implements every service contract with scenario-driven
  behaviour. It runs embedded or as `qtos-simd`.
- `qtosctl`, screenshots and the benchmark make the simulator scriptable.

Then validate on Linux: nested `prism` (Phase 1), then the image (Phase 2).

## Consequences

- Fast iteration with native Windows tooling (MSVC, Qt Creator, VS Code,
  RenderDoc).
- Some behaviour (Wayland client sizing, input via libinput, KMS) is only
  testable on Linux and is tracked in Phase 1 and 2.
