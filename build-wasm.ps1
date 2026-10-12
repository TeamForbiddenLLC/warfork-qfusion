# Build the Warfork WebAssembly client into web/wasm/
param(
    [switch]$Clean,
    [switch]$Configure,
    [string]$Emsdk = ""
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build-wasm"
$SourceDir = Join-Path $Root "source"

$neededExtern = @("zlib", "ogg", "freetype", "vorbis", "tracy", "stb", "glslang")
$missingExtern = @($neededExtern | Where-Object {
        $dir = Join-Path $SourceDir "extern\$_"
        -not (Test-Path (Join-Path $dir "CMakeLists.txt")) -and
        -not (Test-Path (Join-Path $dir "stb_image.h")) -and
        -not (Test-Path (Join-Path $dir "known_good.json"))
    })
if ($missingExtern.Count -gt 0) {
    Write-Host "Sous-modules manquants: $($missingExtern -join ', ')"
    git -C $Root submodule update --init --depth 1 source/extern/zlib source/extern/ogg source/extern/freetype source/extern/vorbis source/extern/tracy source/extern/stb source/extern/glslang
    if ($LASTEXITCODE -ne 0) {
        throw "git submodule update a echoue. Depuis la racine: git submodule update --init"
    }
}

function Find-Emsdk {
    if ($Emsdk -and (Test-Path $Emsdk)) {
        return (Resolve-Path $Emsdk).Path
    }
    foreach ($candidate in @(
            $env:EMSDK,
            "D:\emsdk",
            (Join-Path $env:USERPROFILE "emsdk"),
            (Join-Path $Root "..\emsdk")
        )) {
        if ($candidate -and (Test-Path (Join-Path $candidate "emsdk_env.ps1"))) {
            return (Resolve-Path $candidate).Path
        }
    }
    throw "Emscripten SDK introuvable. Installez emsdk ou passez -Emsdk <chemin>."
}

$emsdkRoot = Find-Emsdk
Write-Host "EMSDK: $emsdkRoot"
. (Join-Path $emsdkRoot "emsdk_env.ps1")
$env:CMAKE_POLICY_VERSION_MINIMUM = "3.5"

if (-not (Get-Command emcmake -ErrorAction SilentlyContinue)) {
    throw "emcmake n'est pas dans le PATH apres emsdk_env.ps1"
}

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Nettoyage $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

$needConfigure = $Configure -or -not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))
if ($needConfigure) {
    Write-Host "Configuration CMake (Release)..."
    emcmake cmake -S $SourceDir -B $BuildDir -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw "emcmake cmake a echoue ($LASTEXITCODE)"
    }
}

Write-Host "Compilation cible warfork..."
cmake --build $BuildDir --parallel --target warfork --config Release
if ($LASTEXITCODE -ne 0) {
    throw "cmake --build a echoue ($LASTEXITCODE)"
}

$wasmOut = Join-Path $Root "web\wasm"
Write-Host "OK: artefacts dans $wasmOut"
Get-ChildItem $wasmOut -File | ForEach-Object { Write-Host ("  {0}  {1:N1} Mo" -f $_.Name, ($_.Length / 1MB)) }
