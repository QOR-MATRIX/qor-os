# QOR-0002: One repository, from QTOS and Demiurge's device side

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** [QTOS-0004](qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md) (amended);
  Demiurge [ADR-083](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-083-qq-on-qt.md),
  [ADR-094](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-094-the-library-and-qor-media.md),
  [ADR-102](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-102-arqade-in-the-launcher-on-qt.md),
  [ADR-103](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-103-the-launcher-moves-to-qt-in-stages.md)

## Context

On 26 October 2026 the owner decided to join two of their projects into one
operating system, **QOR OS**:

- **QTOS** (`QOR-MATRIX/QTOS`, private, proprietary): an operating system
  whose user-facing stack is Qt 6.12. It has the Prism compositor and shell,
  the Lumen design system, system services, controller input, audio, a signed
  Store, and CI with the commercial Qt kit.
- **Demiurge** (`QOR-MATRIX/demiurge-chain`, public, Apache-2.0): the QOR
  ecosystem. It has the QQ game engine, ARQADE, QOR Media, the QOR Launcher
  and its Rust core, QOR ID, Social, the Mesh and the Demiurge chain. Its
  ADR-103 already moves every QOR surface to Qt.

GitHub cannot fork a repository into the organisation that owns it. The QOR
Launcher (0.1.17) ships QQ, QOR Media and ARQADE to people today, and they
are developed in demiurge-chain, under its rules and CI.

## Decision

1. **A private repository, `QOR-MATRIX/qor-os`, seeded with QTOS's full
   history.** It is a mirror, not a GitHub fork. QTOS is frozen: its code at
   `37b621e`, the last commit carried here, and a note at `3ecdded` pointing to
   QOR OS. It takes no new work and remains the record of where QOR OS began.
2. **Demiurge's device-side Qt code is carried with its history**, by
   `git subtree`, **at the same paths it has upstream**:

   | Path here and upstream | What it is |
   |------------------------|------------|
   | `products/qq/` | QQ, the QOR Engine: runtime, QQ Studio, QQ Player, the agent's tools, tests |
   | `products/arqade/qt/` | ARQADE's Qt app |
   | `products/arqade/games/` | ARQADE's games, rebuilt in QQ |
   | `products/arqade/tests/fixtures/` | The replays ARQADE's games are checked against |
   | `tools/qor-media/` | QOR Media: player, viewer, editor and PDF reader |
   | `docs/design/tokens.json` | Demiurge's one look ([ADR-108](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-108-one-look-obsidian-and-gilt-in-two-registers.md)), which their tests read; copied, not a subtree |

   Same paths mean that every relative path, build script and comment in the
   carried code stays true, and an upstream pull applies cleanly.
   [`docs/UPSTREAM.md`](../UPSTREAM.md) says how to pull and push.
3. **demiurge-chain stays the upstream of the carried code until a later
   record cuts over.**
   - A fix that is not specific to QOR OS lands upstream first and is pulled
     with `git subtree pull`.
   - Work specific to QOR OS is done here. That covers the Linux build,
     running under Prism, Lumen theming, the controller service, and taking
     out Windows window docking.
   - At the cut-over, development of the carried code moves here, and
     demiurge-chain's copies freeze by a record of its own. Whether QQ's
     development then stays open source is decided at that point, as a
     product question. Until then QQ remains Apache-2.0 upstream.
4. **The network stays in demiurge-chain, public, under its rules.** That is
   the chain, QOR ID, Social, ARQADE's website and the homepage. QOR OS talks
   to them as services: QOR ID over OAuth, Social over its API, and the chain
   through the QOR core (QOR-0003).
5. **Licences.**
   - Carried files remain under Apache-2.0, with their notices.
     [`LICENSES/Apache-2.0.txt`](../../LICENSES/Apache-2.0.txt) holds the
     text, and [`THIRD_PARTY_NOTICES.md`](../../THIRD_PARTY_NOTICES.md) lists
     the paths it covers.
   - A carried file changed here says so in a comment near its top, as
     Apache-2.0 §4(b) requires.
   - Everything else is proprietary under [`LICENSE`](../../LICENSE). This
     amends QTOS-0004, whose commercial-Qt and private-repository decisions
     stand.

## Consequences

- QOR OS's history begins with QTOS's 36 commits. The carried products bring
  their own: QQ 27, ARQADE's app 13, its games 8, its fixtures 5, QOR Media 14.
  Each arrives through one merge commit that names the upstream commit.
- The carried products build here with their own scripts
  ([`tools/dev/products.ps1`](../../tools/dev/products.ps1)). Windows CI runs
  their suites. The Linux build is Phase 1 work.
- The lint, header and formatting checks skip the carried paths, so they stay
  byte-for-byte mergeable with upstream.
- Two-way flow costs some discipline. A change that mixes a general fix with
  QOR OS work is split before it lands.
- The decision is wrong if the products' QOR OS work starts to dominate their
  changes. That is the signal to cut over early.
