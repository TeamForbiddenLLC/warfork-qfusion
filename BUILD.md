# Building Warfork

Warfork uses CMake (>= 3.20) with presets defined in [`source/CMakePresets.json`](source/CMakePresets.json). Two wrapper scripts and a Docker image cover the common paths so you don't have to remember preset names or per-platform build commands.

## Quick start

One script per platform — pick the one matching your host:

| Host                 | Command                       |
| -------------------- | ----------------------------- |
| Linux                | `./build-linux.sh`            |
| Linux (reproducible) | `./build-linux.sh --docker`   |
| macOS                | `./build-macos.sh`            |
| Windows (PowerShell) | `.\build-windows.ps1`         |

All four default to a release build, init submodules on first run, configure the right CMake preset, build, and stage assets via the `deploy` target. Output ends up in `source/build/warfork-qfusion/`.

## What about Docker for Windows / macOS?

Docker covers the **Linux** target only:

- **macOS in Docker** would require Apple's SDK plus `osxcross`; the SDK can't be redistributed, so this isn't supported. Build natively with `build-macos.sh`.
- **Windows in Docker** could in theory cross-compile via MinGW (toolchain files exist in `source/cmake/`), but CI ships through Visual Studio 2022 / MSBuild — the MinGW path is untested and would diverge from what releases use. Build natively with `build-windows.ps1`.

The per-platform wrappers each handle exactly one host, so there's no auto-detection magic to second-guess — the script you run is the script that builds.

## 1. Clone the repository

The build pulls in third-party dependencies as git submodules. The wrapper scripts will run `git submodule update --init --recursive` automatically on first run, but you can also clone recursively up front:

```bash
git clone --recurse-submodules https://github.com/TeamForbiddenLLC/warfork-qfusion.git
cd warfork-qfusion
```

## 2. Steamworks SDK (optional, on by default)

All shipping presets set `BUILD_STEAMLIB=1`. To satisfy that, place the Steamworks SDK in `third-party/steamworks/sdk/` (so `sdk/` sits next to `third-party/steamworks/README.md`). See [`third-party/steamworks/README.md`](third-party/steamworks/README.md) for the legal disclaimer.

To build *without* Steam integration, pass the option through to CMake:

```bash
./build-linux.sh   release -- -DBUILD_STEAMLIB=0
./build-macos.sh   release -- -DBUILD_STEAMLIB=0
.\build-windows.ps1 release -- -DBUILD_STEAMLIB=0
```

## 3. `build-linux.sh`

```
./build-linux.sh [release|debug] [options] [-- <extra cmake args>]

  --docker       Build inside the warfork-builder Docker image
  --clean        Remove source/build before configuring
  --no-deploy    Skip the 'deploy' target
  -h, --help     Show help
```

Examples:

```bash
./build-linux.sh                              # native release
./build-linux.sh debug                        # native debug
./build-linux.sh release --docker             # reproducible Docker build
./build-linux.sh release --clean              # wipe build dir and rebuild
./build-linux.sh release -- -DBUILD_STEAMLIB=0 -DUSE_GRAPHICS_NRI=0
```

The script runs the `workflow-linux-<config>` preset and sets `CC=gcc-12 CXX=g++-12` unless you override them. With `--docker` it builds (or reuses) the `warfork-builder` image and runs the same flow inside the container.

## 4. `build-macos.sh`

Must run on a macOS host; uses the Xcode generator.

```bash
./build-macos.sh                              # release
./build-macos.sh debug                        # debug
./build-macos.sh release --clean
./build-macos.sh release -- -DBUILD_STEAMLIB=0
```

Output: `source/build/warfork-qfusion/<RelWithDebInfo|Debug>/`. Both `arm64` and `x86_64` hosts are supported.

## 5. `build-windows.ps1`

Run from a *Developer PowerShell for VS 2022* so MSBuild and CMake are on `PATH`:

```powershell
.\build-windows.ps1                           # release
.\build-windows.ps1 debug                     # debug
.\build-windows.ps1 release -Clean            # wipe build dir
.\build-windows.ps1 release -NoDeploy         # skip asset staging
.\build-windows.ps1 release -- -DBUILD_STEAMLIB=0
```

