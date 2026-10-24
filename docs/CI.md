# Continuous integration

Workflows live in [`.github/workflows/`](../.github/workflows/).

| Workflow | Runs on | Needs Qt | Checks |
|----------|---------|----------|--------|
| **Lint** (`lint.yml`) | `ubuntu-24.04` | No | Generated tokens are current, token generator tests, SPDX headers, clang-format 20.1.8, JSON validity |
| **Build** (`build.yml`) | Windows (`windows-2025` or self-hosted), Linux (`ubuntu-24.04` or self-hosted) | Yes (commercial) | Configure (`ci-windows`/`ci-linux`), build, `all_qmllint`, `ctest -LE gpu` (plus `-L gpu` when enabled). Tags and manual runs also upload an installable Windows artifact (kept 7 days) |

Both run on pushes to `main`, on pull requests and on demand; Build also runs on `v*` tags.
Pull-request runs cancel superseded runs. Each OS job runs only when that OS has a Qt source.

## Providing Qt to CI

QTOS uses the commercial Qt 6.12 kit, which CI cannot download
anonymously. Configure **one** of the following in
*Settings → Secrets and variables → Actions*.

### Option A: self-hosted runner (recommended)

Hosted Windows minutes are billed at 2× on private repositories, and a cold
Qt install takes several minutes. A self-hosted runner on a machine that
already has Qt is faster, free, builds incrementally and has a real GPU.

**Current setup:** the QTOS workstation has a repository runner registered as
`<COMPUTERNAME>-qtos` (labels `self-hosted, Windows, X64, qt, msvc, gpu`),
installed in `%USERPROFILE%\ghr\qtos`. It runs as the signed-in user, so no
admin rights are needed. It does **not** start on its own:

```powershell
# Start it for this session (keep the window open while CI runs):
& "$env:USERPROFILE\ghr\qtos\run.cmd"

# Optional: start it automatically at every logon.
Set-Content "$([Environment]::GetFolderPath('Startup'))\qtos-github-runner.cmd" `
  "@start `"QTOS GitHub runner`" /min cmd /c `"%USERPROFILE%\ghr\qtos\run.cmd`""
```

Then point the Windows job at it with the repository variables below.
Until the variables are set, the Windows job is skipped; once they are set,
Windows jobs wait in the queue whenever the runner is not running.

Repository variables (*Settings → Secrets and variables → Actions → Variables*,
or `gh variable set <NAME> --body <VALUE> --repo QOR-MATRIX/QTOS`):

| Variable | Value | Effect |
|----------|-------|--------|
| `QTOS_WINDOWS_RUNNER` | `["self-hosted","windows","qt"]` | Windows job runs on the workstation |
| `QTOS_WINDOWS_QT_DIR` | `C:/Qt/6.12.0/msvc2022_64` | Use the installed kit; skip the installer |
| `QTOS_RUN_GPU_TESTS` | `true` (optional) | Also run GPU-labelled suites (opens a brief test window on the runner's desktop) |
| `QTOS_LINUX_RUNNER`, `QTOS_LINUX_QT_DIR` | runner label JSON, kit path | Same for a Linux runner (for example in WSL2) |

To set up another runner:

```powershell
$token = gh api -X POST repos/QOR-MATRIX/QTOS/actions/runners/registration-token --jq .token
# In an empty folder containing the extracted actions-runner-win-x64 zip:
.\config.cmd --unattended --url https://github.com/QOR-MATRIX/QTOS --token $token `
  --name "$env:COMPUTERNAME-qtos" --labels qt,msvc,gpu --work w --replace
.\run.cmd
```

To remove the workstation runner: stop `run.cmd`, run
`.\config.cmd remove --token (gh api -X POST repos/QOR-MATRIX/QTOS/actions/runners/remove-token --jq .token)`
in its folder, delete the Startup entry if you created one, and delete the
`QTOS_WINDOWS_*` variables.

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
.\.venv\Scripts\pre-commit run --all-files
.\tools\dev\build.ps1 -Preset ci-windows -Test
.\tools\dev\build.ps1 -Preset ci-windows -Target all_qmllint
```
