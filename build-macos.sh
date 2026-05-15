#!/usr/bin/env bash
# macOS build wrapper for Warfork. Must be run on a macOS host (Xcode required).

set -euo pipefail

CONFIG="release"
CLEAN=0
DEPLOY=1
EXTRA_ARGS=()

usage() {
    cat <<'EOF'
Usage: ./build-macos.sh [release|debug] [options] [-- <extra cmake args>]

Options:
    --clean        Remove source/build before configuring
    --no-deploy    Skip the 'deploy' target (assets staging)
    -h, --help     Show this help

Examples:
    ./build-macos.sh                          # release
    ./build-macos.sh debug
    ./build-macos.sh release -- -DBUILD_STEAMLIB=0
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        release|debug) CONFIG="$1"; shift ;;
        --clean)       CLEAN=1; shift ;;
        --no-deploy)   DEPLOY=0; shift ;;
        -h|--help)     usage; exit 0 ;;
        --)            shift; EXTRA_ARGS=("$@"); break ;;
        *)             echo "error: unknown argument '$1'" >&2; usage; exit 1 ;;
    esac
done

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "error: build-macos.sh must run on a macOS host" >&2
    exit 1
fi

if [[ ! -f third-party/angelscript/sdk/angelscript/include/angelscript.h ]]; then
    echo "==> Initialising git submodules"
    git submodule update --init --recursive
fi

PRESET="workflow-macos-${CONFIG}"
BUILD_DIR="$ROOT/source/build"
[[ "$CONFIG" == "release" ]] && CFG_NAME="RelWithDebInfo" || CFG_NAME="Debug"

if [[ "$CLEAN" == "1" && -d "$BUILD_DIR" ]]; then
    echo "==> Cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

echo "==> Configuring ($PRESET)"
cd source
cmake -B ./build --preset "$PRESET" "${EXTRA_ARGS[@]}"
cmake --build ./build --config "$CFG_NAME"
[[ "$DEPLOY" == "1" ]] && cmake --build ./build --target deploy --config "$CFG_NAME"

echo "==> Build complete: source/build/warfork-qfusion/$CFG_NAME/"
