#!/usr/bin/env bash
# Linux build wrapper for Warfork.
#
# Runs a native Linux build by default. Pass --docker to build inside the
# Steam Runtime sniper SDK image (matches CI exactly).

set -euo pipefail

CONFIG="release"
USE_DOCKER=0
CLEAN=0
DEPLOY=1
EXTRA_ARGS=()

usage() {
    cat <<'EOF'
Usage: ./build-linux.sh [release|debug] [options] [-- <extra cmake args>]

Options:
    --docker       Build inside the warfork-builder Docker image
    --clean        Remove source/build before configuring
    --no-deploy    Skip the 'deploy' target (assets staging)
    -h, --help     Show this help

Examples:
    ./build-linux.sh                          # native release
    ./build-linux.sh debug                    # native debug
    ./build-linux.sh release --docker         # reproducible Docker build
    ./build-linux.sh release -- -DBUILD_STEAMLIB=0
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        release|debug) CONFIG="$1"; shift ;;
        --docker)      USE_DOCKER=1; shift ;;
        --clean)       CLEAN=1; shift ;;
        --no-deploy)   DEPLOY=0; shift ;;
        -h|--help)     usage; exit 0 ;;
        --)            shift; EXTRA_ARGS=("$@"); break ;;
        *)             echo "error: unknown argument '$1'" >&2; usage; exit 1 ;;
    esac
done

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if [[ ! -f third-party/angelscript/sdk/angelscript/include/angelscript.h ]]; then
    echo "==> Initialising git submodules"
    git submodule update --init --recursive
fi

if [[ "$USE_DOCKER" == "1" ]]; then
    if ! command -v docker >/dev/null; then
        echo "error: docker not found in PATH" >&2; exit 1
    fi
    if ! docker image inspect warfork-builder >/dev/null 2>&1; then
        echo "==> Building warfork-builder image"
        docker build -t warfork-builder .
    fi
    echo "==> Building Linux ($CONFIG) via Docker"
    docker run --rm -v "$ROOT:/root/warfork" warfork-builder "$CONFIG" "${EXTRA_ARGS[@]}"
    echo "==> Build complete: source/build/warfork-qfusion/"
    exit 0
fi

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "error: build-linux.sh runs on Linux hosts (or use --docker)" >&2
    exit 1
fi

PRESET="workflow-linux-${CONFIG}"
BUILD_DIR="$ROOT/source/build"

if [[ "$CLEAN" == "1" && -d "$BUILD_DIR" ]]; then
    echo "==> Cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

JOBS="$(nproc 2>/dev/null || echo 4)"

echo "==> Configuring ($PRESET)"
cd source
export CC="${CC:-gcc-12}" CXX="${CXX:-g++-12}"
cmake -B ./build --preset "$PRESET" "${EXTRA_ARGS[@]}"
cmake --build ./build -j"$JOBS"
[[ "$DEPLOY" == "1" ]] && cmake --build ./build --target deploy -j"$JOBS"

echo "==> Build complete: source/build/warfork-qfusion/"
