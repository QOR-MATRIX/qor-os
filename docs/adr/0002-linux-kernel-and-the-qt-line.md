# ADR-0002: Linux kernel underneath, Qt above "the Qt Line"

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

QTOS aims to be an OS built "only with Qt". Qt is an application and UI
framework, not a kernel. Qt for MCUs targets microcontrollers, not
desktop-class hardware. Hardware support for real devices (GPUs, Wi-Fi,
power management) is a decades-long effort.

## Decision

- QTOS runs on the **Linux kernel** with a small, headless substrate:
  systemd/logind, PipeWire, NetworkManager, BlueZ, UPower, fwupd, Mesa and
  libinput.
- Everything **user-facing** is Qt: compositor, shell, system UI, apps, SDK.
- Substrate daemons are never used directly by UI. They sit behind QTOS
  service contracts implemented in Qt (`qtos-*d`).

## Consequences

- Broad hardware support from day one. The experience is uniformly Qt.
- QTOS must maintain service daemons that wrap substrate APIs (Phase 3).
- The simulator implements the same contracts, so UI development needs no
  Linux device.
