<#
.SYNOPSIS
    Configures, builds and optionally tests QOR OS with a CMake preset.

.EXAMPLE
    .\tools\dev\build.ps1                      # windows-release
    .\tools\dev\build.ps1 -Preset windows-debug -Test
    .\tools\dev\build.ps1 -Target all_qmllint
#>
[CmdletBinding()]
param(
    [string] $Preset = 'windows-release',
    [string] $Target,
    [switch] $Test,
    [switch] $Fresh
)

$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
. (Join-Path $PSScriptRoot 'Enter-QorShell.ps1')

Push-Location $repo
try {
    $configure = @('--preset', $Preset)
    if ($Fresh) { $configure += '--fresh' }
    cmake @configure
    if ($LASTEXITCODE) { throw "Configure failed ($LASTEXITCODE)" }

    $build = @('--build', '--preset', $Preset)
    if ($Target) { $build += @('--target', $Target) }
    cmake @build
    if ($LASTEXITCODE) { throw "Build failed ($LASTEXITCODE)" }

    if ($Test) {
        ctest --preset $Preset
        if ($LASTEXITCODE) { throw "Tests failed ($LASTEXITCODE)" }
    }
}
finally {
    Pop-Location
}
