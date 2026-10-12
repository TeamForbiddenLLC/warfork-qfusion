# Start the Warfork web client + UDP/HTTP proxy (http://localhost:8081)
param(
    [switch]$SkipInstall
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

if (-not (Get-Command node -ErrorAction SilentlyContinue)) {
    throw "Node.js 18+ is required (node not in PATH)."
}

if (-not (Test-Path (Join-Path $Root ".env")) -and (Test-Path (Join-Path $Root ".env.example"))) {
    Copy-Item (Join-Path $Root ".env.example") (Join-Path $Root ".env")
    Write-Host "Created .env from .env.example"
}

$wsPkg = Join-Path $Root "node_modules\ws"
if (-not $SkipInstall -or -not (Test-Path $wsPkg)) {
    Write-Host "npm install..."
    npm install
    if ($LASTEXITCODE -ne 0) {
        throw "npm install failed ($LASTEXITCODE)"
    }
}

$wasm = Join-Path $Root "web\wasm\warfork.wasm"
if (-not (Test-Path $wasm)) {
    Write-Host "Warning: web/wasm/warfork.wasm is missing. Run .\build-wasm.ps1 first."
}

$game = Join-Path $Root "web\game\basewf"
if (-not (Test-Path $game)) {
    Write-Host "Warning: web/game/basewf is missing. Place licensed pk3/cfg files there."
}

Write-Host "Starting web server..."
npm run web
