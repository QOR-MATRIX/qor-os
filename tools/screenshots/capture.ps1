<#
.SYNOPSIS
    Captures the canonical QTOS screenshots used in the docs (docs/images).

.DESCRIPTION
    Runs qtos-shell once per scene with deterministic options (ephemeral
    preferences, fixed size) and saves each frame with QQuickWindow::grabWindow.

.EXAMPLE
    .\tools\screenshots\capture.ps1
    .\tools\screenshots\capture.ps1 -OutDir C:\temp\shots -Scenes desktop,beam
#>
[CmdletBinding()]
param(
    [string]   $Preset = 'windows-release',
    [string]   $OutDir,
    [string[]] $Scenes,
    [string]   $Size = '1600x1000',
    [ValidateSet('webp', 'png')] [string] $Format = 'webp'
)

$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
if (-not $OutDir) { $OutDir = Join-Path $repo 'docs\images' }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
. (Join-Path $repo 'tools\dev\Enter-QtosShell.ps1')

$exe = Join-Path $repo "build\$Preset\qtos-shell.exe"
if (-not (Test-Path $exe)) { throw "qtos-shell not built for preset '$Preset'." }

$catalog = [ordered]@{
    'lock'           = @('--delay', '2500')
    'desktop'        = @('--unlocked', '--open', 'files,music', '--delay', '4200')
    'beam'           = @('--unlocked', '--open', 'music', '--surface', 'beam', '--query', 'se', '--delay', '3600')
    'control-center' = @('--unlocked', '--open', 'settings', '--surface', 'control-center', '--delay', '3600')
    'overview'       = @('--unlocked', '--open', 'files,music,notes,terminal,monitor', '--surface', 'overview', '--delay', '4600')
    'light'          = @('--unlocked', '--open', 'notes,settings', '--theme', 'light', '--accent', '205', '--delay', '4200')
    'apps'           = @('--unlocked', '--open', 'terminal,monitor', '--delay', '4600')
    'sound'          = @('--unlocked', '--play', '--seek', '64', '--open', 'music,settings/sound/reactive', '--delay', '5600')
}

$selected = if ($Scenes) { $Scenes } else { $catalog.Keys }
foreach ($scene in $selected) {
    if (-not $catalog.Contains($scene)) { throw "Unknown scene '$scene'. Known: $($catalog.Keys -join ', ')" }
    $file = Join-Path $OutDir "$scene.$Format"
    $shellArgs = @('--ephemeral', '--size', $Size, '--screenshot', $file) + $catalog[$scene]
    $p = Start-Process -FilePath $exe -ArgumentList $shellArgs -PassThru -Wait
    if ($p.ExitCode -ne 0) { throw "Scene '$scene' failed with exit code $($p.ExitCode)" }
    Write-Host "Captured $scene -> $file"
}
