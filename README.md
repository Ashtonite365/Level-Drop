# Grok Drop (ashtonite365.grok-drop)

Geode mod for **Geometry Dash 2.2081 (Windows)** with **Geode 5.11.x**. It auto-imports level files from a drop folder as new local levels. By Ashtonite365, GPL-3.0-only.

## Install
1. Close Geometry Dash.
2. Copy `ashtonite365.grok-drop.geode` into
   `C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\geode\mods\`.
3. Install the **GMD API** mod (hjfod.gmd-api ≥1.5.0). This mod needs it.
   - Geode normally offers to install missing index dependencies when it loads a mod.
   - If it doesn't, install it from the in-game Geode mod browser (search "GMD API").
4. Start GD. The folder `%LOCALAPPDATA%\GeometryDash\grok-drop\` is created on first start.

## Use
- Drop `.gmd` files (GDShare format) or `.txt` files with a raw level string (`kS38,...;1,1,2,...` or a gzip+base64 `H4sI...`) into the folder.
- The mod scans:
  - once when the main menu first loads;
  - each time you open **Create** (CreatorLayer);
  - each time you open **My Levels**.
- It skips scanning while the editor is open.
- Each file becomes a **new** level at the top of My Levels. Existing levels are never touched.
- Successful imports move to `imported\`. Failures move to `failed\` along with `<file>.log`.
- A small notification shows the level name.
- The only setting is **Drop folder** (Geode mod settings).
- **When levels are saved:** imports are saved through GD's normal save on exit, the same as GDShare. If the game crashes before exit, the import is lost, and the file is already in `imported\`. To retry, move it back out of `imported\`.

## Build

### Linux cross-compile (how v1.0.0 was built)
You need clang-cl/lld-link/llvm-rc (LLVM 19), CMake, Ninja, [xwin](https://github.com/Jake-Shadle/xwin) and Geode CLI 3.9.0.
- The Geode SDK must be checked out at v5.11.1, with `Geode.lib` from `geode-v5.11.1-win.zip` in `$GEODE_SDK/bin/5.11.1/`.
- The CLI needs a profile, so that it can fetch gmd-api.
```sh
xwin --accept-license splat --output /opt/gd/xwin
export GEODE_SDK=/opt/gd/sdk
geode profile add --name box /path/to/any/dir win
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_TOOLCHAIN_FILE=cmake/win-xwin.cmake -DXWIN=/opt/gd/xwin
cmake --build build      # -> build/ashtonite365.grok-drop.geode
```

### Windows
You need VS 2022 (C++ workload), CMake and Geode CLI. Run `geode sdk install` and `geode sdk install-binaries`, then:
```bat
geode build
```

### GitHub Actions
`.github/workflows/build.yml` uses `geode-sdk/build-geode-mod`. It builds on Windows and uploads the .geode as an artifact.

## Tools
- `tools/make_logo.py` regenerates `logo.png` (original design, PIL).
- `tools/parse_gmd.py FILE` parses a .gmd or .txt the same way the mod / GMD API does.
