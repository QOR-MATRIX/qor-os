# Contributing to QOR OS

Thank you for helping build QOR OS. This guide covers the workflow. The rules
that bind everyone, people and AI assistants alike, are in
[`AGENTS.md`](AGENTS.md). The technical details live in
[`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) and
[`docs/BUILDING.md`](docs/BUILDING.md).

## Before you start

- Read [`VISION.md`](VISION.md) (what QOR OS is and why),
  [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) (how the code is organised)
  and [`docs/ROADMAP.md`](docs/ROADMAP.md) (what is next).
- Architectural changes need a decision record in
  [`docs/decisions/`](docs/decisions/README.md). Copy
  [`TEMPLATE.md`](docs/decisions/TEMPLATE.md), take the next `QOR-NNNN` number,
  index it in the README there, and open it with your change.
- Visual changes to Lumen start from the tokens
  ([`kit/tokens/lumen.tokens.json`](kit/tokens/lumen.tokens.json)) and the spec
  ([`docs/design/LUMEN.md`](docs/design/LUMEN.md)).
- Changes to QQ, ARQADE or QOR Media follow [`docs/UPSTREAM.md`](docs/UPSTREAM.md).
  A general fix goes to demiurge-chain first; work only QOR OS needs is done
  here and marked `Changed for QOR OS:`.
- Changes to the QOR core are made in demiurge-chain, under its rules, and
  reach QOR OS when the pin moves
  ([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)).

## One-time setup

```powershell
git clone https://github.com/QOR-MATRIX/qor-os.git
cd qor-os
python -m venv .venv                                         # repo-local, gitignored
.\.venv\Scripts\python -m pip install -r requirements-dev.txt   # clang-format 20.1.8, pre-commit, cryptography, pinned
.\.venv\Scripts\pre-commit install                     # format, token and header checks on every commit
.\tools\dev\build.ps1 -Test                      # configure, build and test (windows-release), with the QOR core
.\tools\dev\products.ps1                         # build and test QQ, ARQADE and QOR Media
```

Rust (1.91 or later, from [rustup](https://rustup.rs)) is needed for the QOR
core.

## Workflow

1. **Branch from `main`.** Name branches by type:
   `feat/beam-unit-conversion`, `fix/shelf-hover-flicker`,
   `docs/services-reference`, `perf/backdrop-cache`.
2. **Commit in [Conventional Commits](https://www.conventionalcommits.org/) style.**
   Scopes follow the areas: `kit`, `style`, `shell`, `compositor`, `services`,
   `sim`, `apps`, `store`, `input`, `audio`, `platform`, `core`, `qq`, `arqade`,
   `media`, `upstream`, `image`, `sdk`, `tools`, `ci`, `docs`.

   ```text
   feat(shell): add Stash to the Shelf
   fix(kit): keep Spring velocity when retargeting during reduced motion
   feat(qq): run as a Wayland client of Prism
   ```

3. **Keep changes reviewable.** One concern per pull request; split large work.
4. **Open a pull request** using the template. Visual changes need before and
   after screenshots from `tools/screenshots/capture.ps1`.
5. **Merge** by squash (default) or rebase. `main` keeps a linear history,
   except for the merge commits that `git subtree` makes when it pulls from
   upstream.

## Definition of done

- [ ] `cmake --build --preset windows-release` (or `linux-release`) builds without warnings
- [ ] `ctest --preset windows-release` passes, `tst_qorcore` included
- [ ] `cmake --build --preset windows-release --target all_qmllint` reports zero warnings
- [ ] `pre-commit run --all-files` is clean (clang-format, generated tokens, SPDX headers)
- [ ] Changes that touch QQ, ARQADE or QOR Media pass `tools/dev/products.ps1`
- [ ] New behaviour has tests: C++ in `tests/`, QML logic in Qt Quick Test, Python tools with `unittest`
- [ ] Shell and Kit changes keep `qor-shell --benchmark 8` p95 at or below the documented baseline
      ([`docs/PERFORMANCE.md`](docs/PERFORMANCE.md))
- [ ] Works in light and dark, with two accent hues, Reduce Motion and Reduce Transparency, and with a controller
- [ ] Documentation, [`CHANGELOG.md`](CHANGELOG.md) and, at the end of a session, [`OWNER.md`](OWNER.md) updated
- [ ] Nothing secret in the change, its logs or its screenshots; money only as integer amounts ([`AGENTS.md`](AGENTS.md))

## Code review

Reviewers look for correctness, adherence to the Lumen spec, render-on-demand
behaviour (no new continuous frames when idle), accessibility (keyboard focus,
`Accessible` roles, controller navigation), and API stability of service
contracts in `services/interfaces/`. Changing a `.rep` file changes a
cross-process contract, and so does a new QOR core bridge command: say so
explicitly in the PR description.

## Licensing of contributions

QOR OS is proprietary (see [`LICENSE`](LICENSE)), except the code carried from
demiurge-chain, which remains Apache-2.0
([`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). By contributing, you
confirm that you have the right to submit the work and you assign it to
QOR-MATRIX under the project's license. A change to a carried file is made
under Apache-2.0 and marked as changed. Do not add third-party code or assets
without recording them in `THIRD_PARTY_NOTICES.md`, and do not add anything
under copyleft licenses that would conflict with proprietary distribution.

## Conduct

Participation is governed by the [Code of Conduct](CODE_OF_CONDUCT.md).
