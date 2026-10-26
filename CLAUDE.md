# CLAUDE.md: Mage Bros. Native

This is the handoff file for picking up the work. Read it first, then `docs/ROADMAP.md` (where
we are) and `docs/REVERSE_ENGINEERING.md` (what the original does).

## What this project is

Mage Bros. Native is a faithful, one-to-one native C++17 reimplementation of **Mage Bros.**
(Saito Games, 2003–2009, the freeware 2.0 build) using SDL3 and OpenGL 3.3. It is modelled on
the owner's earlier project [Hopmon Native](https://github.com/Exo-MDR-CD2000/HopmonNative).
Both games share the same Saito engine and DirectX 7 framework, so `src/gfx`, `src/audio`,
`src/input`, `src/ui`, `src/framework` and parts of `src/core` came from it.

It reads every asset at runtime from the user's own `MageBros.exe` (SHA-256
`ac191080af002b8ad0615e432faa92a2a47ce76a9aaa667bacf628836c098c49`) and `Music01–06.mid`. The
repository contains no game content.

## Rules from the project owner (must follow)

- **Work only in this project folder.** The Hopmon Native repo is **reference only**. You may
  clone a personal copy to read it, but never edit the hosted GitHub repo, open pull requests
  against it, or change its GitHub page.
- **Use what is already inside the game.** "This cannot technically be called a native
  reimplementation if we cannot use the original collision library. Let's use what is already
  inside the game for the reimplementation." So collision uses the original's library, ColDet
  1.2 (`third_party/coldet`, LGPL, built as a separate static library), never a rewrite. Keep
  this in mind for every module, and revisit it whenever a choice comes up between the
  original's own code or library and a substitute.
- **The owner's aim for the port (2026-10-25):** it must work flawlessly at any resolution,
  refresh rate and frame rate without that affecting the game logic or speed, and play and feel
  identical to the original, enhanced with modern features. Where the original itself behaved
  differently at different frame rates (the heroes' presses, the ending's scroll), the port takes
  the frame-rate-independent behaviour. See "The frame loop and the frame rate" in
  `docs/REVERSE_ENGINEERING.md`.
- **High refresh rates must not change the game.** The port will add upgrades over the original
  (stage 7), including high refresh-rate support that leaves the game logic and speed untouched.
  Whenever the original advances something once per frame instead of once per tick, keep it as
  the original for now, but add it to the "Stage 7: frame-rate dependence" table in
  `docs/ROADMAP.md`.
