# macOS: first run and acceptance

Date: 2026-11-12. Change: `macos-native-run` (`openspec/changes/macos-native-run`).

This records what was actually observed on a Mac. It is a functional acceptance of the port,
not a comparison against the original: no tolerance claim is made here, and screenshots are
listed separately from the log.

## Machine and toolchain

| Item | Value |
|---|---|
| Machine | Apple M5, arm64 |
| OS | macOS 27.2.1 (build 26B131) |
| Compiler | Apple clang 21.0.0 (Xcode SDK 27.2) |
| CMake | 4.4.5 |
| SDL3 | 3.4.18 — the system package for the development build, built from the pinned source for the package |
| Display | Built-in Retina, 1710x1107 at 60 Hz, scale 100% |
| Original | `MageBros.exe`, SHA-256 `ac191080af002b8ad0615e432faa92a2a47ce76a9aaa667bacf628836c098c49` (Mage Bros. 2.0, 2022 rebuild, freeware) |

## Build and tests

The development build, against the system SDL3:

```sh
cmake -S . -B build-mac -G Ninja -DMBN_FETCH_SDL=OFF
cmake --build build-mac
cmake -S . -B build-mac -G Ninja -DMBN_FETCH_SDL=OFF -DMBN_GAME_EXE=/path/to/MageBros.exe
ctest --test-dir build-mac --output-on-failure
```

Result: `formats`, `map_geometry`, `hero_movement` and `collision` all passed (4/4, 1.2 s).
The map and hero tests ran against the original above, unchanged from the Windows runs.

Executable: `build-mac/MageBrosNative.app/Contents/MacOS/MageBrosNative`, Mach-O arm64, SHA-256
`b4db3faf02886b5d93e67d6d8a8436466c99a3b01b4761cfb6266cddc83b6acb`. (During the first
session, before the bundle was added, the same binary was at `build-mac/MageBrosNative`;
`build-mac/MageBrosNative.app` is where it lives now.)

## The session

Automated with the game's own hooks, so the same input runs everywhere:

```sh
./MageBrosNative --game /path/to/MageBros.exe \
  --auto "0:level=1,1;1200:shot=mac-before.png;1300:down=Right;2900:up=Right;3000:shot=mac-moved.png;3100:key=Right Shift;3600:shot=mac-jump.png;3700:down=Right Ctrl;4000:up=Right Ctrl;4100:shot=mac-fire.png;4200:quit"
```

Observed:

- Level 1-1 rendered with its HUD (`LEVEL 1-01`, `TORCH 00/04`, `LIFE`, `GOLD`, `MP`), the
  floor, walls, columns, the bridge and the water.
- After 1.6 s of `Right` the hero had left the start platform and stood on the bridge; the
  camera followed.
- `Right Shift` put the hero in the air (jump).
- `Right Ctrl` fired: a shot is visible beside the hero.
- The run exited with code 0.

The first screenshot, at 1.2 s, shows the camera inside the level geometry. That is the
level-start drop (the hero falls from y = 800 for about 2.3 s) and not a rendering defect; the
later frames are normal.

## The log

```
Mage Bros. Native 0.4.38
Saves: /Users/<user>/Library/Application Support/MageBrosNative/saves/
Audio: 44100 Hz stereo
MIDI: no synthesizer available (place a General MIDI soundfont.sf2 next to the game to enable music)
Monitor 1: "Built-in Retina Display", 1710x1107 at 60.00 Hz, scale 100%, at (0, 0), primary
VSync: on
OpenGL 4.1 Metal - 91.7 | Apple M5
OpenGL vendor: Apple
Provoking vertex: available
Render target 2560x1920, MSAA 4x
Level view: World 1-1: 116 rows, 772 + 200 quads, 0 coins, 4 torches, 0 platforms, 50 objects, 0 fences, 39 enemies, 0 bosses
```

`Provoking vertex: available` means Apple's driver does export it through SDL, even though
`gl3.h` does not declare it. The renderer only shades smoothly today, so this is information,
not a requirement met.

Retina: the window is 1280x960 points and the render target 2560x1920 pixels, so the drawable
is scaled by 2 as intended by `SDL_WINDOW_HIGH_PIXEL_DENSITY`.

## Per-user data

The bundle and the build tree keep nothing beside the executable. On first run, macOS gets:

