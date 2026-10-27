# ADR-0006: Service IPC with Qt Remote Objects

- **Status:** Accepted
- **Date:** 2026-10-23

## Context

The shell and apps need typed, observable access to system state (network,
power, audio, settings…) across process boundaries. They must work against
simulated services on any desktop and real daemons on a device.

Considered: D-Bus everywhere (`Qt DBus`), Qt gRPC, Qt Remote Objects, Qt
Interface Framework (which can generate QtRO backends).

## Decision

- **Qt Remote Objects** for all QTOS-internal services. Contracts are `.rep`
  files in `services/interfaces/`. Properties replicate, so QML binds to them
  directly. Slots and signals cross the boundary.
- **D-Bus** only at freedesktop edges: notifications, MPRIS, portals and
  Secret Service.
- **Qt gRPC** only for `qtos-assistd` and companion devices.
- Replicas are generated so AUTOMOC emits metatypes
  (`cmake/QtosRepc.cmake`) and are registered with `QML_FOREIGN`, giving
  `qmllint` and `qmlsc` full type information.
- Re-evaluate Qt Interface Framework in Phase 1 as a generator for the same
  contracts plus richer simulation backends.

## Consequences

- One contract serves simulator, device daemons, `qtosctl` and tests.
- `.rep` changes are API changes and must be called out in reviews.
- QtRO signatures must match between source and replica builds, so both are
  always generated from the same files.
