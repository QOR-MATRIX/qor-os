# Continuous integration

Workflows live in [`.github/workflows/`](../.github/workflows/).

| Workflow | Runs on | Needs Qt | Checks |
|----------|---------|----------|--------|
| **Lint** (`lint.yml`) | `ubuntu-24.04` | No | Generated tokens are current, token generator tests, the Store's signed catalogue, SPDX headers, clang-format 20.1.8, JSON validity. Carried code ([UPSTREAM.md](UPSTREAM.md)) is skipped by the header and format checks |
| **Build** (`build.yml`) | Windows and Linux, self-hosted (or hosted with the online installer) | Yes (commercial) | Configure (`ci-windows`/`ci-linux`, warnings as errors, the QOR core required), build, `all_qmllint`, `ctest -LE gpu` (plus `-L gpu` when enabled). On both, the carried products (QQ, ARQADE, QOR Media) when they changed. Tags and manual runs also upload an installable Windows artifact (kept 7 days) |

Both run on pushes to `main`, on pull requests and on demand; Build also runs on `v*` tags.
Pull-request runs cancel superseded runs. Each OS job runs only when that OS has a Qt source.

## What the Build job needs on a runner

- **The commercial Qt 6.12 kit** (below).
- **Rust 1.91 or later** (`cargo`) for the QOR core, which `core/` builds from
  demiurge-chain at a pinned commit through Corrosion
  ([QOR-0003](decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)). The CI
  presets set `QOR_BUILD_CORE=ON`, so a runner without Rust fails at configure
  instead of skipping the core. The workstation's runners use Rust 1.98.1.
  Linux also needs `libdbus-1-dev` and `pkg-config`, because the core's
  keychain uses the Secret Service.
- **Network access to GitHub** at configure time. It fetches demiurge-chain at
  the pin, and Corrosion v0.6.1.

### The carried products

