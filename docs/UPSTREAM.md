# Upstream: what QOR OS carries from demiurge-chain

QOR OS carries Demiurge's device-side Qt code with its history and builds the
QOR core from demiurge-chain at a pinned commit
([QOR-0002](decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md),
[QOR-0003](decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)).
demiurge-chain (`QOR-MATRIX/demiurge-chain`, public, Apache-2.0) stays their
upstream until a later record cuts over.

## What is carried

| Path (the same here and upstream) | What it is | Carried by | Upstream record |
|-----------------------------------|------------|------------|-----------------|
| `products/qq/` | QQ, the QOR Engine: the runtime, QQ Studio, the QQ Player, the agent's tools, tests | `git subtree`, with history | ADR-081, ADR-083, ADR-105 |
| `products/arqade/qt/` | ARQADE's Qt app | `git subtree`, with history | ADR-102 |
| `products/arqade/games/` | ARQADE's games and arenas, rebuilt in QQ | `git subtree`, with history | ADR-102 |
| `products/arqade/tests/fixtures/` | Recorded runs that ARQADE's games replay | `git subtree`, with history | ADR-116 |
| `tools/qor-media/` | QOR Media: player, viewer, editor, PDF reader | `git subtree`, with history | ADR-094 |
| `docs/design/tokens.json` | Demiurge's one look, read by the carried tests | copied at the same commit | ADR-108 |
| The QOR core (`tools/qor-launcher/core/` upstream) | Keys, profiles, amounts, the C bridge | built at a pinned commit, never copied | ADR-103, ADR-104 |

Everything Demiurge runs as a service stays upstream: the chain, QOR ID,
Social, ARQADE's website, the homepage and the QOR Launcher for Windows and
Android.

The paths match upstream so that ARQADE's build still finds QQ's runtime at
`../../qq/runtime`, its games at `../games` and its fixtures at
`../tests/fixtures`. Each product's tests also still find `docs/design/tokens.json`.

## State

| Carried path | Upstream commit it was last split at |
|--------------|---------------------------------------|
| All five | demiurge-chain `28d9e97` (`main`, 26 October 2026). Unchanged upstream as of `6a4822d` (28 October, the merge of #109), so no pull was needed |

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
tools/upstream/demiurge.sh pull            # demiurge-chain main
tools/upstream/demiurge.sh pull <commit>   # or a specific commit
pwsh tools/dev/products.ps1                # build and test what came in
```

The script keeps a clone of demiurge-chain in `../_upstream/demiurge-chain`
(set `QOR_UPSTREAM_DIR` to change it). For each carried path it splits
upstream's history there (`git subtree split`) and merges it here
(`git subtree pull`), one merge commit per path. Then it copies
`docs/design/tokens.json` at the same commit. Splitting is deterministic, so
every pull adds only what is new. On Windows the first split of a path takes
minutes.

## Changing carried code

- **A fix that isn't specific to QOR OS** goes upstream first, as a pull
  request to demiurge-chain under its rules (`AGENTS.md` there). It reaches
  QOR OS with the next pull.
- **Work that only QOR OS needs** is made here. That covers the Linux build,
  running as a Wayland client of Prism, Lumen theming, the controller service,
  and removing the Windows window docking. A carried file changed here
  gets a line near its top saying so, as Apache-2.0 §4(b) requires:

  ```cpp
  // Changed for QOR OS: <what changed and why>.
  ```

- **A fix made here first** goes upstream as a patch, because the paths are
  identical:

  ```bash
  tools/upstream/demiurge.sh patch <commit>   # writes build/upstream-patches/*.patch
  # then, on a branch of demiurge-chain:
  git am /path/to/qor-os/build/upstream-patches/*.patch
  ```

Keep each change to one kind: a commit that mixes a general fix with work for
QOR OS only is split before it lands.

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

Nothing. [demiurge-chain #109](https://github.com/QOR-MATRIX/demiurge-chain/pull/109) merged on 28 October 2026 at
`6a4822d`, and the pin moved to it (above). Both records QOR OS was waiting on
are accepted and on demiurge-chain's `main`:

- **ADR-121:** the core's audit ignores RUSTSEC-2026-0344 in `ruzstd`, which
  only subxt's light client reaches and nothing builds, as the launcher's audit
  already does. #107 carried it, and #108 recorded it accepted.
- **ADR-122:** the six active crates, the core among them, declare Apache-2.0,
  the repository's licence, instead of MIT. #109 carried it.

## Cutting over

When QOR OS becomes where these products ship, a QOR record moves their
development here, and a record in demiurge-chain freezes its copies. From then
on there are no more pulls, and the carried paths are QOR OS's own. Whether
QQ's development stays open source is decided in that record.
