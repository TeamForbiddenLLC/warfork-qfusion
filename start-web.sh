#!/usr/bin/env bash
# Start the Warfork web client + UDP/HTTP proxy (http://localhost:8081)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
SKIP_INSTALL=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-install) SKIP_INSTALL=1; shift ;;
    -h|--help)
      echo "Usage: $0 [--skip-install]"
      exit 0
      ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if ! command -v node >/dev/null 2>&1; then
  echo "Node.js 18+ is required (node not in PATH)." >&2
  exit 1
fi

if [[ ! -f "$ROOT/.env" && -f "$ROOT/.env.example" ]]; then
  cp "$ROOT/.env.example" "$ROOT/.env"
  echo "Created .env from .env.example"
fi

if [[ "$SKIP_INSTALL" -eq 0 || ! -d "$ROOT/node_modules/ws" ]]; then
  echo "npm install..."
  npm install
fi

if [[ ! -f "$ROOT/web/wasm/warfork.wasm" ]]; then
  echo "Warning: web/wasm/warfork.wasm is missing. Run ./build-wasm.sh first."
fi

if [[ ! -d "$ROOT/web/game/basewf" ]]; then
  echo "Warning: web/game/basewf is missing. Place licensed pk3/cfg files there."
fi

echo "Starting web server..."
npm run web
