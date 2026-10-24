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
- **One-to-one faithful by default.** The original's behaviour is unchanged when every option is
  at its default: the same tick order, constants, slot counts and quirks, including ones that
  look like bugs. Any modern extras (stage 7) must be opt-in options.
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

Unzip the analysis notes next to the repo, not inside it, giving
`MageBros-Project/MageBros-analysis/`. Its tools then find `../MageBros.exe` on their own, or
you can set `MAGEBROS_EXE` and `MAGEBROS_PDB_DIR`.

## Source layout

```
src/core       PE resources, SHA-256 check, bitmaps, WAV, .x models, levels, config, log
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

- **`XGameScene`** owns:
  - `XLevelView` (map data, object codes),
  - two `XHero`s (`heroA_`, `heroB_`; B is `XHero(false)`, always made, reset only in a
    two-player game, so NON for the whole level in a one-player one),
  - `XLevelLoop`,
  - `XCamera`,
  - `XStatusDisplay` (the HUD),
  - each hero's `XShadow`,
  - both players' `XHeroStatus` and the one-player flag (`SetTwoPlayer`; F2 or `--two-player`).
- **`XLevelLoop`** stands in for the original's `XGameObject` and implements `XHeroWorld`, the
  callbacks XHero uses. It owns:
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
- `shot=<file.bmp>`: write a screenshot.
- `quit`: exit.
- `level=<world>,<level>`: start that level (1-based).
- `place=<x>,<z>`: move hero A there with `XGameScene::TestPlaceHero`, which calls
  `SetStartPos` then `Reset`. `placeB=<x>,<z>` does the same for hero B (two-player games only).
- `players=1` or `players=2`: the number of players for the next `level=`.

`level`, `place`, `placeB` and `players` are test hooks only. Key names are SDL's (`Left Ctrl`,
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
- Visibility uses the camera's look-at Z: `|camZ - z| < 400` to draw and `< 480` to update.

## Where we are (last updated: 2026-10-24, version 0.2.22, stage 5 chunk 16 done)

**Stage 5 (the whole game) is under way: chunks 1 to 16 are done, so every object of the 40
levels is built and player 2 plays; chunk 17, hero-on-hero collision and carrying, is next.** Wait for the owner to say "start the next chunk". The chunk table, with the
levels each chunk completes, is in `docs/ROADMAP.md`, "Stage 5, chunk by chunk"; what each version changed is
in `CHANGELOG.md`.

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

Still to build in stage 5 (the remaining chunks):

| Chunk | What | Completes |
|---|---|---|
| 17 | Hero-on-hero collision and carrying | |
| 18 | Player 2's platform shadow; HUD panel checked against the original | |
| 19 | Checks and the whole-game zip, played start to finish by the owner | |

Player 2 (chunk 16, 0.2.22) is in: `XHero(false)` has MageB's models, `IsHeroA()` picks its
status, shot colour, warp photons, lost-gold line and platform mark; `XGameScene::UpdateInput`
reads both players (XHeroB: D A W S, Left Ctrl / 1, Left Shift / 2, pad 1); every hero check in
the object code tests A then B; `XGameContext::GetCloserHero` picks the closer hero in NORMAL; the
camera takes both heroes; the two-player start, end and Escape are as `XGameObject`. See
"Player 2 (`XHeroB`)" in `docs/REVERSE_ENGINEERING.md`.

Chunk 17 (hero-on-hero): `XHeroBase::ProcessFrame(otherHero)` checks the other hero's box first
on X, Z and Y (`XHeroBase::CollisionCheckX` / `Y` / `Z`, `SetHeroAbove`, `HeroAbove`); the port's
`XHero::ProcessFrame` doesn't take the other hero yet. Also waiting for it:
`XHero::pushedByFan_`, `XPlatformBase::_PushHeroY`'s carry of a hero standing on the carried one,
and `_CollisionCheckY_Hero`'s lifting of the other hero out of the lifted one's box (comments say
"chunk 17"). Chunk 18: `XShadowOnPlatform::Render`'s `shadow_[1]` for hero B. Before
splitting any future large piece, read every state's transitions first: the nest's bird turned out to be one loop sharing its steering, so a split
proposed before reading it didn't fit; the wizard waits for each attack, so it split cleanly.

After stage 5: **stage 6**, the front end (title, save slots, level select, the shop's screen,
the ending); **stage 7**, opt-in modern extras.

### Waiting for the owner to check in play

Not caught by screenshot, so worth the owner's eye: the king mushroom (2-4, 2-7); the camera
shake (hammers in World 2, stalactites in World 3); the genie opening its mouth before an attack,
and whether it looks as pale as the original while it is small and rising; the jump needles
springing up (3-1); how the floor fan floats the mage (3-4: it bobs about 120 above the fan,
and touching the blades from beside it hurts); the ghost (3-5: its warning, swoop and flinch,
and how it looks); two players (0.2.22: player 2's keys and second gamepad, the camera between
the mages, enemies turning to the closer one, the level ending when both are done). Already confirmed by the owner: World 1 start to
finish, the dragon fight, the third-barrel heart.

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
  end of each NORMAL tick; nothing reads it until player 2 lands on hero A (a lifted hero
  doesn't).
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

- Death restarts the level, and finishing the last level of a world goes to the next world.
  Both wait for the level select in stage 6.
- The shop's screen (the building stands; stage 6).
- The hero-on-hero collision and carrying (chunk 17): the two mages pass through each other.
- One or two players is chosen with F2 or `--two-player` until the front end (stage 6) asks.
- Only the starting wand's shot pattern exists. The others come with the shop.
