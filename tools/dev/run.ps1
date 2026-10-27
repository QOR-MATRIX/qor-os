<#
.SYNOPSIS
    Runs the Prism shell (qor-shell) from a build tree.

.EXAMPLE
    .\tools\dev\run.ps1                                  # lock screen, persistent prefs
    .\tools\dev\run.ps1 -- --unlocked --open files,music
    .\tools\dev\run.ps1 -Preset windows-debug -- --theme light
#>
# Named parameters only: everything after `--` goes to qor-shell, so
# `--unlocked` must not bind to -Preset by position.
[CmdletBinding(PositionalBinding = $false)]
param(
    [string] $Preset = 'windows-release',
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $ShellArgs
)

$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
. (Join-Path $PSScriptRoot 'Enter-QorShell.ps1')

$exe = Join-Path $repo "build\$Preset\qor-shell.exe"
if (-not (Test-Path $exe)) { throw "qor-shell not built for preset '$Preset'. Run tools\dev\build.ps1 first." }

& $exe @ShellArgs
exit $LASTEXITCODE
