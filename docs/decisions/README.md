# Decisions

The only decision log for QOR OS ([QOR-0001](QOR-0001-one-rulebook-and-one-decision-log.md)).
Every decision is cited by its prefix and number. New records copy
[`TEMPLATE.md`](TEMPLATE.md), take the next QOR number, and are added here in
the same change. A record is never edited to change its decision; a later
record supersedes it, and both say so.

## QOR OS

| Record | Decision | Status |
|--------|----------|--------|
| [QOR-0001](QOR-0001-one-rulebook-and-one-decision-log.md) | One rulebook (`AGENTS.md`) and this one log; three series: QOR, QTOS, Demiurge ADR | Accepted 2026-10-26 |
| [QOR-0002](QOR-0002-one-repository-from-qtos-and-demiurge.md) | One private repository from QTOS's history; QQ, ARQADE and QOR Media carried with theirs at upstream paths; the network stays in demiurge-chain | Accepted 2026-10-26; decision 3 superseded by QOR-0015 for the products |
| [QOR-0003](QOR-0003-the-qor-core-at-a-pinned-commit.md) | The QOR core is built from demiurge-chain at a pinned commit, never copied; reached only through its bridge; a service of its own on the device | Accepted 2026-10-26 |
| [QOR-0004](QOR-0004-the-base-image-is-fedora-atomic.md) | The image is built on Fedora Atomic with bootc: NVIDIA open modules, Mesa, whole-image updates with rollback; Yocto retired | Accepted 2026-10-26 |
| [QOR-0005](QOR-0005-every-display-at-its-own-refresh.md) | Every display at its own refresh rate on every GPU is the acceptance test; Prism's own KMS backend; the bare-metal gate deferred, not failed | Accepted 2026-10-26 |
| [QOR-0006](QOR-0006-qor-os-1-0-moves-no-money.md) | 1.0 moves no money: free items and test CGT only; Demiurge's money rules apply everywhere | Accepted 2026-10-26 |
| [QOR-0007](QOR-0007-what-1-0-ships.md) | What 1.0 ships, and what waits | Accepted 2026-10-26 |
| [QOR-0008](QOR-0008-before-a-public-release.md) | Release gates the owner holds (Qt distribution licence, name clearance, legal list, keys, sign-in redundancy); the code keeps QTOS names until the name clears | Accepted 2026-10-26; decision 2 superseded by QOR-0009 |
| [QOR-0009](QOR-0009-the-name-is-qor-os.md) | The name is QOR OS, in the code too: `qor-os`, `qor`, `QOR.*`, `QOR_*`, `dev.qorsync.*` where a space cannot go; QTOS names only the predecessor | Accepted 2026-10-27 |
| [QOR-0010](QOR-0010-on-wayland-prism-places-the-products-windows.md) | On Wayland the products' windows stay toplevels and Prism places them; QOR Media's docking case is skipped there | Accepted 2026-10-28 |
| [QOR-0011](QOR-0011-the-creators-provider-key-in-the-secret-service.md) | A creator's own provider key is not a QOR key: it stays in the platform's keychain, the Secret Service on Linux | Accepted 2026-10-28 |
| [QOR-0012](QOR-0012-github-stays-the-source-of-truth-entire-mirrors-it.md) | GitHub stays the source of truth; Entire mirrors QOR OS once it hosts private repositories | Superseded by QOR-0014 (decision 2 first by QOR-0013), 2026-10-29 |
| [QOR-0013](QOR-0013-entire-may-reach-every-repository.md) | Entire's app may reach every QOR-MATRIX repository, QOR OS included; GitHub stays the source of truth | Superseded by QOR-0014, 2026-10-29 |
| [QOR-0014](QOR-0014-qor-os-does-not-use-entire.md) | QOR OS does not use Entire: GitHub alone, as before; Entire's app removed from the organisation | Accepted 2026-10-29 |
| [QOR-0015](QOR-0015-the-products-are-developed-here.md) | QQ, ARQADE's app and games, and QOR Media are developed here, no longer pulled; demiurge-chain's copies frozen (its ADR-126), which the Windows launcher ships; ARQADE's fixtures, the one look and the core still come from upstream; new work is proprietary until the SDK's record | Accepted 2026-10-29; its point on `tokens.json` amended by QOR-0016, its shared rule modules by QOR-0017 |
| [QOR-0016](QOR-0016-the-products-wear-lumen.md) | The products wear Lumen: Demiurge's one look mapped onto Lumen's roles in `lumen.tokens.json`, resolved dark with the person's accent, tint and contrast; Prism gives them to every product (`theme`, `QOR_THEME_COLORS`) | Accepted 2026-10-30 |
| [QOR-0017](QOR-0017-arqades-new-games.md) | ARQADE's new games: seven original games by genre, the dungeon hack-and-slash and the 3D RPG first; rewarded from the start, their rules, Node tests and fixtures shared with demiurge-chain under Apache-2.0; the online world after Phase 2; names wait for the owner | Accepted 2026-10-30 |
| [QOR-0019](QOR-0019-ever-rift-and-darkveil.md) | The owner names the first two of ARQADE's new games: Rift Delve is Ever Rift, Gloamreach is Darkveil, in every path and identifier; Darkveil's story takes "the veil" from its name; the rules decide what they decided; both cross to demiurge-chain | Accepted |
| [QOR-0018](QOR-0018-the-security-and-quality-bar.md) | The security and quality bar, checked by machines: exploit mitigations on every program and read back in CI, the suites under sanitizers, what CI runs pinned by content, Dependabot's pull requests off the workstation, secrets looked for at the door and in the history, the core's dependencies audited weekly, and suites deaf to the machine they run on | Accepted 2026-10-30 |

