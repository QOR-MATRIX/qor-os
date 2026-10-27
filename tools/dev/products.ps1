<#
.SYNOPSIS
    Builds and tests the products QOR OS carries from demiurge-chain: QQ, ARQADE and QOR Media.

.DESCRIPTION
    Each product keeps its own build script upstream (products/qq/build.ps1, products/arqade/qt/build.ps1,
    tools/qor-media/build.ps1). This calls each one with the Qt kit and a build folder under build/products/, so the
    carried code builds exactly as it does in demiurge-chain (docs/UPSTREAM.md). Each script configures, builds and
    runs every Qt Test suite, and fails if any suite fails.

    The suites open real windows on the GPU and drive them with the keyboard and mouse, so run this in a desktop
    session. On a desktop someone is using, a window can lose the keyboard to another application mid-test (QOR
    Media's reader suite is the most exposed). A product whose suites fail is therefore run once more, as the OS's own
    GPU suites are in CI (ctest --repeat until-pass:2), and the retry is reported. A genuine failure fails both attempts.

.EXAMPLE
    .\tools\dev\products.ps1                       # all three
    .\tools\dev\products.ps1 -Product qq,arqade
    .\tools\dev\products.ps1 -Qt D:\Qt\6.12.0\msvc2022_64 -Attempts 1
#>
[CmdletBinding()]
param(
    [ValidateSet('qq', 'arqade', 'media')]
    [string[]] $Product = @('qq', 'arqade', 'media'),
    [string] $Qt = $(if ($env:QTOS_QT_DIR) { $env:QTOS_QT_DIR } else { 'C:\Qt\6.12.0\msvc2022_64' }),
    [string] $BuildRoot,
    [ValidateRange(1, 3)]
    [int] $Attempts = 2
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $BuildRoot) { $BuildRoot = Join-Path $repo 'build\products' }

$scripts = [ordered]@{
    qq     = 'products\qq\build.ps1'
    arqade = 'products\arqade\qt\build.ps1'
    media  = 'tools\qor-media\build.ps1'
}

$failed = @()
$retried = @()
foreach ($name in $scripts.Keys) {
    if ($Product -notcontains $name) { continue }
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        Write-Host "== $name$(if ($attempt -gt 1) { " (attempt $attempt of $Attempts)" })" -ForegroundColor Cyan
        # Each script throws when its build or a suite fails; carry on so one run reports every product.
        try {
            & (Join-Path $repo $scripts[$name]) -Qt $Qt -Build (Join-Path $BuildRoot $name)
            if ($attempt -gt 1) { $retried += $name }
            break
        }
        catch {
            Write-Host $_ -ForegroundColor Red
            if ($attempt -eq $Attempts) { $failed += $name }
        }
    }
}
if ($retried) { Write-Host "Passed on a second attempt: $($retried -join ', ')" -ForegroundColor Yellow }
if ($failed) { throw "Failed: $($failed -join ', ')" }
Write-Host "Products built and tested: $($Product -join ', ')" -ForegroundColor Green
