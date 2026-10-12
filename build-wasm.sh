#!/usr/bin/env bash
# Build the Warfork WebAssembly client into web/wasm/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build-wasm"
SOURCE_DIR="$ROOT/source"
CLEAN=0
CONFIGURE=0
EMSDK_ROOT="${EMSDK:-}"

usage() {
  echo "Usage: $0 [--clean] [--configure] [--emsdk <path>]"
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean) CLEAN=1; shift ;;
    --configure) CONFIGURE=1; shift ;;
    --emsdk)
      [[ $# -ge 2 ]] || usage
      EMSDK_ROOT="$2"
      shift 2
      ;;
    -h|--help) usage ;;
    *) echo "Option inconnue: $1"; usage ;;
  esac
done

find_emsdk() {
  local candidate
  for candidate in \
    "$EMSDK_ROOT" \
    "${EMSDK:-}" \
    "$HOME/emsdk" \
    "$ROOT/../emsdk" \
    "/opt/emsdk"
  do
    if [[ -n "$candidate" && -f "$candidate/emsdk_env.sh" ]]; then
      (cd "$candidate" && pwd)
      return 0
    fi
  done
  echo "Emscripten SDK introuvable. Installez emsdk ou passez --emsdk <chemin>." >&2
  exit 1
}

EMSDK_ROOT="$(find_emsdk)"
echo "EMSDK: $EMSDK_ROOT"
# shellcheck disable=SC1091
source "$EMSDK_ROOT/emsdk_env.sh"
export CMAKE_POLICY_VERSION_MINIMUM=3.5

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake n'est pas dans le PATH apres emsdk_env.sh" >&2
  exit 1
fi

if [[ "$CLEAN" -eq 1 && -d "$BUILD_DIR" ]]; then
  echo "Nettoyage $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

if [[ "$CONFIGURE" -eq 1 || ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "Configuration CMake (Release)..."
  emcmake cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

echo "Compilation cible warfork..."
cmake --build "$BUILD_DIR" --parallel --target warfork --config Release

WASM_OUT="$ROOT/web/wasm"
echo "OK: artefacts dans $WASM_OUT"
ls -lh "$WASM_OUT"/warfork.js "$WASM_OUT"/warfork.wasm "$WASM_OUT"/warfork.data 2>/dev/null || ls -lh "$WASM_OUT"