With `QOR_RUN_GPU_TESTS`, both jobs also build and test QQ, ARQADE and QOR
Media: Windows through `tools/dev/products.ps1`, which calls each product's
own script, and Linux through `tools/dev/products.sh`, QOR OS's Linux build of
them. On Linux the suites run on a display of the script's own, a compositor
on Qt's offscreen platform, so nothing reaches the runner's desktop
([BUILDING.md](BUILDING.md#on-linux)); under WSL the products draw on the GPU
through `GALLIUM_DRIVER=d3d12`. QOR Media needs PDFium, which both scripts
fetch at a pinned release and check by SHA-256.

**Linux runs them only with `QOR_RUN_LINUX_PRODUCTS` as well**, which is not
set. On 28 October a run of them under WSL froze the workstation the Linux
runner shares, during QQ's Player suite, minutes after Mesa's Direct3D 12
driver failed to make a context (`EGL_BAD_ALLOC`) in QQ's play suite; it took
the power button. Windows logged no GPU reset. Earlier that day the same
suite had finished five times under WSL, in three runs. Set the variable on a
runner whose machine can take that, or once the cause is known.

The suites take about ten minutes, and on Windows they drive real windows on
the runner's desktop, so they run only when:

- the push or pull request changes `products/`, `tools/qor-media/`,
  `docs/design/tokens.json`, `tools/dev/products.ps1` (`products.sh`,
  `products-display.qml` and `.github/qt-packages.json` on Linux) or
  `build.yml`;
- the run is for a tag;
- the run was started by hand.

A product whose suites fail is run once more (on Linux, its failed suites).
On a Windows desktop someone is using, a test window can lose the keyboard to
another application, and a genuine failure fails both attempts. The step
summary says whether the products ran.

## Providing Qt to CI

QOR OS uses the commercial Qt 6.12 kit, which CI cannot download
anonymously. Configure **one** of the following in
*Settings → Secrets and variables → Actions*.

### Option A: self-hosted runners (current)

Hosted Windows minutes are billed at 2× on private repositories, and a cold
Qt install takes several minutes. A self-hosted runner on a machine that
already has Qt is faster, free, builds incrementally and has a real GPU.

**Current setup.** The owner's workstation (the reference laptop) runs two
runners:

| Runner | Where | Labels |
|--------|-------|--------|
| `<COMPUTERNAME>-qtos` | `%USERPROFILE%\ghr\qtos`, work folder `w` | `self-hosted, Windows, X64, qt, msvc, gpu` |
| `<COMPUTERNAME>-qtos-linux` | WSL2 Ubuntu, `~/ghr/qtos-linux` | `self-hosted, Linux, X64, qt, gcc, wsl, gpu` |

- Both run as the signed-in user, so no admin rights are needed.
- A full Windows build of the OS with its tests takes about 2½ minutes once
  incremental. The first build with the QOR core compiles its Rust
  dependencies, which takes several more minutes.

### Runners for the organisation

A runner serves either one repository or the organisation. Both runners are
registered to the organisation (`QOR-MATRIX`, since 27 October 2026), in its
Default runner group, so the same two serve QOR OS and the frozen QTOS. The
Default group does not allow public repositories, so the public
demiurge-chain never runs on this workstation, and it must stay that way: a
self-hosted runner on a public repository runs code from anyone's pull
request.

`tools/ci/runners.ps1` moves both runners in one command:

1. It leaves a runner already registered at the scope asked for as it is, so
   running it twice is harmless.
2. It refuses to touch a runner GitHub reports busy. Stopping one mid-job
   orphans the job, and GitHub then refuses to remove the runner until the
   job times out.
3. It removes each other runner from where it is registered now, and
   registers it at the new scope with the same name and labels.
4. It pins the Linux runner's PATH (with `~/.cargo/bin`) and display
   variables.
5. It starts both, minimised.

Tokens are piped from `gh`, never printed.

```powershell
gh auth refresh -h github.com -s admin:org   # once: organisation runners need the admin:org scope
.\tools\ci\runners.ps1 -Scope org            # both runners, for every repository in QOR-MATRIX
.\tools\ci\runners.ps1 -Scope repo -Repo qor-os   # or one repository, with the repo scope only
.\tools\ci\runners.ps1 -StartOnly            # just start them (after a reboot, without Startup entries)
```

QTOS is frozen, so its CI runs only if someone pushes to it.

Repository variables (*Settings → Secrets and variables → Actions → Variables*,
or `gh variable set <NAME> --body <VALUE> --repo QOR-MATRIX/qor-os`). The
private repositories of a Free organisation cannot read organisation
variables, so each repository sets its own:

| Variable | Value | Effect |
|----------|-------|--------|
| `QOR_WINDOWS_RUNNER` | `["self-hosted","windows","qt"]` | Windows job runs on the workstation |
| `QOR_WINDOWS_QT_DIR` | `C:/Qt/6.12.0/msvc2022_64` | Use the installed kit; skip the installer |
| `QOR_RUN_GPU_TESTS` | `true` (optional) | Also run GPU-labelled suites and the carried products (opens brief test windows on the runner's desktop) |
| `QOR_RUN_LINUX_PRODUCTS` | `true` (optional, unset) | With `QOR_RUN_GPU_TESTS`, also run the carried products on Linux ([why it is off](#the-carried-products)) |
| `QOR_LINUX_RUNNER` | `["self-hosted","linux","qt"]` | Linux job runs in WSL on the workstation |
| `QOR_LINUX_QT_DIR` | `/home/<user>/Qt/6.12.0/gcc_64` | Use the installed kit |

The variables took their `QOR_` names on 27 October 2026
([QOR-0009](decisions/QOR-0009-the-name-is-qor-os.md)); the frozen QTOS keeps its own `QTOS_` set. Until they are set,
a job is skipped; once they are set, jobs wait in the queue whenever the runner
is not running.

**Starting the runners at sign-in** is a Startup-folder entry for each, which
the owner adds once. Assistants do not write Startup entries.

```powershell
Set-Content "$([Environment]::GetFolderPath('Startup'))\qor-github-runner.cmd" `
  "@start `"QOR OS Windows runner`" /min cmd /c `"%USERPROFILE%\ghr\qtos\run.cmd`""
Set-Content "$([Environment]::GetFolderPath('Startup'))\qor-github-runner-linux.cmd" `
  "@start `"QOR OS Linux runner`" /min wsl.exe -d Ubuntu --cd ~ --exec /home/$($env:USERNAME.ToLower())/ghr/qtos-linux/run.sh"
```

If CI does not run, first check that both runners are online:
`gh api repos/QOR-MATRIX/qor-os/actions/runners` (or
`orgs/QOR-MATRIX/actions/runners` once they belong to the organisation).

#### Setting up a runner from scratch

```powershell
# Windows, in an empty folder holding the extracted actions-runner-win-x64 zip:
$token = gh api -X POST repos/QOR-MATRIX/qor-os/actions/runners/registration-token --jq .token
.\config.cmd --unattended --url https://github.com/QOR-MATRIX/qor-os --token $token `
  --name "$env:COMPUTERNAME-qtos" --labels qt,msvc,gpu --work w --replace
.\run.cmd
```

For the organisation, use `orgs/QOR-MATRIX/actions/runners/registration-token`
and `--url https://github.com/QOR-MATRIX`.

#### Linux runner in WSL2

The Linux job runs inside WSL2 Ubuntu, using the commercial kit in `~/Qt`
([BUILDING.md](BUILDING.md#linux-on-wsl2-windows-workstation)) and Rust from
rustup in `~/.cargo` (1.98.1).

With `QOR_RUN_GPU_TESTS`, the Linux job also runs two things on WSLg's
display: the GPU suites, and `tools/dev/prism-protocol-check.sh`, which
connects real clients to the built compositor.

Two runner files are pinned rather than captured from the shell that
configured the runner. `tools/ci/runners.ps1` writes both:

- **`.path`** holds a Linux-only PATH, starting with `~/.cargo/bin`. WSL
  appends the whole Windows PATH, so a job could otherwise run a Windows
  `.exe` for a tool missing on Linux.
- **`.env`** holds WSLg's display variables. `WAYLAND_DISPLAY` is the absolute
  path of WSLg's socket, `/mnt/wslg/runtime-dir/wayland-0`. With systemd
  enabled, `/run/user/<uid>` is recreated whenever the distribution starts,
  and WSLg's socket is not always linked into it, so a relative `wayland-0`
  can point at nothing and GPU tests would wait for a window forever. The
  GPU step checks for the socket and fails with this pointer instead.

GPU suites run with a 120-second timeout and one retry
(`ctest --repeat until-pass:2`). Under WSLg a window is occasionally never
reported as exposed. A genuine failure still fails both attempts.

```bash
# Set up (in WSL). Rust and the core's system package first:
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.98.1
sudo apt-get install libdbus-1-dev pkg-config
# Download the actions-runner-linux-x64 tarball, check its SHA-256, extract it to ~/ghr/qtos-linux, then:
./config.sh --unattended --url https://github.com/QOR-MATRIX/qor-os --token <registration token> \
  --name "$(hostname)-qor-linux" --labels qt,gcc,wsl,gpu --work _work --replace
printf '%s\n' "$HOME/.cargo/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/lib/wsl/lib" > .path
printf '%s\n' LANG=C.UTF-8 DISPLAY=:0 WAYLAND_DISPLAY=/mnt/wslg/runtime-dir/wayland-0 XDG_RUNTIME_DIR=/run/user/$(id -u) > .env
```

WSL keeps the distribution running while the runner's `wsl.exe` process is
alive, so start the runner from Windows (`tools/ci/runners.ps1 -StartOnly`, or
the Startup entry above).

**WSL's clock drifts.** After the laptop sleeps, WSL's clock can be hours away
from Windows'. It was 6 h 25 min ahead on 26 October 2026. The runner's session
is a signed token with a time window, so GitHub refuses it, and the runner logs
"The runner registration has been deleted from the server" and exits, even
though the registration exists. `tools/ci/runners.ps1` sets WSL's clock from
Windows before it starts the runners. To fix it by hand:
`wsl -u root date -u -s "<Windows' UTC time>"`, then start the runner again.

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
environment and are redacted from logs. Hosted images include Rust.

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
python tools/store/build_store.py check
python tools/ci/check_headers.py
.\.venv\Scripts\pre-commit run --all-files
.\tools\dev\build.ps1 -Preset ci-windows -Test
.\tools\dev\build.ps1 -Preset ci-windows -Target all_qmllint
.\tools\dev\products.ps1
```
