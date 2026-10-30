# QOR-0015: QQ, ARQADE's app and games, and QOR Media are developed here

- **Status:** Accepted
- **Date:** 2026-10-29
- **Deciders:** the owner, who chose the assistant's recommended answer to each
  of three questions
- **Supersedes:** decision 3 of [QOR-0002](QOR-0002-one-repository-from-qtos-and-demiurge.md),
  for the four paths named below
- **Relates to:** QOR-0002, [QOR-0003](QOR-0003-the-qor-core-at-a-pinned-commit.md),
  [QOR-0008](QOR-0008-before-a-public-release.md); Demiurge
  [ADR-120](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-120-qor-os-carries-the-device-side-products.md)
  and ADR-126 (the same cut-over, seen from demiurge-chain, where it freezes
  the copies)

## Context

QOR-0002 carried QQ, ARQADE's Qt app, its games and their replay fixtures, and
QOR Media from demiurge-chain, with their history, at the same paths. It kept
demiurge-chain their upstream "until a later record cuts over", and named the
signal to cut over early: the products' work for QOR OS coming to dominate
their changes. On 29 October 2026 the owner asked to "move forward with their
development transition to this repo entirely". The state that day:

- **No change upstream.** Nothing in the carried paths or in
  `docs/design/tokens.json` changed in demiurge-chain after the carry at
  `28d9e97`, through 23 commits to `db98ac0` (its `main` that day).
- **Every change was made here.** The products' changes since the carry are
  QOR OS's:
  - the Linux build ([BUILDING.md](../BUILDING.md#6-the-products-qq-arqade-qor-media));
  - the agent's key in the Secret Service (QOR-0011);
  - placement by Prism on Wayland (QOR-0010).

  Of the 374 files in the four product paths, these changes touched eleven, and
  each says so near its top.
- **Only CI here can test them.** demiurge-chain's CI has no licensed Qt, so
  the products' Qt suites run there only on the owner's PC. Here CI builds and
  tests them with the licensed kit.
- **The Windows launcher ships them.** The QOR Launcher for Windows (0.1.17,
  published from demiurge-chain) builds and ships them from demiurge-chain's
  copies (`scripts/stage-arqade.mjs`, `scripts/stage-qor-media.mjs` and
  `src-tauri/src/qq.rs` in `tools/qor-launcher/`).
- **The rules and fixtures are shared.** Five modules in the games hold
  ARQADE's rules and nothing else, importing nothing:
  - `logic/rules.mjs` of Orbital, Void Runner, Synapse and Rift Survivor;
  - the arenas' `logic/arena.mjs`.

  The games here play by them in QQ. Upstream, ARQADE's Node tests
  (`products/arqade/tests/*.test.mjs`, in its CI) import them from
  `products/arqade/games/` and replay the recorded runs in
  `products/arqade/tests/fixtures/`. ARQADE's service is to confirm runs by the
  same rules (Demiurge's ADR-102 decision 5; ADR-116, proposed).

The owner was asked three questions and chose the recommended answer to each:

- what moves: "The products", with the core and both launchers staying
  upstream;
- the licence of new work on QQ: "Closed until the SDK";
- what the Windows launcher ships: "Ships frozen copies".

## Decision

1. **The four product paths are developed in QOR OS from 29 October 2026.**
   - The paths are `products/qq/`, `products/arqade/qt/`,
     `products/arqade/games/` and `tools/qor-media/`.
   - They are no longer pulled from demiurge-chain, whose copies are frozen by
     its ADR-126.
   - Their last upstream commit is `28d9e97`, the same in these paths as
     `db98ac0`.
   - This supersedes QOR-0002 decision 3 for them.
2. **What is still shared with demiurge-chain.**
   - **ARQADE's rules and fixtures**, because its server and its Node tests
     judge runs by the same files:
     - The five rule modules stay identical here and upstream. A change to one
       is made in both places at once, with the fixture it changes, by a pull
       request there.
     - The fixtures stay carried and are still pulled.
     - `tools/upstream/demiurge.sh patch` writes such a change for upstream.
   - **`docs/design/tokens.json`**, Demiurge's one look, is still copied, until
     Phase 1 gives the products' look one source in Lumen.
   - **The QOR core** stays at a pinned commit (QOR-0003), because both
     launchers build it too.
3. **Licence.**
   - The 374 files in the four paths, listed in
     [`LICENSES/from-demiurge-chain.txt`](../../LICENSES/from-demiurge-chain.txt),
     came from demiurge-chain under Apache-2.0. They stay under it, with their
     notices.
   - That includes the eleven changed here before today, under Apache-2.0 as
     the `LICENSE` then said.
   - Work on them from today is QOR OS's, under its proprietary `LICENSE`.
     Apache-2.0 §4 allows a modification its own terms. A changed file still
     says so near its top (§4(b)).
   - The exception is the five rule modules. They are published upstream as
     they change (decision 2), so they stay under Apache-2.0.
   - A file added to these paths carries QOR OS's SPDX header, and
     `tools/ci/check_headers.py` checks that.
   - Whether QQ's source is published again is decided with the public SDK
     (Phase 4), in its own record. Until then, none of the new work is
     published.
4. **The products keep their own form.**
   - Their build scripts, CMake projects and formatting stay as they are, and
     the formatting hooks still skip them.
   - Their open items in Demiurge's roadmap move into
     [ROADMAP.md](../ROADMAP.md).
5. **The QOR Launcher for Windows ships the frozen copies.**
   - It stays in demiurge-chain with the network, and builds the products as
     they were at the freeze.
   - New work on the products reaches people through QOR OS's own release,
     once QOR-0008's gates hold.
   - Only the owner releases a fix into the launcher's copies, by a pull request
     there. That fix is then Apache-2.0, like the rest of that repository.

## Consequences

- **One set of rules.** The products follow this repository's rules alone.
  Demiurge's rules on money, language and secrets were already in `AGENTS.md`.
- **No two-way flow.** Four paths are no longer pulled, and a general fix is
  no longer made upstream first.
- **Rule changes cross repositories.** A change to a game's rules now spans
  two repositories: its module and its fixture change in both. Published
  upstream, it also reaches the launcher's frozen games at the launcher's next
  release, so it has to suit them too.
- **The launcher's users keep 0.1.17's products** unless the owner releases a
  fix to them.
- **A sign this is wrong:** the launcher needs product fixes often. Then a later
  record has it take QOR OS's builds, or moves it here.