The Windows presets enable `BUILD_UNIT_TEST=1`, `USE_GRAPHICS_NRI=1`, and (debug) `WF_BUILD_DSYM=ON`. Output is at `source\build\warfork-qfusion\<RelWithDebInfo|Debug>\`.

## 6. Docker (Linux target only)

The image is based on the Steam Runtime sniper SDK, matching CI exactly. `./build-linux.sh --docker` handles image build + run for you, but the raw flow is:

```bash
docker build -t warfork-builder .

# Release, in-place
docker run --rm -v "$(pwd):/root/warfork" warfork-builder

# Debug + extra CMake args
docker run --rm -v "$(pwd):/root/warfork" warfork-builder debug -DBUILD_STEAMLIB=0
```

The container runs [`docker-entrypoint.sh`](docker-entrypoint.sh), which configures `workflow-linux-${CONFIG}` and runs `make` plus `make deploy`.

## 7. Native build details (when you want to drop the wrappers)

### Linux

Dependencies: `gcc-12`/`g++-12`, CMake >= 3.20, GNU Make, plus `libsdl2-dev`, `libopenal-dev`, `libcurl4-openssl-dev`, `zlib1g-dev`, `libfreetype-dev`, `libvulkan-dev`, X11/Wayland headers.

```bash
cd source
export CC=gcc-12 CXX=g++-12
cmake -B ./build --preset workflow-linux-release
cmake --build ./build -j"$(nproc)"
cmake --build ./build --target deploy -j"$(nproc)"
```

### Windows

From a *Developer Command Prompt for VS 2022*:

```bat
cd source
cmake -B .\build --preset workflow-windows-release -G "Visual Studio 17 2022" -A x64
cmake --build .\build --config RelWithDebInfo
cmake --build .\build --target deploy --config RelWithDebInfo
```

### macOS

```bash
brew install cmake git zip unzip libidn2 curl
cd source
cmake -B ./build --preset workflow-macos-release
cmake --build ./build --config RelWithDebInfo
cmake --build ./build --target deploy --config RelWithDebInfo
```

The Xcode generator is used; `arm64` and `x86_64` are both supported.

## 8. CMake presets

| Preset                     | Platform | CMAKE_BUILD_TYPE  |
| -------------------------- | -------- | ----------------- |
| `workflow-linux-release`   | Linux    | RelWithDebInfo    |
| `workflow-linux-debug`     | Linux    | Debug             |
| `workflow-windows-release` | Windows  | RelWithDebInfo    |
| `workflow-windows-debug`   | Windows  | Debug             |
| `workflow-macos-release`   | macOS    | RelWithDebInfo    |
| `workflow-macos-debug`     | macOS    | Debug             |

Linux presets prefer system libraries (`zlib`, `OpenAL`, `cURL`, `freetype`); other platforms build the bundled copies in `source/extern/`.

## 9. Useful CMake options

Defined in [`source/CMakeLists.txt`](source/CMakeLists.txt). Pass via `-D<NAME>=<VALUE>` after `--`.

| Option                | Default | Purpose                                  |
| --------------------- | ------- | ---------------------------------------- |
| `BUILD_STEAMLIB`      | OFF*    | Build the Steamworks integration module  |
| `BUILD_UNIT_TEST`     | OFF     | Build cmocka/utest-based unit tests      |
| `GAME_MODULES_ONLY`   | OFF     | Build only `cgame`/`game`/`ui` modules   |
| `USE_GRAPHICS_NRI`    | OFF*    | Use the NVIDIA NRI renderer (Vulkan/D3D) |
| `USE_CRASHPAD`        | OFF*    | Enable Crashpad crash reporting          |
| `USE_GRAPHICS_X11`    | ON      | (Linux) X11 backend                      |
| `USE_GRAPHICS_WAYLAND`| ON      | (Linux) Wayland backend                  |
| `USE_SYSTEM_ZLIB`     | OFF*    | Use system zlib instead of bundled       |
| `USE_SYSTEM_OPENAL`   | OFF*    | Use system OpenAL Soft                   |
| `USE_SYSTEM_CURL`     | OFF*    | Use system cURL                          |
| `USE_SYSTEM_SDL2`     | OFF     | Use system SDL2                          |
| `USE_SYSTEM_FREETYPE` | OFF*    | Use system FreeType                      |
| `USE_SYSTEM_OGG`      | OFF     | Use system Ogg                           |
| `USE_SYSTEM_VORBIS`   | OFF     | Use system Vorbis                        |

\* The workflow presets override several of these defaults — check [`source/CMakePresets.json`](source/CMakePresets.json).
