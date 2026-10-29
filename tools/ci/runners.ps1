<#
.SYNOPSIS
    Moves the workstation's two self-hosted runners (Windows and WSL Linux) to a new registration, and starts them.

.DESCRIPTION
    The workstation runs the Windows runner in %USERPROFILE%\ghr\qtos and the Linux runner in WSL at ~/ghr/qtos-linux
    (docs/CI.md). Each is registered to one place: a repository, or the organisation, where every repository the
    organisation's Default runner group allows can use it. This script removes each runner from wherever it is
    registered now and registers it again at -Scope:

        repo   one repository (-Repo, qor-os by default), with the token's repo scope. The default: on 29 October
               2026 GitHub stopped giving jobs to the organisation's runners, and moved here they took them at once.
        org    QOR-MATRIX, for every repository its Default runner group allows (needs the gh token's admin:org
               scope: gh auth refresh -h github.com -s admin:org)

    It keeps each runner's name, labels and work folder, pins the Linux runner's PATH (with ~/.cargo/bin for the QOR
    core) and display variables, and starts both, minimised. Registration and removal tokens are piped from gh to the
    runners and never printed.

    A runner already registered at -Scope is left as it is, so running the script twice is harmless. -Reregister
    removes and registers each runner again even there: for a runner GitHub shows online that takes no jobs (docs/CI.md).
    Removing a registration also deletes the settings GitHub migrated for it (.runner_migrated, .credentials_migrated),
    which a runner loads before its own. A runner GitHub reports busy is never stopped: stopping it would orphan its
    job, and GitHub then refuses to remove it until that job times out. Wait for the job, or cancel it, and run the
    script again.

    Starting the runners at sign-in is a Startup-folder entry the owner adds once (docs/CI.md); this script does not
    write one.

.EXAMPLE
    .\tools\ci\runners.ps1
    .\tools\ci\runners.ps1 -Scope org
    .\tools\ci\runners.ps1 -StartOnly
    .\tools\ci\runners.ps1 -Reregister
#>
[CmdletBinding()]
param(
    [ValidateSet('org', 'repo')]
    [string] $Scope = 'repo',
    [string] $Org = 'QOR-MATRIX',
    [string] $Repo = 'qor-os',
    [string] $Distro = 'Ubuntu',
    [switch] $StartOnly,
    [switch] $Reregister
)

$ErrorActionPreference = 'Stop'
$windowsRunner = Join-Path $env:USERPROFILE 'ghr\qtos'
$linuxRunner = 'ghr/qtos-linux'   # under the WSL user's home

function Get-Token([string] $url, [string] $kind) {
    # $url is a runner's gitHubUrl: https://github.com/OWNER or https://github.com/OWNER/REPO
    $path = ([Uri] $url).AbsolutePath.Trim('/')
    $api = if ($path.Contains('/')) { "repos/$path" } else { "orgs/$path" }
    $token = gh api -X POST "$api/actions/runners/$kind-token" --jq .token 2>$null
    if (-not $token) { throw "gh could not create a $kind token for $url. For an organisation, run: gh auth refresh -h github.com -s admin:org" }
    return $token
}

function Read-Runner([string] $json) {
    if (-not $json) { return $null }
    return ($json.TrimStart([char]0xFEFF) | ConvertFrom-Json)
}

function Assert-Idle([string] $url, [string] $name) {
    # GitHub's view, not the local processes': a job outlives a stopped listener on the server.
    $path = ([Uri] $url).AbsolutePath.Trim('/')
    $api = if ($path.Contains('/')) { "repos/$path" } else { "orgs/$path" }
    $env:QOR_RUNNER_NAME = $name
    $busy = gh api "$api/actions/runners" --paginate --jq '.runners[] | select(.name == $ENV.QOR_RUNNER_NAME) | .busy' 2>$null
    if ($busy -eq 'true') { throw "Runner '$name' is running a job at $url. Wait for it, or cancel it, then run this again." }
}

$target = if ($Scope -eq 'org') { "https://github.com/$Org" } else { "https://github.com/$Org/$Repo" }

