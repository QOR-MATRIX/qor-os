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
| All five | demiurge-chain `28d9e97` (`main`, 26 October 2026) |

| QOR core pin | What that core has |
|--------------|---------------------|
| `28d9e970fef3bcedb0d0cfbdcf71c74949fed5ec` | Sparks and errors, the vault, profiles and their confirmations; bridge commands `core_version`, `token_info`, `parse_amount`, `format_sparks` |

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
2. Build and test against it, either by setting the pin or by pointing at a
   local checkout:

   ```bash
   cmake --preset windows-release -DQOR_CORE_COMMIT=<full hash>
   cmake --preset windows-release -DQOR_CORE_SOURCE_DIR=X:/Repoz/demiurge-chain   # a local checkout
   ```

3. Change `QOR_CORE_COMMIT` in [`core/CMakeLists.txt`](../core/CMakeLists.txt)
   and the table above in one commit, named "Pin the QOR core at
   demiurge-chain `<short hash>`", saying what changed in the core.
4. CI must pass, including `tst_qorcore`.

**Next move:** to the merge commit of
[demiurge-chain #107](https://github.com/QOR-MATRIX/demiurge-chain/pull/107)
(L13.2), once the owner merges it. That change moves QOR ID's client, the
chain client, Social's client, the Library's index, the Mesh and Qontrol into
the core, each behind a Cargo feature. `desktop` turns on all of them, and the
Windows launcher and QOR OS build with it (demiurge-chain's ADR-120). So the
same commit that moves the pin sets `QOR_CORE_FEATURES` to `desktop` in
`core/CMakeLists.txt`. Checked ahead on 26 October 2026: QOR OS configured
with `QOR_CORE_SOURCE_DIR` at #107's branch (`48e9bfa`) and
`QOR_CORE_FEATURES=desktop`, with warnings as errors. The whole core compiled
and linked on Windows in 3 min 22 s, and `tst_qorcore` passed 9 of 9.

Two of the owner's decisions come first:
- **ADR-121, proposed in that PR.** It extends the launcher's guarded audit
  ignore of RUSTSEC-2026-0344 (`ruzstd`, reached only through subxt's light
  client, which nothing builds) to the core's lockfile. The PR must not merge
  until it is accepted.
- **The crates' licence.** The manifests say MIT while the repository is
  Apache-2.0. ADR-120 there notes this as the owner's question.

## Cutting over

When QOR OS becomes where these products ship, a QOR record moves their
development here, and a record in demiurge-chain freezes its copies. From then
on there are no more pulls, and the carried paths are QOR OS's own. Whether
QQ's development stays open source is decided in that record.
