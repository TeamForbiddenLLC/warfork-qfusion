# Windows build wrapper for Warfork.
# Run from a Developer PowerShell for VS 2022 (or any shell where cmake/msbuild are on PATH).

[CmdletBinding()]
param(
    [ValidateSet('release', 'debug')]
    [string]$Config = 'release',
    [switch]$Clean,
    [switch]$NoDeploy,
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$ExtraArgs
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (-not (Test-Path 'third-party/angelscript/sdk/angelscript/include/angelscript.h')) {
    Write-Host "==> Initialising git submodules"
    git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { throw "submodule init failed" }
}

$preset   = "workflow-windows-$Config"
$cfgName  = if ($Config -eq 'release') { 'RelWithDebInfo' } else { 'Debug' }
$buildDir = Join-Path $root 'source/build'

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "==> Cleaning $buildDir"
    Remove-Item -Recurse -Force $buildDir
}

Write-Host "==> Configuring ($preset)"
Push-Location source
try {
    & cmake -B .\build --preset $preset -G "Visual Studio 17 2022" -A x64 @ExtraArgs
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

    & cmake --build .\build --config $cfgName
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

    if (-not $NoDeploy) {
        & cmake --build .\build --target deploy --config $cfgName
        if ($LASTEXITCODE -ne 0) { throw "deploy failed" }
    }
} finally {
    Pop-Location
}

Write-Host "==> Build complete: source\build\warfork-qfusion\$cfgName"
