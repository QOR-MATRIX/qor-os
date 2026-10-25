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
installed in `%USERPROFILE%\ghr\qtos`.

- `QTOS_WINDOWS_RUNNER`, `QTOS_WINDOWS_QT_DIR` and `QTOS_RUN_GPU_TESTS` are set.
- A full Windows build with tests takes about 2½ minutes.
- The runner runs as the signed-in user, so no admin rights are needed.
- It starts minimised at every sign-in through a Startup-folder entry. To set
  it up on another machine, or to start it by hand:

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

#### Linux runner in WSL2

The same workstation also runs the Linux job inside WSL2 Ubuntu, using the
commercial kit in `~/Qt` ([BUILDING.md](BUILDING.md#linux-on-wsl2-windows-workstation)).
The runner is `<COMPUTERNAME>-qtos-linux`, with labels
`self-hosted, Linux, X64, qt, gcc, wsl, gpu`, installed in `~/ghr/qtos-linux`.

| Variable | Value |
|----------|-------|
| `QTOS_LINUX_RUNNER` | `["self-hosted","linux","qt"]` |
| `QTOS_LINUX_QT_DIR` | `/home/<user>/Qt/6.12.0/gcc_64` |

With `QTOS_RUN_GPU_TESTS`, the Linux job also runs two things on WSLg's
display: the GPU suites, and `tools/dev/prism-protocol-check.sh`, which
connects real clients to the built compositor.

Two runner files are pinned rather than captured from the shell that
configured the runner:

- **`.path`** holds a Linux-only PATH. WSL appends the whole Windows PATH, so
  a job could otherwise run a Windows `.exe` for a tool missing on Linux.
- **`.env`** holds WSLg's display variables. `WAYLAND_DISPLAY` is the absolute
  path of WSLg's socket, `/mnt/wslg/runtime-dir/wayland-0`. With systemd
  enabled, `/run/user/<uid>` is recreated whenever the distribution starts,
  and WSLg's socket is not always linked into it, so a relative `wayland-0`
  can point at nothing and GPU tests would wait for a window forever. The
  GPU step checks for the socket and fails with this pointer instead.

GPU suites run with a 120-second timeout and one retry
(`ctest --repeat until-pass:2`). Under WSLg a window is occasionally never
reported as exposed. It happened once, right after a full rebuild, and could
not be reproduced in 24 runs. A genuine failure still fails both attempts.

```bash
# Set up (in WSL). Download the actions-runner-linux-x64 tarball, check its SHA-256, extract it to ~/ghr/qtos-linux.
./config.sh --unattended --url https://github.com/QOR-MATRIX/QTOS --token <registration token> \
  --name "$(hostname)-qtos-linux" --labels qt,gcc,wsl,gpu --work _work --replace
printf '%s\n' /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/lib/wsl/lib > .path
printf '%s\n' LANG=C.UTF-8 DISPLAY=:0 WAYLAND_DISPLAY=/mnt/wslg/runtime-dir/wayland-0 XDG_RUNTIME_DIR=/run/user/$(id -u) > .env
```

WSL keeps the distribution running while the runner's `wsl.exe` process is
alive, so start the runner from Windows:

```powershell
# For this session:
Start-Process wsl.exe '-d','Ubuntu','--cd','~','--exec',"/home/$env:USERNAME/ghr/qtos-linux/run.sh" -WindowStyle Minimized
# At every sign-in (Startup folder, next to the Windows runner's entry):
Set-Content "$([Environment]::GetFolderPath('Startup'))\qtos-github-runner-linux.cmd" `
  "@start `"QTOS Linux runner`" /min wsl.exe -d Ubuntu --cd ~ --exec /home/$($env:USERNAME.ToLower())/ghr/qtos-linux/run.sh"
```

### Option B: hosted runners with the Qt Online Installer

Add repository **secrets**:

- `QT_INSTALLER_JWT_TOKEN`: a Qt Account JWT for unattended installs (preferred), **or**
- `QT_INSTALLER_EMAIL` and `QT_INSTALLER_PASSWORD`: the Qt Account login.

`tools/ci/install_qt.py` downloads the official online installer and installs
the kit and add-ons in [`.github/qt-packages.json`](../.github/qt-packages.json)
in unattended mode. It then checks that the CMake packages listed under
`cmake` exist, because the installer exits successfully even when it skips a
package it cannot resolve. The result is cached with `actions/cache` (keyed on
OS, Qt version and the package list). Credentials are passed through the
environment and are redacted from logs.

The same script sets up a workstation or a self-hosted runner. Without
credential variables it uses an existing Qt Account sign-in (`qtaccount.ini`,
written by any interactive Qt installer run), and it adds packages to an
existing installation through that installation's MaintenanceTool:

```bash
python3 tools/ci/install_qt.py --platform linux --root ~/Qt   # e.g. WSL2 Ubuntu
```

To change modules, edit `qt-packages.json`. Since Qt 6.8, add-on IDs are
architecture-independent (`qt.qt6.6120.addons.qtgraphs`); the per-kit IDs are
virtual and cannot be selected. List available IDs with the installer's
`search` command, for example
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
