# Upstream: what QOR OS shares with demiurge-chain

demiurge-chain (`QOR-MATRIX/demiurge-chain`, public, Apache-2.0) is
Demiurge's repository: the chain, QOR ID, Social, the websites and the QOR
Launcher. QOR OS shares three things with it:

- the QOR core, built from it at a pinned commit
  ([QOR-0003](decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md));
- ARQADE's rules and recorded runs;
- Demiurge's one look.

QQ, ARQADE's app and games, and QOR Media came from it with their history
([QOR-0002](decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md)).
They are developed here since 29 October 2026
([QOR-0015](decisions/QOR-0015-the-products-are-developed-here.md)), and
demiurge-chain's copies of them are frozen (its ADR-126).

## What is shared

| Path (the same here and upstream) | What it is | How | Upstream record |
|-----------------------------------|------------|-----|-----------------|
| `products/arqade/tests/fixtures/` | Recorded runs that ARQADE's games replay here and its rules replay upstream | carried by `git subtree`, with history | ADR-102 |
| ARQADE's rules: `logic/rules.mjs` of Orbital, Void Runner, Synapse and Rift Survivor, and the arenas' `logic/arena.mjs`, in `products/arqade/games/` | The rules and nothing else, which ARQADE's Node tests upstream import and its service is to confirm runs by | kept identical, changed in both at once ([below](#changing-a-games-rules)) | ADR-102, ADR-116 (proposed) |
| QOR OS's own games ([QOR-0017](decisions/QOR-0017-arqades-new-games.md)): Rift Delve's and Gloamreach's `logic/rules.mjs`, their Node tests in `products/arqade/tests/` (and Gloamreach's pilot) and their fixtures | Written here, under Apache-2.0; its Node test runs in QOR OS's lint workflow too | not yet upstream: taken there by one pull request once the owner names the game, then kept identical like the rules above | (a record there, with the pull request) |
| `docs/design/tokens.json` | Demiurge's one look: the products' own colours outside QOR OS, and what their tests read. In QOR OS they wear Lumen's roles under its names ([QOR-0016](decisions/QOR-0016-the-products-wear-lumen.md)) | copied at the same commit | ADR-108 |
| The QOR core (`tools/qor-launcher/core/` upstream) | Keys, profiles, amounts, the C bridge | built at a pinned commit, never copied | ADR-103, ADR-104 |

Everything Demiurge runs as a service stays upstream: the chain, QOR ID,
Social, ARQADE's website, the homepage and the QOR Launcher for Windows and
Android.

## The products, developed here

| Path | What it is | Upstream record |
|------|------------|-----------------|
| `products/qq/` | QQ, the QOR Engine: the runtime, QQ Studio, the QQ Player, the agent's tools, tests | ADR-081, ADR-083, ADR-105 |
| `products/arqade/qt/` | ARQADE's Qt app | ADR-102 |
| `products/arqade/games/` | ARQADE's games and arenas, rebuilt in QQ | ADR-102 |
| `tools/qor-media/` | QOR Media: player, viewer, editor, PDF reader | ADR-094 |

They came by `git subtree`, with their history, at the paths they have
upstream. They keep those paths, so that ARQADE's build still finds QQ's
runtime at `../../qq/runtime`, its games at `../games` and its fixtures at
`../tests/fixtures`, and each product's tests still find
`docs/design/tokens.json`.

Since QOR-0015:

- **They are not pulled**, and nothing of them goes upstream but ARQADE's
  rules.
- **Licence.** Their files from demiurge-chain stay Apache-2.0; they are
  listed in [`LICENSES/from-demiurge-chain.txt`](../LICENSES/from-demiurge-chain.txt),
  and one changed here says so near its top. Other work on them is QOR OS's,
  under [`LICENSE`](../LICENSE).
- **The Windows launcher** ships demiurge-chain's frozen copies, the same code
  as its 0.1.17. Only the owner releases a fix into them, by a pull request
  there.
