# ADR-0005: Qt 6.12 as the baseline

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

The development machine has Qt 6.8.x (LTS), 6.9, 6.10, 6.11 and 6.12.0
installed. QTOS is a forward-looking product that benefits from the newest
graphics and QML tooling, for example HDR swapchains in QRhi, variable font
axes in QML, `Text.CurveRendering`, stable Qt Graphs, and Qt Canvas Painter
and Qt Labs StyleKit for evaluation.

## Decision

Target **Qt 6.12** (`qt_standard_project_setup(REQUIRES 6.12)`, pinned
6.12.0 in `cmake/QtosLocateQt.cmake` and `.github/qt-packages.json`). Move
to the next Qt LTS when it is released and validated with the benchmark and
the test suites.

## Consequences

- Newest APIs are available and nothing is held back for older Qt versions.
- Upgrades are deliberate: bump the pinned version, regenerate the CI cache
  key (automatic via the package manifest hash), and re-run the benchmark.