```
~/Library/Application Support/MageBrosNative/
+-- magebros_native.ini
+-- magebros_native.log
+-- save_location.txt
+-- saves/
```

`save_location.txt` contains the save folder path; `saves/` is where a campaign's
`progress.ini` is written once the game saves.

## The package

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMBN_SDL_FROM_SOURCE=ON
cmake --build build
cmake --build build --target package
```

Produces `build/MageBrosNative-<version>-macos-arm64.zip` (2.7 MB) holding
`MageBrosNative.app`, `MageBrosAssets`, `mbn_tests`, `mbn_collision_tests`, `README.md`,
`LICENSE`, `THIRD_PARTY_NOTICES.md`, `licenses/` (including the ColDet LGPL text) and `docs/`.
`otool -L` on the bundled executable lists only system libraries and frameworks: no
`/opt/homebrew` entry, so no SDL3 or other Homebrew library is needed at run time.

The zip was unpacked into a temporary folder and started from there; it reached level 1-1
and exited with code 0. Packaged executable SHA-256:
`e0c972d30757b74fd8d74b0eaf2db1339ad0c893719593c05a2c9e516f9bcb5d`.

## Screenshots (separate evidence, not committed)

| File | SHA-256 | Shows |
|---|---|---|
| `acceptance-1-1.png` | `34c7d4048a2bcbe076dfcbfb6813eaedb966db5c53bdaba4e018b84bf025dd2f` | level 1-1 after the drop |
| `mac-moved.png` | `eee04406ec4666a834cf1855e2f46cca7911019772db0cb5ad97b26c21ead83f` | after 1.6 s of `Right` |
| `mac-jump.png` | `39690f6c5f4969efbad6c91421a02992e23259b6904ea23227d668922e6529e1` | mid-jump |
| `mac-fire.png` | `965c0abd9d4dbda37a9673eece89ca30465bfb84b25a24d80e59d48efc5490da` | after firing |

## Specification scenarios

Every scenario in `specs/native-platforms/spec.md`, with the evidence for it.

| Scenario | Result | Evidence |
|---|---|---|
| Clean checkout builds | Pass | A fresh `git clone` into a temporary folder, the README commands only: configure, build, `mbn_tests` and `mbn_collision_tests` pass, package written |
| No Windows components are required | Pass | That same build used the default `MBN_BUILD_GAME=ON` on macOS with no Windows SDK, `winmm` or resource compiler |
| Level 1-1 is played | Pass | The automated session above: level rendered, `Right` moved the hero, `Right Shift` jumped, `Right Ctrl` fired |
| Audio is audible | Not verified | The device opened at 44100 Hz stereo; nobody has listened to it yet |
| First run on macOS creates the data folder | Pass | The folder was removed, the game run, and `magebros_native.ini`, `magebros_native.log`, `save_location.txt` and `saves/` appeared |
| A configured path still wins | Pass | `save_path = /tmp/mbn-save-test` in the ini: the log reported `Saves: /tmp/mbn-save-test/` and the folder was created there |
| Capabilities appear in the log | Pass | The log excerpt above: vendor, renderer, version, `MSAA 4x`, provoking vertex |
| A missing provoking-vertex function is not fatal | Not verified | This driver exports the function, so the branch was not exercised; the loader and `device.cpp` treat it as optional by construction |
| Package runs on a machine without SDL3 | Partial | The bundle links only system libraries (no `/opt/homebrew`) and ran from a temporary folder; it was not tried on a second Mac |
| Licences travel with the binary | Pass | The zip listing: `LICENSE`, `THIRD_PARTY_NOTICES.md`, `licenses/coldet-LGPL.txt` |

## Not verified

- Audibility of music and effects by ear. The audio device opened (44100 Hz stereo) and the
  game ran, but the owner has not yet confirmed that sound is heard. No SoundFont is installed
  on this machine, so music reports "no synthesizer available" exactly as it would on Windows.
- A save written after real progress (the folder and the pointer file appear; `progress.ini`
  needs a session that saves).
- Gamepad input on macOS.
- Side-by-side comparison with the original on Windows for the same input. The automation
  command above is the material for it; the comparison is the next change's work.
- Windowed/fullscreen switching, other display layouts, and macOS versions before 27.
