#!/bin/bash
set -e

# Default to release config if none provided, allowing docker run arguments to override
CONFIG=${1:-release}
# Shift off the first argument so any subsequent arguments can be passed to cmake
shift || true
CMAKE_ARGS="$@"
CONFIG_NAME="linux-x86_64-${CONFIG}"

export CC=gcc-12 CXX=g++-12

cd source
cmake -B ./build --preset "workflow-linux-${CONFIG}" ${CMAKE_ARGS}

cd build
make -j$(nproc)
make deploy -j$(nproc)