## QTOS (inherited)

QTOS's records, moved unchanged from `docs/adr/` (where they were numbered
ADR-0001 to ADR-0010) into [`qtos/`](qtos/). They stand unless a QOR record
says otherwise.

| Record | Decision | Status in QOR OS |
|--------|----------|------------------|
| [QTOS-0001](qtos/QTOS-0001-record-architecture-decisions.md) | Record decisions as ADRs in `docs/adr/` | Superseded by QOR-0001 |
| [QTOS-0002](qtos/QTOS-0002-linux-kernel-and-the-qt-line.md) | Linux kernel underneath, Qt above "the Qt Line" | Stands; extended by QOR-0004 |
| [QTOS-0003](qtos/QTOS-0003-compositor-strategy.md) | Prism on Qt Wayland Compositor, with a decision gate | Stands; gate deferred by QOR-0005 |
| [QTOS-0004](qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md) | Commercial Qt; a private, proprietary repository | Stands; amended by QOR-0002 (carried Apache-2.0 code) and QOR-0008 |
| [QTOS-0005](qtos/QTOS-0005-qt-6-12-baseline.md) | Qt 6.12 baseline | Stands |
| [QTOS-0006](qtos/QTOS-0006-service-ipc-qt-remote-objects.md) | Service IPC over Qt Remote Objects | Stands |
| [QTOS-0007](qtos/QTOS-0007-lumen-style-on-quick-templates.md) | Lumen style on Qt Quick Templates | Stands |
| [QTOS-0008](qtos/QTOS-0008-simulation-first-development.md) | Simulation-first development | Stands |
| [QTOS-0009](qtos/QTOS-0009-oklch-tokens-single-source.md) | OKLCH tokens from a single source | Stands |
| [QTOS-0010](qtos/QTOS-0010-materials-and-render-on-demand.md) | Shared materials and render on demand | Stands |

## Demiurge records QOR OS depends on

Demiurge's records live in
[demiurge-chain](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/DECISIONS.md)
and are never copied here. These are the ones QOR OS builds on:

| Record | What it settles for QOR OS |
|--------|----------------------------|
| [ADR-002](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-002-value-from-spending.md), [ADR-006](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-006-demand-sinks.md) | Every mechanic that touches CGT passes the spend test and names its demand sink |
| [ADR-008](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-008-language-discipline.md) | No investment language, anywhere |
| [ADR-035](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-035-eighteen-decimals.md), [ADR-101](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-101-the-spark-is-a-thousandth-of-a-cgt.md) | CGT has 18 decimals; amounts are integer embers; a Spark is 10^15 embers |
| [ADR-081](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-081-qq-is-qor-engine.md), [ADR-083](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-083-qq-on-qt.md), [ADR-105](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-105-qq-makes-real-games-in-3d-and-2d.md) | QQ is the QOR Engine, on Qt 6, for 3D and 2D games |
| [ADR-087](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-087-strangers-games-are-confined.md) | A stranger's game is confined; QOR OS's sandbox delivers the operating-system half |
| [ADR-091](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-091-social.md) | Social |
| [ADR-094](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-094-the-library-and-qor-media.md) | The Library and QOR Media |
| [ADR-102](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-102-arqade-in-the-launcher-on-qt.md) | ARQADE on Qt |
| [ADR-103](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-103-the-launcher-moves-to-qt-in-stages.md), [ADR-104](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-104-the-launcher-on-android-on-qt.md) | Every QOR surface moves to Qt over the Rust core and its C bridge |
| [ADR-108](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-108-one-look-obsidian-and-gilt-in-two-registers.md) | Demiurge's one look (`docs/design/tokens.json`), which Phase 1 maps onto Lumen |
| [ADR-112](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-112-the-controllers-own-key-signs-every-agent-authorisation.md) | The controller's own key signs every agent authorisation |
