# ADR-0007: Lumen as a Qt Quick Controls style on Qt Quick Templates

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

Apps should look native by writing plain `import QtQuick.Controls`. Qt 6.12
ships **Qt Labs StyleKit**, a new declarative styling system, as a Labs
(preview) module. Qt Quick Templates is the long-standing, stable foundation
of all built-in styles.

## Decision

Implement Lumen as a **Qt Quick Controls 2 style module** (`URI Lumen`)
whose controls derive from `QtQuick.Templates`, falling back to the Basic
style for controls not yet implemented. Controls use QTOS.Kit primitives
(`Surface`, `Icon`, `Spring`) and `Theme` roles. Re-evaluate StyleKit when it
leaves Labs.

Classic Qt Widgets apps get Lumen through a `QStyle` (proxy over Fusion) and
the `qtos` QPA platform theme, both fed by generated C++ tokens.

## Consequences

- 23 controls today, with Basic fallback for the rest (for example ComboBox,
  SpinBox, Dialog). Coverage grows per phase.
- Lumen controls can use SDF shapes and springs, which StyleKit may not
  support in the same way.
