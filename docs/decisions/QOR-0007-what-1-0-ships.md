# QOR-0007: What QOR OS 1.0 ships

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** QOR-0002 to QOR-0006, [ROADMAP.md](../ROADMAP.md)

## Context

QOR OS is meant to serve five kinds of people: players, game designers, game
developers, app developers and creators. The two projects behind it hold
about ten products, and one founder working with agents cannot finish them
all before a first release.

## Decision

QOR OS 1.0 is a bootable image (QOR-0004) for the reference laptop and for
desktops with an NVIDIA RTX, Intel or AMD GPU. It ships:

| Area | In 1.0 |
|------|--------|
| Shell | Prism with its own KMS backend (QOR-0005), a controller-first Home and Library, Spaces and Overview, Beam, Control Center, the lock screen and the greeter |
| Games | Steam and Proton through gamescope; QQ games; ARQADE with its games and arenas |
| Making games | QQ Studio and the QQ Player |
| Media | QOR Media |
| People | One QOR ID sign-in; Social's chat, friends and World |
| Keys | The vault service, from the QOR core (QOR-0003), with approvals drawn by Prism |
| Store | Free themes, widgets and apps, signed (QOR-0006) |
| System | Settings, Terminal, Monitor, controllers, audio, image updates with rollback |

These wait for a later release: GNOSIS, Stream, paid items in the Store or
the Market, Spatial mode, on-device intelligence, Continuity and the public
SDK.

## Consequences

- [ROADMAP.md](../ROADMAP.md) orders the work in phases toward this list. A
  phase ends when its exit is shown working, not on a date.
- An item moves into or out of 1.0 only by a later record.
