# Run Warfork in the browser

## Requirements

- CMake ≥ 3.5 and a native compiler (MSVC, GCC, or Clang)
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (`emcc` on `PATH`), for example `D:\emsdk`
- Node.js 18+
- Git submodules (`zlib`, `ogg`, `freetype`, `vorbis`, `tracy`, `stb`)
- A native Warfork dedicated server (`wf_server`) if you want multiplayer

```bash
git submodule update --init --depth 1 source/extern/zlib source/extern/ogg source/extern/freetype source/extern/vorbis source/extern/tracy source/extern/stb
```

Install Emscripten (example):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
# Windows: emsdk_env.bat / emsdk_env.ps1
source ./emsdk_env.sh
```

## Native build

The existing native client and server must keep their current behavior.

```bash
cmake -S source -B build
cmake --build build
```

## WASM build

From the repository root:

```powershell
.\build-wasm.ps1
```

```bash
chmod +x build-wasm.sh start-web.sh
./build-wasm.sh
```

Optional flags: `-Clean` / `--clean`, `-Configure` / `--configure`, `-Emsdk <path>` / `--emsdk <path>`.

Outputs are copied to `web/wasm/`:

- `warfork.js`
- `warfork.wasm`
- `warfork.data` (if `web/preload/` contains files)

## Web UI and proxy

```powershell
.\start-web.ps1
```

```bash
./start-web.sh
```

The script copies `.env.example` to `.env` if needed, runs `npm install`, then starts `web/server.mjs`.

Open http://localhost:8081

Expected logs:

```
Web server listening on http://localhost:8081
WebSocket proxy listening on ws://localhost:8081/ws
Warfork UDP server: 127.0.0.1:44400
```

The page loads the WASM client automatically, mounts `web/game/basewf/` into the virtual filesystem, then calls `Module.callMain()`. Click the canvas once to unlock audio and pointer lock.

## Warfork server

Start the native dedicated server on the configured port (`WARFORK_PORT`, default 44400). Native clients still talk UDP to the dedicated server directly. Browser clients go through the WebSocket proxy.

## Assets

Copy Steam’s `basewf` into `web/game/basewf/` (see `web/game/basewf/README.md`). You can also pack `assets/` with `.\package-web-assets.ps1` or `./package-web-assets.sh`. Git only keeps the README in that folder. Optional tiny files for the Emscripten `.data` image can go in `web/preload/basewf/`, then rebuild WASM.
