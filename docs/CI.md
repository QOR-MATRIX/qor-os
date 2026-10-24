# Continuous integration

Workflows live in [`.github/workflows/`](../.github/workflows/).

| Workflow | Runs on | Needs Qt | Checks |
|----------|---------|----------|--------|
| **Lint** (`lint.yml`) | `ubuntu-24.04` | No | Generated tokens are current, token generator tests, SPDX headers, clang-format 20.1.8, JSON validity |
| **Build** (`build.yml`) | Windows (`windows-2025` or self-hosted), Linux (`ubuntu-24.04` or self-hosted) | Yes (commercial) | Configure (`ci-windows`/`ci-linux`), build, `all_qmllint`, `ctest -LE gpu`; Windows uploads an installable artifact |

Both run on pushes to `main`, on pull requests and on demand.
Pull-request runs cancel superseded runs.

## Providing Qt to CI

QTOS uses the commercial Qt 6.12 kit, which CI cannot download
anonymously. Configure **one** of the following in
*Settings → Secrets and variables → Actions*.

### Option A: self-hosted runner (recommended)

Hosted Windows minutes are billed at 2× on private repositories, and a cold
Qt install takes several minutes. A self-hosted runner on a machine that
already has Qt is faster, free and has a real GPU.

1. *Settings → Actions → Runners → New self-hosted runner*. Install it on the
   workstation (Windows) and give it the labels `self-hosted, windows, qt`.
2. Repository variables:
   - `QTOS_WINDOWS_RUNNER` = `["self-hosted","windows","qt"]`
   - `QTOS_QT_DIR` = `C:/Qt/6.12.0/msvc2022_64`
3. Optional: a Linux runner with `QTOS_LINUX_RUNNER` and a Linux kit path.
   Note that `QTOS_QT_DIR` applies to both jobs, so use separate runners per
   OS with matching paths, or set the variable per environment.

With a GPU runner you can also drop `-LE gpu` to run the Kit QML tests.

### Option B: hosted runners with the Qt Online Installer

Add repository **secrets**:

- `QT_INSTALLER_JWT_TOKEN`: a Qt Account JWT for unattended installs (preferred), **or**
- `QT_INSTALLER_EMAIL` and `QT_INSTALLER_PASSWORD`: the Qt Account login.

`tools/ci/install_qt.py` downloads the official online installer, installs
the package IDs in [`.github/qt-packages.json`](../.github/qt-packages.json)
in unattended mode, and caches the result with `actions/cache` (keyed on OS,
Qt version and the package list). Credentials are passed through the
environment and are redacted from logs.

To change modules, edit `qt-packages.json`. List available IDs with the
installer's `search` command, for example
`qt-online-installer-windows-x64-online.exe --root C:\tmp search qt.qt6.6120`.

Without either option, the Build workflow's preflight job succeeds and writes
a summary explaining how to enable Qt builds, so pull requests are not
blocked.

## Branch protection

[`.github/rulesets/main.json`](../.github/rulesets/main.json) defines the
`main` ruleset:

- no deletion and no force-push
- linear history
- pull requests required, with stale approvals dismissed and conversations resolved
- the Lint job required
- repository admins can bypass

Apply it with `python tools/github/bootstrap.py --ruleset`. GitHub only
enforces rulesets on **private** repositories on the Team or Enterprise
plans (or on public repositories). On the Free plan the command reports that
the ruleset could not be applied.

## Dependabot

`.github/dependabot.yml` updates GitHub Actions weekly (grouped) and the
pinned Python tooling monthly.

## Local parity

Everything CI runs can be run locally:

```powershell
python tools/tokens/generate_tokens.py --check
python -m unittest discover -s tests/tools -v
python tools/ci/check_headers.py
pre-commit run --all-files
.\tools\dev\build.ps1 -Preset ci-windows -Test
.\tools\dev\build.ps1 -Preset ci-windows -Target all_qmllint
```
