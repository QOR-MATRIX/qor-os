# Rules for everyone working on QOR OS

These rules bind every contributor to this repository: the owner, other
people, and every AI assistant (Claude, Codex, Cursor, Copilot and any other).
They merge Demiurge's rules ([`AGENTS.md` in demiurge-chain](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/AGENTS.md))
with QTOS's build and CI practice, as
[QOR-0001](docs/decisions/QOR-0001-one-rulebook-and-one-decision-log.md)
decided. Where this file and an upstream rule both apply (in the carried code
or the QOR core), the stricter one wins.

## 1. Read these, in this order

1. [`OWNER.md`](OWNER.md): where QOR OS stands, on one page
2. [`docs/ROADMAP.md`](docs/ROADMAP.md): the only roadmap
3. [`docs/decisions/README.md`](docs/decisions/README.md): the only decision log (QOR, QTOS and Demiurge records)
4. [`VISION.md`](VISION.md): what QOR OS is and its architecture
5. [`docs/UPSTREAM.md`](docs/UPSTREAM.md): what is carried from demiurge-chain, and how it moves
6. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), [`docs/BUILDING.md`](docs/BUILDING.md), [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md)

[`docs/README.md`](docs/README.md) lists every current document. A document not
in that list is not current.

## 2. The code is the truth

- Verify a claim against the code before repeating it, in a document, a
  commit message or a reply.
- When behaviour changes, update the affected documents in the same change:
  [`CHANGELOG.md`](CHANGELOG.md) always, and whichever of the roadmap, the
  architecture, the building guide or a subsystem's document it touches.
- Never add a second roadmap, status table, decision list or economic model.
  Update the existing one.
- A new decision is a new QOR record in `docs/decisions/`, indexed in
  `docs/decisions/README.md`. A record is never edited to change its decision:
  a later record supersedes it, and both say so.
- **[`OWNER.md`](OWNER.md) is rewritten at the end of every session that
  changes QOR OS.** It stays under one printed page, glosses every term of art
  on first use, and contains no claim that was not checked against the tree in
  that session. A session that leaves it stale is not finished.

## 3. Scope and boundaries

- **This repository** is QOR OS: everything that runs on the device.
- **Carried code**:
  - The carried paths are `products/qq/`, `products/arqade/qt/`,
    `products/arqade/games/`, `products/arqade/tests/fixtures/`,
    `tools/qor-media/` and `docs/design/tokens.json`. They come from
    demiurge-chain under Apache-2.0
    ([QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md)).
  - Change them only as [`docs/UPSTREAM.md`](docs/UPSTREAM.md) says. General
    fixes go upstream first; work only QOR OS needs is done here.
  - Keep their licence notices, and mark a carried file changed here with a
    `Changed for QOR OS:` comment.
  - Lint, header and format checks skip them on purpose; don't reformat them.
- **The QOR core** (keys, profiles, amounts, the chain client) is built from
  demiurge-chain at a pinned commit and is never copied here
  ([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)).
  Change it upstream, under demiurge-chain's `AGENTS.md`, then move the pin.
- **The network** is the chain, QOR ID, Social, ARQADE's website and the
  homepage. It stays in demiurge-chain, public, under its rules. QOR OS talks
  to it as services.
- **QTOS** (`QOR-MATRIX/QTOS`) is frozen: its code at `37b621e`, plus a note saying so (`3ecdded`). Do no new work there.
- **Demiurge's old documentation does not exist.** Its pre-realignment
  documents were deleted on 13 September 2026. Never read, restore, quote or
  rely on them, including from git history. The carried histories begin after
  that date.

## 4. Money

From Demiurge's rules and
[QOR-0006](docs/decisions/QOR-0006-qor-os-1-0-moves-no-money.md):

- **QOR OS 1.0 moves no money.** It has free items and test CGT on the devnet
  only. No code that charges, pays out, holds a balance, or converts between
  dollars and CGT reaches a build anyone installs.
- CGT has 18 decimals. Amounts are integer **embers** in `u128` (1 CGT =
  10^18 embers; a Spark is 10^15). They cross the core's bridge as decimal
  strings.
- Never use floating point for money. Refuse excess precision; never truncate
  an amount.
- Never invent a value that Demiurge's `docs/economics/OPEN_QUESTIONS.md` lists
  as undecided: no issuance, split, curve, burn share, fee or platform share,
  in code, tests or documents. A test that needs one uses an obvious
  placeholder.
- Every mechanic that touches CGT passes the spend test (ADR-002) and names
  its demand sink (ADR-006).