- **One-to-one faithful by default.** The original's behaviour is unchanged when every option is
  at its default: the same tick order, constants, slot counts and quirks, including ones that
  look like bugs. Any modern extras (stage 7) must be opt-in options. The exception: a slip in
  the original that the owner chooses to fix (so far the overlapping lost-gold messages, 0.4.13,
  and the shop's keys, 0.4.9). Every bug or quirk of the original found, fixed or kept, goes in
  the "Bugs and quirks in the original" table at the end of `docs/REVERSE_ENGINEERING.md`.
- **Never commit game material:**
  - `MageBros.exe`, `MageBros.pdb`, `.mid` files, or anything exported from them;
  - screenshots of the game;
  - the private analysis notes (`MageBros-analysis-private.zip` and its `MageBros-analysis/`
    folder: disassembly listings and PDB dumps derived from Saito Games' program).

  `.gitignore` covers these; check `git status` before every commit anyway.
- **Licensing.** The port's own code is MIT. `src/game` is a reconstruction of the original
  program and is excluded from the MIT grant (see `LICENSE`, part 2). Keep the project
  MIT-friendly: no copyleft code outside `third_party`, and give each third-party piece its
  license in `third_party/` and `THIRD_PARTY_NOTICES.md`.
- **Work in chunks.** Each chunk is one coherent slice. Report progress with the roadmap table
  and say which chunk we're on. Don't start a new chunk until the owner asks for it.
- Later on, the owner will ask for "the complete decompiled code". No decompiled C exists. The
  analysis is disassembly plus PDB names and types. At that point, offer Ghidra pseudo-C or the
  listings.

## Folder layout on the owner's machine

```
MageBros-Project/                   (C:\Users\ExoJag\Documents\MageBros-Project)
  MageBros.exe, MageBros.pdb        the original game (with its debug symbols)
  Music01.mid ... Music06.mid
  MageBros-analysis-private.zip     private RE notes: never put these in the repo
  MageBros-analysis/                the same notes, unzipped (listings, pdb dumps, tools)
  MageBrosNative-0.1.0-windows-x64.zip   an early test build (releases are now on GitHub)
  MageBrosNative/                   this repository
```

A personal read-only copy of Hopmon Native is at
`C:\Users\ExoJag\Documents\Hopmon-Native-Reference\HopmonNative` (on the same commit as its
GitHub `main` when last checked). Read it for engine code to adapt (`core/saves` came from it);
never change it.

Unzip the analysis notes next to the repo, not inside it, giving
`MageBros-Project/MageBros-analysis/`. Its tools then find `../MageBros.exe` on their own, or
you can set `MAGEBROS_EXE` and `MAGEBROS_PDB_DIR`.

## Source layout

```
src/core       PE resources, SHA-256 check, bitmaps, WAV, .x models, levels, config, log, saves
src/gfx        OpenGL loader; Direct3D 7 / DirectDraw emulation (device.cpp maps the D3D states)
src/audio      sound mixer (stereo pan, per-sound copies), MIDI (TinySoundFont), music streaming
src/input      SDL3 -> DirectInput-style keyboard/gamepad state
src/ui         options menu overlay
src/framework  the DX7 SDK framework (d3dapp, d3dfile, d3dtextr, d3dutil)
src/game       the reconstructed game: one file per original module, original class names
src/main.cpp   AssetViewerApp: game view / level viewer / model viewer, CLI, automation
tools          MageBrosAssets (inventory / selftest / export)
tests          format tests (no game needed); map, hero and collision tests
docs           REVERSE_ENGINEERING.md (findings, by module), ROADMAP.md (status)
third_party    ColDet 1.2, TinySoundFont, dr_libs, stb, cgltf, font8x8
```

`src/game` is collected by `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`, so new files only need
re-running CMake. The hero test target (`mbn_hero_tests`) lists its sources explicitly, and
`tests/jmodel_stub.cpp` stubs JModel for it. Add sources there if XHero starts depending on
them.

### How the game code fits together

- **`XApplication`** (`src/game/XApplication.*`, owned by `AssetViewerApp` in `main.cpp` as
  `app_`) is the original's screen state machine: `FrameMove(n)` runs the current
  `XProcessObject` screen and makes the next one when a screen calls `SetState`. It keeps what
  the original application keeps: the `XSaveData` being played (`saveData_`: the `XLevelStatus`
  of world, level and progress, and both `XHeroStatus`es) and its slot (`slot_`, −1 until one is
  chosen; `SetSaveData`, `DoSaveData`), the one-player flag (`SetTwoPlayer`: the title sets it; F2 and `--two-player` are
  test hooks), the `XStatusDisplay`, the `JSoundList`, the menus' `JDInput` (`GetDInput`) and a
  music request (`PlayMusic(track)`, which `main.cpp` takes with `TakeMusicRequest`). Screens
  built: `XLogoObject`, `XTitleObject`, `XSelectSlot`, `XSelectLevelObject`, the game
  (`XGameScene`, kept and restarted with `Start()` on each entry; `Start` remakes its `XShop`)
  and `XEndingObject`. `PlayMusic` ignores the track already
  playing and `StopMusic` clears it, as `JMidiList` (the logo and the bosses' "stop the music"
  use `StopMusic`).
- **Saving**: `src/core/saves.*` (adapted from Hopmon Native's, without its legacy registry
  import) resolves the save folder (`%APPDATA%\MageBrosNative\saves`, or the ini's
  `save_path`) and reads and writes `original/progress.ini` safely. `XSaveData` (`src/game`)
  maps a slot to a section `1P_SLOTn` / `2P_SLOTn` (`ReadSlot`, `WriteSlot`), with the
  original's `Reset` / `Valid`. The game calls `DoSaveData` after each level's fade-out.
  `DoSaveData` does nothing while `slot_` is −1, and `StartGameAt` (N / B, F2, `--auto
  level=`) sets it to −1, so test-hook games never touch saves.
- **`XProcessObject`** is every screen's base: its state (NON, FADEIN, FADEOUT, PROCESS),
  `tick_`, `processType_` and the venetian-blind fade (`StartFadeIn` / `Out`, `ProcessFadeIn` /
  `Out`, `RenderFade`).
- **`XGameScene`** (the original's `XGameObject`) owns:
  - `XLevelView` (map data, object codes),
  - two `XHero`s (`heroA_`, `heroB_`; B is `XHero(false)`, always made, reset only in a
    two-player game, so NON for the whole level in a one-player one),
  - `XLevelLoop`,
  - `XCamera`,
  - each hero's `XShadow`;
  - the shop (`XShop`, `src/game/XShop.*`), opened through `XGameContext::startShop`.

  Its `ProcessFrame(n)` is `XGameObject::ProcessFrame`: it updates the application's `JDInput`
  (Escape and the shop's keys), runs the fade and the level's end (process types 0–4), then
  `n` calls of `Tick()` (one tick of play). While the shop is open it runs only the status
  display and the camera, then `XShop::ProcessFrame(n)`.
- **`XLevelLoop`** holds the lists the original's `XGameObject` owns (`XGameScene` is the
  `XGameObject` screen itself) and implements `XHeroWorld`, the callbacks XHero uses. It owns:
  - an `XGameContext`, the shared pointers that objects reach through `XGameObject` in the
    original;
  - `XParticleList` (300 slots);
  - `XStaticObjList` (150);
  - `XEnemyList` (110);
  - `XBulletList` (50);
  - `XPlatformList` (35);
  - `XWater`;
  - the boss (`XBoss.cpp`, one at most);
  - `XCandleList` (30, the wall candles);
  - the torches, coins and hero shots.
- **Tick order** (`XGameObject::ProcessFrame`):
  1. status display
  2. map (goal)
  3. water
  4. particles
  5. hero shots
  6. enemy bullets
  7. hero A, hero B
  8. enemies
  9. boss
  10. coins
  11. torches
  12. platforms
  13. static objects
  14. candles
  15. camera
- **Render order:**
  1. map
  2. hero and shadow
  3. coins
  4. torches
  5. platforms
  6. static objects
  7. candles
  8. bullets
  9. enemies
  10. boss
  11. water
  12. particles
  13. shots
- `XLevelLoop::IsReconstructed` / `kDone` lists the level codes that have been built. Every
  other code still draws a coloured marker cube (`XLevelView::FilterMarkers`). Add each code to
  `kDone` when you build its object.

## Build and test

Tools and tests only (no SDL), on any OS:

```sh
cmake -S . -B build -DMBN_BUILD_GAME=OFF -DMBN_GAME_EXE="/path/to/MageBros.exe"
cmake --build build -j
ctest --test-dir build --output-on-failure   # formats, collision, map_geometry, hero_movement
```

- Without `MBN_GAME_EXE`, only the format and collision tests run.
- `mbn_hero_tests` lands the mage on all 40 start floors, checks the jump height of 74.82, and
  runs 800,000 random ticks.

**Windows (the owner's platform).** Run `make_release.bat` (VS 2022+, "Desktop development with
C++"). It builds, tests and writes `build\MageBrosNative-<ver>-windows-x64.zip`. Each run
raises the version in `CMakeLists.txt` first: patch by default, or pass `minor`, `major` or
`none`. A failed build puts the version back, so commit `CMakeLists.txt` with each release. It
runs the map and hero tests when `..\MageBros.exe` exists, and pauses at the end. Or open the
folder in Visual Studio / use `cmake -S . -B build -A x64`. SDL3 is fetched on first configure.

**GitHub releases.** `.github/workflows/release.yml` runs on every push to `main` (and by hand
from the Actions tab): on a Windows runner it configures, builds, runs the no-game tests and
`cpack`, then publishes the zip as release `v<version>` (from `CMakeLists.txt`). Its notes are
that version's section of `CHANGELOG.md` (the commit message if there is none) and a link to the
full changelog. If that release already exists it does nothing, so a push only releases
when the version was raised: keep running `make_release.bat` (patch bump) for each chunk and
commit `CMakeLists.txt` with it. Pushing therefore publishes publicly; the zip holds no game
content.

**Cross-compile for Windows from Linux** (llvm-mingw):

```sh
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j && (cd build-win && cpack -G ZIP)
```

**Linux game build for headless testing.** Install the SDL3 build dependencies, or turn off
SDL's X11 extras, then configure with `-DMBN_BUILD_GAME=ON`. A container with no system GL
headers needs `GL/gl.h`, `GL/glx.h`, `GL/glext.h` and `KHR/khrplatform.h` on the include path,
taken from the Khronos registries. Run it under Xvfb with the dummy audio driver:

```sh
SDL_AUDIO_DRIVER=dummy xvfb-run -a -s "-screen 0 1280x1024x24" \
  ./build-game/MageBrosNative --game /path/to/MageBros.exe --windowed \
  --auto "30:level=1,5;40:place=100,600;60:down=Right;120:up=Right;130:shot=a.bmp;140:quit"
```

### Automation (`--auto`, for test runs and screenshots)

The argument is a `;`-separated list of `ms:action=arg` entries. The times are milliseconds since
start, not game ticks (a tick is 10 ms). The mage drops in for a few seconds after `level=` or
`place=`, so take screenshots 4000+ ms later. They are handled in
`AssetViewerApp::OnAutomation` in `src/main.cpp`:

- `key=<SDL key name>`: press a key. `down=` and `up=` hold and release one.
- `sdlkey=<SDL key name>`: a real key event through SDL's queue, which the options menu reads
  (`key=` never reaches it). Driving the menu: `sdlkey=F10` opens it (only while a player has
  control, so not on the logo: the title answers from about 13 s), then `sdlkey=Down`,
  `sdlkey=Return`, `sdlkey=Escape` about 200 ms apart. `focus=0` / `focus=1` fake the window
  losing and getting back the focus.
- `shot=<file.bmp>`: write a screenshot.
- `quit`: exit.
- `level=<world>,<level>`: start that level (1-based), from any screen (the logo included).
  The game opens on the logo for 9.5 s, then the title (which waits for a choice), so every
  test run starts with `level=`. To test the title itself: it opens at about 9.6 s and its menu
  answers from about 12.2 s (after the fade-in). `key=Down` and `key=Return` drive it;
  `key=Escape` alone picks EXIT and quits the program after the fade (about 2.5 s).
- `place=<x>,<z>`: move hero A there with `XGameScene::TestPlaceHero`, which calls
  `SetStartPos` then `Reset`. `placeB=<x>,<z>` does the same for hero B (two-player games only).
- `players=1` or `players=2`: the number of players for the next `level=`.
- `ending`: go to the ending from any screen (also F3 in a level, or `--ending` on the command
  line, which starts there instead of the logo). The first credit slides in about 8 s later. The
  last one stops in the middle about 45 s later and waits for a button (allow a little more:
  startup takes time).

`level`, `place`, `placeB`, `players` and `ending` (and F3, `--ending`) are test hooks only. Key names are SDL's (`Left Ctrl`,
`Right Ctrl`, `Left Shift`, `D`, `1`); a wrong name does nothing, silently.

Screenshots are game images: view them, then delete them. Never commit them.

## Reverse-engineering workflow

The original ships with its PDB, so every function, class and field has its real name. There
is no decompiled C. For each module:

1. **Find its functions.**
   - `listings/<Class>.asm` in the analysis notes, or
   - `python3 MageBros-analysis/tools/dis.py "XClass::Method"`. This needs Python 3 and
     `llvm-objdump` on the PATH. It names call targets and shows `.rdata` floats inline, as
     `[0x4b5f1c]{240}`. `dis2.sh` does the same with the prologue and epilogue noise stripped.
2. **Get the field layout.**
   - `python3 tools/fields.py <Class>` reads `pdb/types.txt`.
   - The `annot*.py` filters label `[ecx + off]` accesses with field names. Pipe a listing
     through them:

     | Filter | Labels fields of |
     |---|---|
     | `annot.py` | hero |
     | `annotG.py` | game object |
     | `annotT.py` | torch, coin, fire |
     | `annotE.py E` | enemy |
     | `annotE.py S` | static object |
     | `annotE.py B` | collide box |
     | `annotJ.py` | JObject3D |
     | `annotP.py` | particles |

     `annotE.py` also takes `off=name` arguments for subclass fields.
3. **Write the C++.**
   - Write it in `src/game/<Class>.{h,cpp}` under the original names.
   - Comment each function with the original function it re-expresses, and note any deliberate
     deviation.
   - Keep the constants exact.
   - Keep the original's loop shapes, for example "stop at the first empty slot".
4. **Write the findings up** in `docs/REVERSE_ENGINEERING.md` under the module's section.
5. **Verify.**
   - Build and run the tests.
   - Run the headless game at the level that uses the feature (`level=` and `place=`) and look
     at the screenshots.
   - Cross-build the Windows zip for the owner.
6. **Update `docs/ROADMAP.md`**: mark the chunk done and the next one **Next**. Also update the
   README's "What works today" section.
7. **Add the version to `CHANGELOG.md`**: a `## [x.y.z] - YYYY-MM-DD` section (that exact form:
   the release workflow looks for it) at the top, in plain words for players, taking in anything
   under `## [Unreleased]`. Commit it with the code so the release picks it up.

Conventions:

- `XEnemyBase` and the others keep the original's member names, with a trailing `_`.
- Model and sound IDs are the resource IDs in hex, for example `0x179` (MageCOL) and `0x26b`.
- Object y = floor level × 25.
- Visibility uses the camera's look-at Z: `|camZ - z| < 400` to draw and `< 480` to update. The
  port's draw range grows with widescreen (`XGameContext::visibleRange`); the update range never
  changes (it is game logic). Sound panning grows the same way (`XGameScene::PlaySound`). Both
  use `ViewWidthFactor` in `XGameScene.cpp`, the window's shape as laid out each frame: never
  hardcode an aspect ratio (the owner: the window can be resized to anything). 2D screens use
  `Device::ViewXRange` the same way (the view's x range in 640 x 480 pixels).

## Where we are (last updated: 2026-10-26, version 0.4.13, stage 7 chunk 2d-iii done but the options menu)

**Stages 1 to 6 are done: the owner played the whole game on 0.3.6 from start to finish and it
plays fine (2026-10-25), which closed stage 5 chunk 19 and stage 6 chunk 9.** Stage 7, the
opt-in modern extras, has begun. Chunk 1 (the options menu made for Mage Bros.) is done in
0.4.0, tested by the owner and pushed to the **`menu-dev` branch**: stage 7's menu work stays on
`menu-dev`, apart from `main`, until the owner says to merge it (a push to `main` publishes a
release; a push to `menu-dev` doesn't). Chunk 2 (the Video page) is split into small chunks
for thoroughness: 2a windowed mode (0.4.1, tested by the owner, on `menu-dev`), 2b exclusive fullscreen (0.4.2,
tested by the owner, on `menu-dev`), 2c monitors (0.4.3, tested by the owner, on
`menu-dev`), 2d-i VSync (0.4.4, on `menu-dev`), 2d-ii widescreen culling (0.4.5, tested by the owner, on
`menu-dev`) and widescreen sound panning (0.4.6, tested by the owner, on `menu-dev`; at 4:3 and
Stretch the panning is the original's bit for bit), then 2d-iii, widescreen on the other screens and the HUD, taken one screen at a time with the
owner. Done so far, all on `menu-dev` with 4:3 screenshots byte for byte the same as before:
the title's tile background filled out to the view's width on the title, save slots and level
select, with the fade strips spanning the whole view (0.4.7); the shop sliding in from the view's
edges over a full-width dim, the dim only when wider than 4:3 (0.4.8); the Saito Games logo's
white backdrop widened with the view (0.4.10, Stretch also unchanged); the ending's film strips
across the whole view and its credits waiting and leaving past the view's edges (0.4.11; the
ending can't be compared byte for byte, as the corridor scrolls once a frame, so a temporary log
showed the original's tiles and credit positions at 4:3 and Stretch); 0.4.12 added F3 in a level
and `--ending` to reach the ending, for the owner to test it; the HUD settings (HUD position,
HUD max width, HUD size) hooked into `XStatusDisplay` and `XFPSDisplay` (0.4.13; HUD position
defaults to Centered, the owner's call, and config_version 20 resets older files once; tried by
the owner, who also had the original's overlapping lost-gold messages fixed in every mode, the
first entry of the "Bugs and quirks in the original" table in `docs/REVERSE_ENGINEERING.md`). The
options menu in widescreen is left as it is for now (the owner: the menus and level select are
fine). Beside them, 0.4.9 made
the shop's button take every fire / jump key of the shopper and Enter (the owner couldn't find
how to buy or leave with the keyboard). **2d-iv, the field of view, next.** The owner's must: 16:9 with no culling or pop-in at all
(verified by screenshot comparison with a draw-everything build). The owner then played 21:9
(a 2560 x 1080 window): every object, enemy and boss showed and behaved normally to the edges, so
native 21:9 is feasible. The camera and lens were checked value by value against the original
(all the same): see "The camera and lens: the original and the port, side by side" in
`docs/REVERSE_ENGINEERING.md`. The owner set aside a
G-Sync / FreeSync frame-limit choice as a possible later feature (noted in the ROADMAP). The chunks after 2 are to be planned with the owner (smooth motion may become a stage of
its own).
Wait for the owner to say "start the next chunk". The tables are in `docs/ROADMAP.md` ("Stage
7, chunk by chunk"); what each version changed is in `CHANGELOG.md`.

**The options menu** (`src/ui/options_menu.*`, F10 / gamepad Start) came from Hopmon Native
almost line for line, with its config (`src/core/config.*`), input remapping (`src/input`) and
the renderer's extras (`src/gfx/device.cpp`). Chunk 1 made it Mage Bros.':

- `AssetViewerApp` (`main.cpp`) implements the menu hooks of `CD3DApplication`:
  - `PlayUISound`: the title's 0xec / 0xed.
  - `CanReturnToMainMenu` / `ReturnToMainMenu`: any screen but the logo and the title, to state 4;
    in a level the menu also shows "Level select" (`ReturnToLevelSelect` ->
    `XApplication::LeaveLevel`: the save as `XGameScene::InitLevel` found it, kept by
    `MarkLevelStart`, then state 6; nothing is written, so leaving can't dodge Escape's gold loss
    and keep the level's gold).
  - `InLevel`.
  - `CanOpenOptions`: `XApplication::PlayerHasControl`, which needs a running screen in PROCESS
    that isn't the logo or the ending; in a level, `XGameScene::PlayerHasControl` (PLAYING, a
    NORMAL mage, the shop not fading).
  - `ReloadMusic` / `MusicSource`.
  - `OnActivateApp`: the original's WM_ACTIVATEAPP music pause.
  - `OnKeyDown(DIK_P)`: the original's pause, toggling `m_bFrameMoving`.
- `PlayMusic` in `main.cpp` is `JMidiList::Play` with Hopmon Native's MP3/FLAC replacement
  (`MusicStream::FindReplacement`); `XApplication::StopMusic` closes both players.
- `XFPSDisplay` (`src/game`) is the original's counter, drawn by `XApplication::Render(d, bb,
  showFps, fps)`.
- The key pages are built from `kKeyActions` in `options_menu.cpp` (group, label, DIK):
  rebinding a DIK moves every place the game reads it (the menus' W / S too).
- `OptionsMenu::NotYet` greys out what isn't built: Field of view (nothing calls
  `Device::SetGameplayView` yet), Cheats and Mods (`core/mods.cpp` is still a stub; its header and the
  menu's mod pages still speak of Hopmon's assets).
- Smooth motion (`interpolation`, on by default, from Hopmon) only sets how often the picture is
  drawn: no game object takes part in `Interp::Tracked`, so frames repeat between ticks. Off, the
  framework draws only on frames with a tick (at most 100 a second, the original's pace).
- `config_version` 15 rewrote the ini notes for Mage Bros.' keys and dropped Hopmon's two
  cheats; `unlock_all_levels` is still read but nothing uses it.

**The window (chunk 2a, 0.4.1)**, in `src/framework/d3dapp.cpp`:

- SDL's window events (resized, moved, maximized, restored, display changed) call
  `WindowChanged`. `kWindowSettleMs` (500 ms) after the last one, `SaveWindowState` reads the
  window and saves `width` / `height`, `window_x` / `window_y`, `maximized` and `display` (only in
  Window mode, not minimized). A maximized window keeps its earlier size and place.
- `PlaceWindow` (at start and when switching to Window) sets the size and position twice:
  moving a window onto a monitor with another Windows display scale makes Windows resize it with
  the old scale's frame, which grew the window a little on every start before.
- `WindowPosVisible` drops a saved position no longer on any monitor (then centred).
- `WindowFitArea` is a monitor's usable area less the title bar and frame (measured, or
  estimated at the monitor's scale when no bordered window exists). `DefaultWindowSize` is the
  largest `kWindowScales` multiple of 640 x 480 within it: the first start (`Config::firstRun`)
  and Reset all settings use it.
- `ToggleFullscreen` goes to `lastFullscreen` (set by `ApplyDisplaySettings` whenever a
  fullscreen mode is applied; the menu's revert puts it back) and saves.
- Window sizes are SDL window coordinates, which are real pixels on Windows here: at the
  owner's 150% scaling a 1280 x 960 window is 1280 x 960 pixels.
- The owner's monitors, as SDL lists them (and the log, at every start): 1 "LS28AG700N",
  3840 x 2160 at 143.86 Hz, 150%, primary; 2 "PA278CGV", 2560 x 1440 at 143.97 Hz, 100%, left of
  it (Windows lists it on the AMD graphics; it still runs at 144 fps); 3 "TX-NR646", 1920 x 1080
  at 60 Hz, 100%, right of it (an Onkyo receiver, most likely a TV; it offers 3840 x 2160 modes).

**VSync (chunk 2d-i, 0.4.4)**: `vsync` is 0 / 1 / 2 (adaptive = swap interval -1, probed once
at start: `adaptiveSupported_`); `ApplyVSync` falls back to 1, reads back the interval
(`VSyncInEffect`) and runs after every display change. `JudgeVSync` (each second, only with Smooth
motion on, no limit, menu closed) sets `VSyncOverride` after two seconds of frames above 115% of
the refresh rate with VSync on (+1), or within 3% of it with VSync off (-1). Measured on the
owner's NVIDIA (Smooth motion on): VSync on or adaptive 144 / 60; off about 980 in a window, 490
in 4K borderless; Smooth motion off 100 (60 on the 60 Hz monitor with VSync). The frame limiter
schedules each frame from when the last was due (`lastPresent += interval`).

**Monitors (chunk 2c, 0.4.3)**: `CD3DApplication::SetDisplay` sets `display` and
`display_name` together (the menu and the window tracking use it); `ResolveDisplay` (at start
and on SDL_EVENT_DISPLAY_ADDED / REMOVED) finds the number by name, takes the primary for now
when that monitor is missing (keeping the name), and gives an older ini its name. `PlaceWindow`
uses a saved position only on the chosen monitor (`WindowPosOn`). The borderless window's extra
row is `Device::SetHiddenBottomRows(1)`: the layout uses the rows above it.

**Exclusive fullscreen (chunk 2b, 0.4.2)**: `fullscreen_refresh` (0 = the desktop's rate) goes
to `SDL_GetClosestFullscreenDisplayMode` with the chosen size (or the desktop's).
`OptionsMenu::RefreshList` gives a size's rates; `DisplayCfg::hz` carries the pending choice and
the keep / revert snapshot. `PendingMark` puts `*` only on rows that differ from what is applied.
The exclusive resolution list drops modes below 640 x 480 and the desktop's own size (Desktop
is last) and is clamped. Never press Enter on an exclusive mode in a test run: it changes the
owner's real screen mode; preview with Left / Right only. Testing window
  moves: a DPI-unaware PowerShell's `MoveWindow` works in 150%-scaled coordinates on the primary.

Stage 6 (the front end), for reference: `XApplication`'s states 3 logo, 4 title, 5 save slots,
6 level select, 7 game, 9 ending (8, the register screen, is never reached:
`RegInfo_Registered` returns 1 in the freeware build); `XProcessObject`'s fades; saving in
`core/saves` (no import of the original's registry saves, the owner's decision).

Playable now, with nothing left unbuilt (no marker cubes):

- **World 1**, 1-1 to 1-10 (stage 4): movement, shots, magic, torches, coins, goal and warp, the
  HUD, sound, death, shadows and particles; pillars, barrels, fences, fence doors, the shop
  building, wall candles; slimes, needles, bees, wallabies, barrel enemies, needle fans, fish;
  cannons and their balls; all seven platform kinds; water; the dragon.
- **World 2**, 2-1 to 2-10 (chunks 1 to 7b): trees, plants, mushrooms, needle balls, squids,
  hammers, fans, turtles, the genie and its lamp, the genie's needle-ball shots.
- **World 3**, 3-1 to 3-10 (chunks 8 to 14): pots, kiwis, jump needles, stalactites, rocket
  launchers and rockets, floor fans, ghosts, the nest and its 40 birds (the boss confirmed by the
  owner).
- **World 4**, 4-1 to 4-10: the first three worlds' objects and bosses plus the wizard (15a,
  confirmed by the owner) with its blade and its wand and fireballs (15b). The owner played
  World 4 level by level and found nothing wrong (2026-10-24). A script over the 40 levels' object fields found no code outside
  `kDone` but `!` (the invisible block, never drawn).

Keep the Tab viewers and the test hooks (`--auto`, N / B, F2) working alongside
the front end. Stage 6 chunk 1 notes: the sound cache moved to `JSoundList` (the application's);
`JSurfaceList::Create(id, colorKey)` makes a bitmap with or without the colour key, as the
original's `CreateSurface` flag (the fade strips 0x1f7 and the logo 0x2d1 have none; `Get` still
makes any other keyed); `XLevelStatus` has its play part (`IncreaseLevel` / `World`,
`IsLast*`, `Reset`, `Valid`), saved inside `XSaveData`. `XApplication::FrameMove` takes at most
10 ticks a frame, as the original.

**The shop (chunk 6, 0.3.5)** is in: `XShop` (see "The shop (`XShop`)" in
`docs/REVERSE_ENGINEERING.md`). Testing it: the shops stand at x 60 in 1-3, 1-6, 1-9, 2-3, 2-6,
2-9, 3-3, 3-6, 3-9, 4-2, 4-5 and 4-8. The door is at x 120, at the shop's z (1-3: z 6460; 4-2:
z 340, right by the start: the quickest by hand (the owner's tip)). In 4-2 a breakable fence panel
(the 'w' fence door, an enemy: `XEnemy.cpp`'s `case 'w'`) stands in front of the door and keeps
the shop shut; to test there, comment that case out temporarily (the owner's way), then
`place=180,340` at 4.5 s and `place=120,340` at 6.5 s: the shop opens about 2 s later. Put the
line back before the release build. In 1-3, `place=100,6400` first, wait about 14 s for
the camera, then `place=120,6460`. The mage is pushed out to x 129 and the shop opens as it
lands. The shop's menu answers from about 0.4 s after it opens (the 40-tick fade). Test-hook
games start with 0 gold, so every item is greyed out. To test buying, temporarily set the gold
in `XShop::Start`, and remove it afterwards.

**The wands (chunk 7, 0.3.5)** are in: `XLevelLoop::CreateFire` picks the pattern by
`wandType`; shots carry `large` (small: sprite 0.8, radius 6.4) and, for the homing ones, a target
enemy or boss (`XEnemyList::GetClosestEnemy`). `FindDirectionXZ` moved to `XMath`. See "Shots"
in `docs/REVERSE_ENGINEERING.md`. Testing tips: test-hook games start with wand 0, so set
`wandType` temporarily (e.g. after `SetStatus` in `XGameScene::InitLevel`). Slimes for homing:
1-1, `place=40,1040`, fire about 6 s later. The dragon: 1-10, `place=100,1340` puts the mage on
the island (z 1300 to 1460); the dragon becomes a target about 10 s in.

**The ending (chunk 8, 0.3.6)** is in: `XEndingObject` and `XCamera::ResetForEnding` (see "The
ending" in `docs/REVERSE_ENGINEERING.md`). `--auto ending` jumps there. Its corridor scrolls once
a frame (`textureOffset`, as the original), so it scrolls faster at higher frame rates. The owner chose
to keep it so (2026-10-25: minor and visual only); the details are under the "Stage 7:
frame-rate dependence" table in `docs/ROADMAP.md`.

Player 2 (chunk 16, 0.2.22) is in: `XHero(false)` has MageB's models, `IsHeroA()` picks its
status, shot colour, warp photons, lost-gold line and platform mark; `XGameScene::UpdateInput`
reads both players (XHeroB: D A W S, Left Ctrl / 1, Left Shift / 2, pad 1); every hero check in
the object code tests A then B; `XGameContext::GetCloserHero` picks the closer hero in NORMAL; the
camera takes both heroes; the two-player start, end and Escape are as `XGameObject`. See
"Player 2 (`XHeroB`)" in `docs/REVERSE_ENGINEERING.md`.

Hero on hero (chunk 17, 0.2.23) is in: `XHero::ProcessFrame(in, map, world, other)` tests the
other hero's box first on X and Z, lands on it (`other->SetHeroAbove`, not while
`pushedByFan_`), and a hero with `HeroAbove()` pushes the rider by its own move; platforms carry
and lift a stacked pair. See "Hero on hero" in `docs/REVERSE_ENGINEERING.md`. Testing tip: drop
the second hero in only after the first has landed (`placeB`, then `place` 3+ s later), or the two
falling boxes overlap and one is pushed aside. Chunk 18 (0.2.24): `XShadowOnPlatform::Render`
draws hero B's shadow on `shadow_[1]`; player 2's HUD panel matched the original instruction for
instruction, so it needed no change. A good place to test platforms with both heroes: the
elevator `A` in 1-6 at (100, 3100), with solid floor at (20, 3100) beside it (the `C`
platforms of 1-5 move, so a `place` onto them drops into the pit). Before
splitting any future large piece, read every state's transitions first: the nest's bird turned out to be one loop sharing its steering, so a split
proposed before reading it didn't fit; the wizard waits for each attack, so it split cleanly.

### Waiting for the owner to check in play

The owner's full play-through of 0.3.6 covered everything built up to stage 6. From stage 7:
the options menu of 0.4.0 (tested by the owner: works as expected; MP3/FLAC music was not
tried, as there were no files to try). The window of 0.4.1 was tested by the owner too, F11 into
exclusive fullscreen and back included (2026-10-25), and so were 0.4.2 (refresh rate), 0.4.3
(monitors), 0.4.5 (widescreen, at 16:9 and 21:9) and 0.4.6 (widescreen sound, by ear).
The ending in widescreen (0.4.11) was checked by the owner with 0.4.12's F3 (2026-10-26: "seems to
work"). Waiting for the owner's check: 0.4.7 (the menus' background and fades in widescreen), 0.4.8
(the shop in widescreen), 0.4.9 (the shop's keys) and 0.4.10 (the logo in widescreen), checked here
by screenshots only. The widescreen screenshots so far are at 16:9, 21:9 and 32:9; 16:10 wasn't
captured (the owner judged it fine: every fix works from the view's width, so 16:10, between 4:3
and 16:9, needs nothing of its own). The HUD settings of 0.4.13 were tried by the owner ("seems to
work fine"), who then found the lost-gold messages overlapping in two-player games, a slip in the
original as well: fixed in every mode in 0.4.13 (player 2's message 12 lower), seen on screen
here through a temporary `ShowLostGoldMessage` call in `InitForGame` (removed). MP3/FLAC music is the one feature not yet tried by anyone. (0.4.4's VSync driver
override warning can only be tried by changing the NVIDIA Control Panel's own VSync setting.)

### Notes from earlier stages

- Fixed after stage 4, from the owner's play: the fence doors (`v` `w`) are the wooden gates of
  the original (they had been built as metal fence pieces, which made 1-2 unfinishable); the
  original's light and lens and D3D7 normal scaling (0.2.2); the game view clears to black, as
  `XApplication::Render` does, so the pits show black through the water (0.2.4; the viewers keep
  their navy).
- `XStaticObjList::CreateObject` calls every new object's `CreateExtraCollideBox`, as the
  original does (the tree, the large pot and the turning cannon add boxes that way).
- `XCamera::ProcessFrame` always counts its tick and runs the shake; it follows only a hero that
  is alive (not NON, DEAD or COMPLETED). `XCamera::StartShaking` is reached through
  `XGameContext::startShaking`.
- The genie's lamp has its own `StartShaking` (it rocks the lamp, not the camera).
- Enemy bullets (cannonballs, flame, needle balls, rockets) never hit enemies: they check the map,
  objects, platforms and heroes only. A hero's shot stops on a bullet; the bullet carries on.
- `XHero::pushedByFan_` (the original's `pushedByFan`) is set by the floor fan and cleared at the
  end of each NORMAL tick; a hero lifted by a fan doesn't count as landing on the other hero.
- The nest's 40 birds are `XBossBase` objects owned by the nest (not in any list); shots only
  test the nest, and the waiting birds are drawn at its centre, as the original.
- 4-10 is an arena (floor at z 860..1020, x 40..160) surrounded by bottomless pit; the wizard
  wakes only when a hero is within 110 (XZ) of (100, 1140), so a test `place` must be at the
  arena's far edge (z 1036; 1030 is exactly 110 and doesn't wake it). Check the map rows (`0`
  floor, `.` pit) before choosing a `place`.
- SDL reads gamepads even in a window without focus: a test run while the owner plays another
  copy picks up their controller (it fired shots in a 3-10 test). Close other copies first.
- Player 2's keys clash with the game view's music keys (1, 2), so those are off in two-player
  games. Player 1's shots in a two-player test once went the wrong way in a screenshot and never
  again: most likely a gamepad of the owner's (see the note above), not the code.
- A shot stops on a hero's box only in NORMAL (`XHeroBase::CollisionCheck`); before 0.2.22 the
  port tested the box in any state.

### Working notes (tools and habits that worked)

- **Testing saves without touching the owner's**: copy `build/Release/MageBrosNative.exe` to a
  scratch folder, run it once (it writes its own ini there), set `save_path = saves` in that ini
  and test there (pass `--game` with the full path). A first run with an empty `save_path` makes
  `%APPDATA%\MageBrosNative` (with `save_location.txt`); remove it afterwards if it wasn't there
  before. **The owner now has real saves there (since 0.3.2): never change
  `%APPDATA%\MageBrosNative\saves`.** Every run, the isolated one included, rewrites
  `save_location.txt` in that folder with its save folder: back it up before testing and put it
  back after (it can't harm the saves, as copying only fills an empty folder, but keep it
  exact). The title's menu answers from about 12.2 s; after Return there the slot screen answers
  from about 18.2 s; after Return there (19 s) level select answers from about 24.2 s; a level
  fades in for 2.5 s after it starts (Escape does nothing until then), and a lost level saves and
  is back on level select about 12 s after Escape.

- **Building here**: CMake is not on the PATH; it is
  `/c/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`
  (and `ctest.exe` beside it), with the existing `build/` (Visual Studio 18 2026 generator,
  `MBN_GAME_EXE` set). `make_release.bat` runs from PowerShell as
  `"" | & cmd.exe /c "<full path>\make_release.bat"` (the empty input answers its final pause).
  It prints a harmless "'vswhere.exe' is not recognized" line.
- **Reading the listings**: `annotE.py E off=name ...` for enemies; for other classes a small
  filter that labels `[reg + off]` from a field map works well (fields from `pdb/types.txt`:
  find the class's `LF_FIELDLIST` by its destructor's name). Push order is reversed:
  `XCollideBox(app, w, h)` pushes h first. `fields.py` needs the field-list type id, not the
  class name.
- **Finding objects in levels**: levels are PE resources (ids in `src/core/game_files.cpp`'s
  `kLevelIds`); `pe_rsrc.load()` from the analysis tools gives the bytes. The object field is
  columns 8 to 12 of each line, and a cell is at x = col · 40 + 20, z = row · 40 + 20. Each
  level ends with a line of underscores: the line above it is row 0 (counting the underscores
  as row 0 puts everything 40 too far along Z).
- **Checking behaviour**: a temporary `fprintf(stderr, ...)` in the object's `ProcessFrame`
  (state, counter, position), run with `--auto ... 2> log.txt`, shows state machines that
  screenshots can't; a temporary hook can force a rare event (e.g. calling `Damage` once).
  Remove both before the release build. Writing C++ with a backslash-n escape through a Bash
  heredoc and Python turned it into a real newline here; use the Edit tool or `chr(92)`.
- **Screenshots**: `--auto` times are milliseconds. After `level=` the mage drops for about 3 s;
  after `place=` the camera eases at most 5 units a tick (500 a second), so a far `place`
  needs several seconds before the shot. Shots are 1280 × 960 BMPs: shrink them to PNG (e.g.
  with PowerShell's System.Drawing) to view, then delete them.
- **Per chunk**: disassemble, write the code, build and test, screenshot the level, update
  `docs/REVERSE_ENGINEERING.md`, `docs/ROADMAP.md`, `README.md`, `CLAUDE.md` and
  `CHANGELOG.md`, run `make_release.bat`, then report and wait for "commit and push". Pushing
  publishes the GitHub release; there is no need to watch the run unless the commit changed the
  workflow or the build setup.

### Known stand-ins (replace them when their real code lands)

- None left in the front end.