if (-not $StartOnly) {
    # ── Windows runner ──────────────────────────────────────────────────────
    $state = Read-Runner (Get-Content (Join-Path $windowsRunner '.runner') -Raw -ErrorAction SilentlyContinue)
    $name = if ($state) { $state.agentName } else { "$env:COMPUTERNAME-qtos" }
    if ($state -and $state.gitHubUrl -eq $target -and -not $Reregister) {
        Write-Host "Windows runner '$name': already registered at $target"
    }
    else {
        if ($state) { Assert-Idle $state.gitHubUrl $name }
        Get-Process Runner.Listener, Runner.Worker -ErrorAction SilentlyContinue |
            Where-Object { $_.Path -and $_.Path.StartsWith($windowsRunner, [StringComparison]::OrdinalIgnoreCase) } |
            Stop-Process -Force
        Push-Location $windowsRunner
        try {
            if ($state) {
                Write-Host "Windows runner '$name': removing from $($state.gitHubUrl)"
                $remove = Get-Token $state.gitHubUrl 'remove'
                & .\config.cmd remove --token $remove | Out-Null
                if ($LASTEXITCODE) { throw "config.cmd remove failed ($LASTEXITCODE)" }
                Remove-Item .runner_migrated, .credentials_migrated -Force -ErrorAction SilentlyContinue
            }
            Write-Host "Windows runner '$name': registering at $target"
            $register = Get-Token $target 'registration'
            & .\config.cmd --unattended --url $target --token $register --name $name --labels qt,msvc,gpu --work w --replace | Out-Null
            if ($LASTEXITCODE) { throw "config.cmd failed ($LASTEXITCODE)" }
        }
        finally { Pop-Location }
    }

    # ── Linux runner in WSL ─────────────────────────────────────────────────
    # Fields read in bash: .runner starts with a byte-order mark, which a Windows console may mis-decode.
    # No double quote crosses wsl.exe intact, so the pattern matches each quote with ".".
    $field = { param($key) (wsl.exe -d $Distro --cd '~' --exec sed -n -E "s/.*$key.: .([^,]*).,.*/\1/p" "$linuxRunner/.runner" 2>$null | Out-String).Trim() }
    $url = & $field 'gitHubUrl'
    $name = & $field 'agentName'
    if (-not $name) { $name = "$((wsl.exe -d $Distro --exec hostname).Trim())-qor-linux" }
    if ($url -eq $target -and -not $Reregister) {
        Write-Host "Linux runner '$name': already registered at $target"
    }
    else {
        if ($url) { Assert-Idle $url $name }
        # Stop run.sh and the listener by their path. "[g]hr" matches "ghr" but not this command line itself.
        $pattern = "[$($linuxRunner[0])]$($linuxRunner.Substring(1))/(run|bin/Runner)"
        wsl.exe -d $Distro --cd '~' --exec bash -c "for p in `$(pgrep -f '$pattern'); do kill `$p; done; true"
        if ($url) {
            Write-Host "Linux runner '$name': removing from $url"
            # The token reaches Linux with Windows' line ending, which a token in an HTTP header must not carry.
            Get-Token $url 'remove' |
                wsl.exe -d $Distro --cd '~' --exec bash -c "t=`$(tr -d '\r\n'); cd $linuxRunner && ./config.sh remove --token `$t >/dev/null && rm -f .runner_migrated .credentials_migrated"
            if ($LASTEXITCODE) { throw "config.sh remove failed ($LASTEXITCODE)" }
        }
        Write-Host "Linux runner '$name': registering at $target"
        Get-Token $target 'registration' |
            wsl.exe -d $Distro --cd '~' --exec bash -c "t=`$(tr -d '\r\n'); cd $linuxRunner && ./config.sh --unattended --url $target --token `$t --name $name --labels qt,gcc,wsl,gpu --work _work --replace >/dev/null"
        if ($LASTEXITCODE) { throw "config.sh failed ($LASTEXITCODE)" }
    }
    # Pinned rather than captured: a Linux-only PATH (cargo for the QOR core), and WSLg's display (docs/CI.md).
    wsl.exe -d $Distro --cd '~' --exec bash -c "cd $linuxRunner && printf '%s\n' `$HOME/.cargo/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/lib/wsl/lib > .path && printf '%s\n' LANG=C.UTF-8 DISPLAY=:0 WAYLAND_DISPLAY=/mnt/wslg/runtime-dir/wayland-0 XDG_RUNTIME_DIR=/run/user/`$(id -u) > .env"
}

# ── Start both, minimised ───────────────────────────────────────────────────
# WSL's clock can drift hours from Windows' after sleep. The runner's session is a signed token with a time window,
# so a drifted clock makes GitHub refuse it ("the runner registration has been deleted"). Set it from Windows first.
$utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ss')
wsl.exe -d $Distro -u root --exec date -u -s $utc | Out-Null

$windowsRunning = Get-Process Runner.Listener -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($windowsRunner, [StringComparison]::OrdinalIgnoreCase) }
if (-not $windowsRunning) {
    Start-Process cmd.exe -ArgumentList '/c', "`"$windowsRunner\run.cmd`"" -WindowStyle Minimized
}
$linuxRunning = wsl.exe -d $Distro --cd '~' --exec bash -c "pgrep -f '[$($linuxRunner[0])]$($linuxRunner.Substring(1))/bin/Runner.Listener' >/dev/null && echo yes"
if (-not $linuxRunning) {
    $linuxHome = (wsl.exe -d $Distro --cd '~' --exec pwd).Trim()
    Start-Process wsl.exe -ArgumentList '-d', $Distro, '--cd', '~', '--exec', "$linuxHome/$linuxRunner/run.sh" -WindowStyle Minimized
}
Write-Host "Both runners are running. Check: gh api orgs/$Org/actions/runners (organisation) or repos/$Org/$Repo/actions/runners"
