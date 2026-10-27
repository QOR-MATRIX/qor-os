# QOR-0008: Before a public release; the code keeps its QTOS names until the name is cleared

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** [QTOS-0004](qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md), QOR-0006; Demiurge
  [ADR-034](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-034-ticker-dmrg.md)

## Context

Development can go ahead now. Publishing an operating system image for
anyone to download raises questions that only the owner can settle, with
outside help:

- **Qt licensing.** QTOS-0004 settled development on the commercial Qt kit.
  Distributing Qt inside a free-to-download OS image and an SDK is another
  matter. Several modules QOR OS uses are only commercial or GPLv3, including
  Qt Wayland Compositor, Qt Quick 3D, Qt Quick 3D Physics and Qt Graphs. The
  distribution terms come from the agreement with The Qt Company.
- **The name.** Demiurge's release gates require a trademark clearance
  opinion on its own name, because its screening (ADR-034) found DEMIURGE
  live in classes 9, 41 and 42. "QOR OS", QQ and ARQADE have not been
  screened.
- **The rest of Demiurge's list:**
  - Terms of Service and a privacy policy;
  - a legal opinion on rewards, prizes, identity checks, sanctions and age
    limits;
  - code-signing certificates;
  - an external security review.

  Demiurge's SECURITY.md also treats a recovery phrase pasted into a chat on
  5 October 2026 as exposed. Its key must be replaced before anything holds
  value.
- **Sign-in.** QOR ID, Social and the devnet run on one hosting provider in
  one region. An operating system that signs people in through QOR ID needs
  redundancy and offline login first.

Renaming the code now would touch every module, program and option, and the
name might still have to change after clearance.

## Decision

1. **No public image, SDK or download until:**
   - The Qt Company has confirmed in writing the licence for distributing Qt
     in a QOR OS image and SDK;
   - a qualified attorney has cleared "QOR OS" (and QQ and ARQADE) as names;
   - Demiurge's legal list above is complete;
   - code-signing and Secure Boot keys exist (QOR-0004);
   - QOR ID has a second region and offline login, and the 5 October key has
     been replaced.

   [ROADMAP.md](../ROADMAP.md) tracks these as release gates.
2. **The code keeps its QTOS names until the name is cleared.** That covers
   the `QTOS` CMake project and `QTOS_*` options, the `QTOS.*` QML modules,
   the `qtos::` namespace, and programs such as `qtos-shell`, `qtosctl` and
   `qtos-simd`. The product people see is called QOR OS. One later record
   renames the code, once and completely.

## Consequences

- Private builds, CI and testing on the owner's machines carry on as before
  under QTOS-0004.
- Documentation calls the product QOR OS, and calls a program by its current
  name.
- If clearance rejects "QOR OS", only documentation and user-facing strings
  change, which is why the code rename waits.