## 5. Language

QOR is infrastructure for players and makers, not a vehicle for appreciation
(Demiurge ADR-008). These never appear in copy, comments, identifiers or
documents:

- APY, yield, or return in the financial sense;
- ROI, investment, passive income;
- any statement about CGT's price or future value.

Makers are paid when their work is used, licensed or remixed.

## 6. Keys and secrets

- **Code that handles keys lives only in the QOR core.** QOR OS reaches it
  through the bridge, and no command answers with a key. On the device the
  vault is a service of its own, outside Prism and every app, and its
  approvals are drawn by Prism in the trusted layer.
- **Nothing secret reaches a log**, test output, CI output, screenshot or
  commit. That covers tokens, keys, passwords, recovery phrases, backup
  codes, sign-in links and email addresses. A new flow that handles one of
  them is checked for this in the same change, whatever the value is called.
- **Secrets stay out of the repository:**
  - The Store's signing keys live on the owner's machine, in
    `~/.qor-os/store-signing/` (or QTOS's `~/.qtos/store-signing/` until the
    owner moves them; `tools/store/build_store.py` uses whichever holds them).
  - Qt Account credentials (`qtaccount.ini`) are never printed or copied into
    the tree.
  - Runner registration tokens are piped from `gh`, never printed.
  - Never open `.env` files, files named `*secret*`, or key files belonging to
    demiurge-chain.
- SQL values are bound, never written into a statement.

## 7. Building, testing and CI

QTOS's practice, which QOR OS keeps:

- **Qt 6.12, the commercial kit, only**
  ([QTOS-0004](docs/decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md),
  [QTOS-0005](docs/decisions/qtos/QTOS-0005-qt-6-12-baseline.md)).
  Builds use the CMake presets; CI's presets treat warnings as errors and
  require the QOR core.
- **Rust 1.91 or later** builds the QOR core through Corrosion; CI uses 1.98.1.
- **Before calling work done:**
  - `cmake --build --preset <preset>` builds without warnings, and
    `ctest --preset <preset>` passes;
  - `all_qmllint` reports zero warnings;
  - `pre-commit run --all-files` is clean (clang-format 20.1.8, generated
    tokens, SPDX headers);
  - for the carried products, `tools/dev/products.ps1` passes.
- **Every source file carries `SPDX-License-Identifier`**, except the carried
  code, which keeps its own notices.
- **New behaviour has tests**: Qt Test for C++, Qt Quick Test for QML logic,
  `unittest` for Python, and golden images for Kit components.
- **Render on demand.** No continuous frames or wakeups when nothing changes.
  Shell and Kit changes are benchmarked before and after on the same machine
  ([docs/PERFORMANCE.md](docs/PERFORMANCE.md)).
- **CI runs on self-hosted runners with the licensed kit** ([docs/CI.md](docs/CI.md)).
  If CI does not run, first check that the runners are online.
- **Report failures honestly, with the output.** Never weaken a test or a
  check to make it pass. Loosening a check is the owner's decision.
- **On the owner's Windows machine:**
  - Never kill or relink a running `qor-shell` from `build/windows-release`;
    build and test in another preset's folder (`build/ci-windows`).
  - `tools/store/build_store.py fixtures` re-signs everything and is not
    idempotent. Run it only when fixtures must change, never chained into a
    commit.

## 8. Working agreements

- Commit and push only work the owner has asked for. Never publish an image,
  release, package or deployment without the owner's explicit word.
- Commit messages say what changed and why, in plain words. Commits by an
  assistant end with its `Co-Authored-By` line.
- Ask the owner before naming a new product, service or public module.
- **Names** ([QOR-0009](docs/decisions/QOR-0009-the-name-is-qor-os.md)). The product is **QOR OS**:
  two words, capitals, one space, in prose, the interface and metadata. Where a
  space cannot go:
  - `qor-os` for the repository, packages, artifacts and the CMake project;
  - `qor` in code: the `qor::` namespace, `qor_*` targets, `qor-*` programs
    (`qor-shell`, `qorctl`, `qor-simd`), `qor.*` logging categories;
  - `QOR.*` QML modules, beside the carried `QOR.Media`;
  - `QOR_*` options, environment and CI variables;
  - `dev.qorsync.*` for identifiers that need a domain (Store packages).

  "QTOS" names only the predecessor: its frozen repository, its records
  (QTOS-0001 to 0010) and its history.

  The parts keep their names: Prism (compositor and shell), Lumen (design
  language), Beam, Shelf and Rail.
