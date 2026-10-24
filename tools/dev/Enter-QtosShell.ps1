<#
.SYNOPSIS
    Loads the MSVC x64 toolchain and the QTOS Qt kit into the current PowerShell session.

.DESCRIPTION
    Finds Visual Studio 2022 (or Build Tools) with vswhere, imports the environment
    produced by vcvars64.bat, prepends Qt's bin directory and Qt's bundled CMake/Ninja
    to PATH, and sets QTOS_QT_DIR. Dot-source it so the variables persist:

        . .\tools\dev\Enter-QtosShell.ps1

.PARAMETER QtDir
    Qt kit directory. Defaults to $env:QTOS_QT_DIR, then C:\Qt\6.12.0\msvc2022_64.
#>
[CmdletBinding()]
param(
    [string] $QtDir = $(if ($env:QTOS_QT_DIR) { $env:QTOS_QT_DIR } else { 'C:\Qt\6.12.0\msvc2022_64' })
)

$ErrorActionPreference = 'Stop'

if ($env:QTOS_DEV_SHELL -eq '1') {
    Write-Verbose 'QTOS developer shell already loaded.'
    return
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'vswhere.exe not found. Install Visual Studio 2022 or the Build Tools.' }

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'No Visual Studio installation with the x64 C++ toolset was found.' }

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
$env:PATH = "$(Split-Path $vswhere);$env:PATH"   # vcvars64.bat calls vswhere itself
cmd /c "`"$vcvars`" >nul && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "Env:$($Matches[1])" -Value $Matches[2] }
}

if (-not (Test-Path (Join-Path $QtDir 'bin\qmake.exe'))) { throw "Qt kit not found at $QtDir" }
$qtRoot = (Resolve-Path (Join-Path $QtDir '..\..')).Path
$env:QTOS_QT_DIR = $QtDir
$env:PATH = @(
    (Join-Path $QtDir 'bin'),
    (Join-Path $qtRoot 'Tools\CMake_64\bin'),
    (Join-Path $qtRoot 'Tools\Ninja'),
    $env:PATH
) -join ';'
$env:QTOS_DEV_SHELL = '1'

Write-Host "QTOS developer shell ready  ·  MSVC: $vsPath  ·  Qt: $QtDir" -ForegroundColor Cyan
