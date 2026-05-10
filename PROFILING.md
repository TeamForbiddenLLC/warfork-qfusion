# Profiling Warfork with Tracy

The codebase ships with [Tracy](https://github.com/wolfpld/tracy) zones across the per-frame hot paths. This guide walks through building a profiler-enabled game, building the Tracy GUI client, and connecting them.

Tracy is vendored at version **0.11.2** in [`source/extern/tracy/`](source/extern/tracy/) — you do not need to install it system-wide.

## How it works

- The game runtime links `TracyClient` and emits zone events when `TRACY_ENABLE` is defined at compile time.
- The Tracy profiler (a separate GUI app) connects to the running game over TCP and visualizes the timeline.
- When `TRACY_ENABLE` is **off** (the default), every Tracy macro expands to nothing — there is zero runtime cost, so non-profiled release builds are unaffected.

## 1. Build the game with Tracy enabled

Pass `-DTRACY_ENABLE=ON` through the platform wrapper:

```bash
# Linux
./build-linux.sh release -- -DTRACY_ENABLE=ON

# macOS
./build-macos.sh release -- -DTRACY_ENABLE=ON

# Windows (Developer PowerShell for VS 2022)
.\build-windows.ps1 release -- -DTRACY_ENABLE=ON
```

> **Tip**: Profile a `RelWithDebInfo` build (the default for the workflow presets) rather than `Debug` — debug builds spend most of their time in unoptimized code, which makes the timeline dominated by overhead rather than the work you care about.

When the executable starts, it will listen on TCP port **8086** for an incoming Tracy connection.

## 2. Build the Tracy profiler GUI

The GUI is a separate application that lives in [`source/extern/tracy/profiler/`](source/extern/tracy/profiler/). Build it once and reuse the binary across captures.

### Linux

System packages required (Debian/Ubuntu shown — the names map directly on Fedora/Arch):

```bash
sudo apt install build-essential cmake git pkg-config \
    libcapstone-dev libtbb-dev libfreetype-dev libdbus-1-dev \
    libwayland-dev libxkbcommon-dev wayland-protocols
```

Build:

```bash
cd source/extern/tracy/profiler
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

The binary lands at `source/extern/tracy/profiler/build/tracy-profiler`.

If you're on a system without Wayland (e.g. an older X11-only setup), pass `-DLEGACY=ON` to fall back to the X11 backend:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLEGACY=ON
```

### macOS

```bash
brew install cmake capstone freetype tbb glfw
cd source/extern/tracy/profiler
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The binary lands at `source/extern/tracy/profiler/build/tracy-profiler`.

### Windows

From a *Developer PowerShell for VS 2022*:

```powershell
cd source\extern\tracy\profiler
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The binary lands at `source\extern\tracy\profiler\build\Release\tracy-profiler.exe`. Dependencies (capstone, freetype, glfw, etc.) are fetched automatically via CPM during configure — no manual setup needed.

### Prebuilt option

If you don't want to build the GUI yourself, the upstream project publishes prebuilt Windows binaries on its [releases page](https://github.com/wolfpld/tracy/releases). Make sure the download matches the vendored version (**0.11.2**) — the wire protocol is not stable across major versions.

## 3. Connect and capture

1. Launch the Tracy profiler GUI (`tracy-profiler` / `tracy-profiler.exe`).
2. In the connection dialog, leave the address as `127.0.0.1` (or enter the game's IP if profiling remotely) and click **Connect**.
3. Start the game (`warfork`, `wf_server`, or `wftv_server`). The profiler attaches automatically and the timeline begins streaming.
4. Play through the scenario you want to profile.
5. Disconnect or close the game — the GUI keeps the capture loaded and offers **File → Save** to write a `.tracy` file for later analysis.

For headless / CI captures, the `tracy-capture` tool in [`source/extern/tracy/capture/`](source/extern/tracy/capture/) records to a file without a GUI:

```bash
cd source/extern/tracy/capture
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tracy-capture -o my-session.tracy
```

Run that *before* starting the game; it will wait for an instrumented client to connect, record until you hit Ctrl+C, then write the capture.