- **Their open items** from Demiurge's roadmap are in [ROADMAP.md](ROADMAP.md).

## State

| Path | Upstream commit it was last split at |
|------|---------------------------------------|
| `products/arqade/tests/fixtures/` | demiurge-chain `28d9e97` (`main`, 26 October 2026). Unchanged upstream as of `8b4344e` (29 October, the merge of #113), so no pull was needed |
| The four products | `28d9e97`, identical in their paths to `db98ac0`, where demiurge-chain froze its copies. Developed here since |

| QOR core pin | Features | What that core has |
|--------------|----------|---------------------|
| `6a4822d10c93694d6d12abf33a75b4df6cac918a` (28 October 2026, the merge of #109) | `desktop` | The same code as `e99c7ce`. Its manifest declares Apache-2.0 instead of MIT (demiurge-chain's ADR-122) |
| `e99c7ce62045da08701a2ad6f7769a1a1d87afe6` (27 October 2026, the merge of #107) | `desktop` | Everything below, plus the modules L13.2 moved in from the Windows launcher: QOR ID's and Social's clients, the chain client, the Library's index, the Mesh and Qontrol. The bridge's commands are unchanged |
| `28d9e970fef3bcedb0d0cfbdcf71c74949fed5ec` (26 October 2026, the first pin) | none | Sparks and errors, the vault, profiles and their confirmations; bridge commands `core_version`, `token_info`, `parse_amount`, `format_sparks` |

`tools/upstream/demiurge.sh status` prints the live values. Update the tables
above whenever they change.

## Pulling from upstream

From a clean working tree, in Git Bash or WSL:

```bash
tools/upstream/demiurge.sh pull            # the fixtures and the look, from demiurge-chain main
tools/upstream/demiurge.sh pull <commit>   # or a specific commit
pwsh tools/dev/products.ps1                # build and test the products against them
```

The script keeps a clone of demiurge-chain in `../_upstream/demiurge-chain`
(set `QOR_UPSTREAM_DIR` to change it). It splits the fixtures' upstream
history there (`git subtree split`) and merges it here (`git subtree pull`),
in one merge commit. Then it copies `docs/design/tokens.json` at the same
commit, and says if ARQADE's rules differ from upstream's. Splitting is
deterministic, so every pull adds only what is new.

## Changing the products

A change to a product is made here, like any other in QOR OS. A file from
demiurge-chain that is changed gets a line near its top saying so, as
Apache-2.0 §4(b) requires:

```cpp
// Changed for QOR OS: <what changed and why>.
```

A file added to a product carries QOR OS's SPDX header
(`tools/ci/check_headers.py` checks it). The products keep their own build
scripts and formatting; don't reformat them.

## Changing a game's rules

ARQADE's rules live once, in each game's `logic/rules.mjs` (the arenas'
`logic/arena.mjs`), and are used in two repositories: by the game here, and
by ARQADE's Node tests and service upstream. Every change to a game's rules
fails its test until its fixture and the digest of the rules' state are made
again ([games/README.md](../products/arqade/games/README.md)). So a change to
the rules is made in both repositories at once:

1. **Here:** change the rules and the game, record the game's run again
   with ARQADE's games suite, and commit. For QOR OS's own games, update the
   digest their Node test pins here too (`node --test
   products/arqade/tests/<game>.test.mjs`).
2. **Upstream:** write the change as patches, which apply there because the
   paths are the same:

   ```bash
   tools/upstream/demiurge.sh patch <commit>   # the rules, fixtures, look and shared tests it changed
   # then, on a branch of demiurge-chain:
   git am /path/to/qor-os/build/upstream-patches/*.patch
   ```

   Update the digests its Node tests pin (`products/arqade/tests/`), and open
   a pull request there for the owner. Published there, the rules stay
   Apache-2.0, and reach the Windows launcher's frozen games at its next
   release, so the change has to suit those too.
3. **Once it is merged,** `tools/upstream/demiurge.sh status` shows the rules
   the same as upstream's again, and a pull takes the fixture back unchanged.

## Moving the core's pin

1. Pick a commit on demiurge-chain's `main` (never a branch commit).
2. Build and test against it from a checkout of that commit:

   ```bash
   git -C X:/Repoz/_upstream/demiurge-chain checkout <full hash>
   cmake --preset windows-release -DQOR_CORE_SOURCE_DIR=X:/Repoz/_upstream/demiurge-chain
   ```

3. Change `QOR_CORE_COMMIT` (and `QOR_CORE_FEATURES`, if the crate's features
   changed) in [`core/CMakeLists.txt`](../core/CMakeLists.txt) and the table
   above in one commit, named "Pin the QOR core at demiurge-chain
   `<short hash>`", saying what changed in the core. Both are written on every
   configure, so every build folder, CI's included, follows the move.
4. CI must pass, including `tst_qorcore`.

### Moves so far

- **28 October 2026, to `6a4822d`:** the merge of
  [demiurge-chain #109](https://github.com/QOR-MATRIX/demiurge-chain/pull/109), which the owner merged
  that day. The core's code did not change: its manifest declares Apache-2.0
  instead of MIT (demiurge-chain's ADR-122, accepted by the owner on 27
  October), and two comments, in the manifest and in `.cargo/audit.toml`, now
  record ADR-121 as accepted. At the merge the core built into QOR OS from the
  `ci-windows` preset and `tst_qorcore` passed 9 of 9.
  [`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md) changed in the same
  commit.
- **27 October 2026, to `e99c7ce`:** the merge of
  [demiurge-chain #107](https://github.com/QOR-MATRIX/demiurge-chain/pull/107)
  (L13.2), with the `desktop` feature. That change moved QOR ID's client, the
  chain client, Social's client, the Library's index, the Mesh and Qontrol from
  the Windows launcher into the core, each behind a Cargo feature, and
  `desktop` turns on all of them, as the Windows launcher and QOR OS build it
  (demiurge-chain's ADR-120). It was checked ahead at the PR's head (`48e9bfa`)
  and again at the merge: the whole core compiles and links into QOR OS with
  warnings as errors, and `tst_qorcore` passes 9 of 9. The move also stopped
  the pin and the features being cache options, so a build folder follows the
  pin in the file.
- **26 October 2026, `28d9e97`:** the first pin, with no features.

### Open upstream

Nothing. ADR-126, demiurge-chain's side of the cut-over (below), is on its
`main`: the owner merged
[demiurge-chain #113](https://github.com/QOR-MATRIX/demiurge-chain/pull/113)
on 29 October 2026, as `8b4344e`. In the products' paths it added only a note
at the top of each one's README there, and it changed nothing that is shared.

The records QOR OS waited on for the core are accepted and on demiurge-chain's
`main` too. ADR-121 (#107, accepted by #108) has the core's audit ignore
RUSTSEC-2026-0344 in `ruzstd`, which only subxt's light client reaches and
nothing builds. ADR-122 (#109) has the six active crates, the core among
them, declare Apache-2.0, the repository's licence, instead of MIT.

## The cut-over, 29 October 2026

QOR-0002 kept demiurge-chain the products' upstream until a record moved
their development here. On 29 October 2026 the owner asked to "move forward
with their development transition to this repo entirely", and chose:

- the products move, and the core and both launchers stay upstream;
- new work on QQ is closed until the public SDK's record decides whether its
  source is published;
- the Windows launcher ships frozen copies.

[QOR-0015](decisions/QOR-0015-the-products-are-developed-here.md) records it
here and ADR-126 upstream. Nothing had changed upstream in the products'
paths since the carry, so nothing was pulled first. From then on, the
products' paths are QOR OS's own, except ARQADE's rules, which stay shared.
