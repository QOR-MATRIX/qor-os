# ADR-0009: OKLCH tokens as the single source of the design system

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

Lumen needs light and dark appearances, a user-chosen accent, smooth
transitions between them, and identical colors in QML, Qt Widgets and the
documentation. sRGB hex palettes interpolate poorly (muddy midpoints) and
cannot rotate an accent perceptually.

## Decision

- Define every color role in **OKLCH** for light and dark, with optional
  `followAccent`, in `kit/tokens/lumen.tokens.json`, together with radius,
  spacing, typography, elevation, motion and material tokens.
- Generate `kit/Tokens.qml`, `platform/common/lumentokens.h` and
  `docs/design/tokens.md` with `tools/tokens/generate_tokens.py`. Commit the
  outputs and enforce freshness in pre-commit and CI.
- Resolve colors at runtime in OKLab (`Oklch.themed`), driven by springs, with
  chroma-reducing gamut mapping.

## Consequences

- Appearance and accent changes animate system-wide with no per-component code.
- The same math exists in C++ (`kit/src/oklchmath.cpp`), GLSL (wallpaper) and
  Python (docs). The tests cover the C++ and Python implementations against
  reference values.
