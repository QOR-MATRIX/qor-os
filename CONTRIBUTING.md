# Contributing to QTOS

Thank you for helping build QTOS. This guide covers the workflow. The
technical details live in [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) and
[`docs/BUILDING.md`](docs/BUILDING.md).

## Before you start

- Read [`VISION.md`](VISION.md) (what QTOS is and why) and
  [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) (how the code is organised).
- Architectural changes need an ADR in [`docs/adr/`](docs/adr/). Copy
  [`0000-template.md`](docs/adr/0000-template.md), number it, and open it with
  your pull request.
- Visual changes to Lumen start from the tokens
  ([`kit/tokens/lumen.tokens.json`](kit/tokens/lumen.tokens.json)) and the spec
  ([`docs/design/LUMEN.md`](docs/design/LUMEN.md)).

## One-time setup

```powershell
git clone https://github.com/QOR-MATRIX/QTOS.git
cd QTOS
python -m pip install -r requirements-dev.txt   # clang-format 20.1.8 + pre-commit, pinned
pre-commit install                               # format, token and header checks on every commit
.\tools\dev\build.ps1 -Test                      # configure, build and test (windows-release)
```

## Workflow

1. **Branch from `main`.** Name branches by type:
   `feat/beam-unit-conversion`, `fix/shelf-hover-flicker`,
   `docs/services-reference`, `perf/backdrop-cache`.
2. **Commit in [Conventional Commits](https://www.conventionalcommits.org/) style.**
   Scopes follow the areas: `kit`, `style`, `shell`, `compositor`, `services`,
   `sim`, `apps`, `platform`, `image`, `sdk`, `tools`, `ci`, `docs`.

   ```text
   feat(shell): add Stash to the Shelf
   fix(kit): keep Spring velocity when retargeting during reduced motion
   perf(kit): cache Aurora noise field in a layer texture
   ```

3. **Keep changes reviewable.** One concern per pull request; split large work.
4. **Open a pull request** using the template. Visual changes need before and
   after screenshots from `tools/screenshots/capture.ps1`.
5. **Merge** by squash (default) or rebase. `main` keeps a linear history.

## Definition of done

- [ ] `cmake --build --preset windows-release` (or `linux-release`) builds without warnings
- [ ] `ctest --preset windows-release` passes
- [ ] `cmake --build --preset windows-release --target all_qmllint` reports zero warnings
- [ ] `pre-commit run --all-files` is clean (clang-format, generated tokens, SPDX headers)
- [ ] New behaviour has tests: C++ in `tests/`, QML logic in Qt Quick Test, Python tools with `unittest`
- [ ] Shell and Kit changes keep `qtos-shell --benchmark 8` p95 at or below the documented baseline
      ([`docs/PERFORMANCE.md`](docs/PERFORMANCE.md))
- [ ] Works in light and dark, with two accent hues, Reduce Motion and Reduce Transparency
- [ ] Documentation and [`CHANGELOG.md`](CHANGELOG.md) updated

## Code review

Reviewers look for correctness, adherence to the Lumen spec, render-on-demand
behaviour (no new continuous frames when idle), accessibility (keyboard focus,
`Accessible` roles), and API stability of service contracts in
`services/interfaces/`. Changing a `.rep` file changes a cross-process
contract: say so explicitly in the PR description.

## Licensing of contributions

QTOS is proprietary (see [`LICENSE`](LICENSE)). By contributing, you confirm
that you have the right to submit the work and you assign it to QOR-MATRIX
under the project's license. Do not add third-party code or assets without
recording them in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), and do
not add anything under copyleft licenses that would conflict with
proprietary distribution.

## Conduct

Participation is governed by the [Code of Conduct](CODE_OF_CONDUCT.md).
