<#
.SYNOPSIS
    Runs the Prism shell (qtos-shell) from a build tree.

.EXAMPLE
    .\tools\dev\run.ps1                                  # lock screen, persistent prefs
    .\tools\dev\run.ps1 -- --unlocked --open files,music
    .\tools\dev\run.ps1 -Preset windows-debug -- --theme light
#>
# Named parameters only: everything after `--` goes to qtos-shell, so
# `--unlocked` must not bind to -Preset by position.
[CmdletBinding(PositionalBinding = $false)]
param(
    [string] $Preset = 'windows-release',
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $ShellArgs
)

$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
. (Join-Path $PSScriptRoot 'Enter-QtosShell.ps1')

$exe = Join-Path $repo "build\$Preset\qtos-shell.exe"
if (-not (Test-Path $exe)) { throw "qtos-shell not built for preset '$Preset'. Run tools\dev\build.ps1 first." }

& $exe @ShellArgs
exit $LASTEXITCODE
