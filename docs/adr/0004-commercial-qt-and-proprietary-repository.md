# ADR-0004: Commercial Qt (Enterprise) and a proprietary repository

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

Several modules QTOS depends on are GPLv3-or-commercial rather than LGPL,
including Qt Wayland Compositor, Qt Application Manager, Qt Virtual Keyboard,
Qt Quick 3D and Qt Graphs. QOR-MATRIX holds a **Qt Enterprise** commercial
license (framework, Qt Creator, Qt Design Studio, Qt for MCUs). It includes
commercial-only tooling such as the Qt Quick Compiler extensions (`qmlsc`).

## Decision

- Develop and ship QTOS on the **commercial Qt** kit. Development, CI and
  release builds use licensed Qt binaries (local installs, self-hosted
  runners, or the official online installer with the organisation's
  account). Consult the Qt license agreement before introducing open-source
  Qt builds anywhere in the pipeline.
- Keep the repository **private** and the code **proprietary**
  (`LicenseRef-QTOS-Proprietary`, see `LICENSE`).
- Third-party assets must be under permissive licenses compatible with
  proprietary distribution (currently OFL fonts and ISC/MIT icons) and are
  listed in `THIRD_PARTY_NOTICES.md`.

## Consequences

- Full access to the Enterprise module set and commercial tooling.
- CI needs credentials or a self-hosted runner ([CI.md](../CI.md)).
- Branch protection on a private repository requires a paid GitHub plan.
- Open-sourcing later would be a new decision, revisiting module licensing.
