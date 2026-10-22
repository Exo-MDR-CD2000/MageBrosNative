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
mage bros project/                  (C:\Users\ExoJag\Desktop\mage bros project)
  MageBros.exe, MageBros.pdb        the original game (with its debug symbols)
  Music01.mid ... Music06.mid
  MageBros-analysis-private.zip     private RE notes: never put these in the repo
  MageBrosNative-0.1.0-windows-x64.zip   the last test build
  MageBrosNative/                   this repository
```

Unzip the analysis notes next to the repo, not inside it, giving
`mage bros project/MageBros-analysis/`. Its tools then find `../MageBros.exe` on their own, or
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
  - `XHero` (player 1),
  - `XLevelLoop`,
  - `XCamera`,
  - `XStatusDisplay` (the HUD),
  - the hero's `XShadow`.
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
  - the torches, coins and hero shots.
- **Tick order** (`XGameObject::ProcessFrame`):
  1. status display
  2. map (goal)
  3. water
  4. particles
  5. hero shots
  6. enemy bullets
  7. hero A (*hero B*)
  8. enemies
  9. *boss*
  10. coins
  11. torches
  12. platforms
  13. static objects
  14. *candles*
  15. camera

  The steps in italics are not built yet. Their slots are marked by comments in
  `XLevelLoop.cpp`.
- **Render order:**
  1. map
  2. hero and shadow
  3. coins
  4. torches
  5. platforms
  6. static objects
  7. *candles*
  8. bullets
  9. enemies
  10. water
  11. particles
  12. shots
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

The argument is a `;`-separated list of `tick:action=arg` entries, handled in
`AssetViewerApp::OnAutomation` in `src/main.cpp`:

- `key=<SDL key name>`: press a key. `down=` and `up=` hold and release one.
- `shot=<file.bmp>`: write a screenshot.
- `quit`: exit.
- `level=<world>,<level>`: start that level (1-based).
- `place=<x>,<z>`: move the hero there with `XGameScene::TestPlaceHero`, which calls
  `SetStartPos` then `Reset`.

`level` and `place` are test hooks only.

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

Conventions:

- `XEnemyBase` and the others keep the original's member names, with a trailing `_`.
- Model and sound IDs are the resource IDs in hex, for example `0x179` (MageCOL) and `0x26b`.
- Object y = floor level × 25.
- Visibility uses the camera's look-at Z: `|camZ - z| < 400` to draw and `< 480` to update.

## Where we are (last updated: end of stage 4 chunk 5d)

Stage 4 (first playable: World 1, player 1) is in progress. Chunks 1–4d and 5a–5d are done.
World 1-1 onwards plays with:

- movement, shots, magic, torches, coins, the goal and warp;
- the HUD, stereo sound, the death sequence, shadows and particles;
- pillars, barrels and fences;
- slimes, needles, bees, wallabies, barrel enemies, fence doors and needle fans, all with coin
  and heart drops;
- cannons (`H` `I` `J` `K`) and their bullets (`XBullet.cpp`): rolling balls in worlds 1–2,
  straight balls in worlds 3–4, with smoke and the BulletExplode burst;
- all seven platform kinds (`XPlatform.cpp`, codes `A`–`G`): the hero lands on them, rides
  them (push) and is lifted by elevators; enemies, shots and bullets stop on them; the hero's
  shadow is drawn on them (`XShadowOnPlatform`);
- water (`XWater.cpp`) in every level whose row flag is `W` (all of worlds 1, 3 and 4): the
  hero dies with a splash below −58, shots, cannonballs and dropping platforms meet it with
  waves (`XParticle_WaterWave`), and fish (`i`) swim and leap out of it.

### Next: chunk 5e, the dragon (World 1's boss, code `6`, level 1-10)

Original listings to read:

- `XBossBase` (the shared boss code: health, damage, `BossDefeated`, collision with the hero)
- `XBoss_Dragon` (head) and `XBoss_DragonBody` (its body segments)
- `XBullet_Flame` (its flames) and `XBulletList::CreateFlame`
- `XGameObject` for where the boss is created, ticked (after the enemies) and drawn

The work, in order:

1. **`XBossBase`** and **`XBoss_Dragon` / `XBoss_DragonBody`**: create the boss for code `6`
   (`XLevelLoop::Load`, `Kind::Boss`), tick it after the enemies and draw it. The dragon swims
   in the water (`_ProcessFrame_Swim1/2`, `_CheckSplash`) and moves along the floor; level 1-10
   is a water level.
2. **`XBullet_Flame`** in `XBullet.cpp`.
3. **The HUD**: the boss's health (`XGameScene::RenderStatus`, `bossHealth`).
4. **The goal**: a boss level opens when the boss is defeated (`XLevelLoop.cpp` ~252, marked).
5. Add `6` to `kDone`. Camera shake with the boss is still an open item (ROADMAP).

Then:

| Chunk | What it covers | Where to start |
|---|---|---|
| 6 | Polish and the playable World 1 zip | Check by screenshot what is still unverified: the fence door breaking, the third-barrel heart, torch flames, coin sparks, warp rings and lights, the hit flash and the damage flash. Compare against the original. |

After stage 4 come:

- **Stage 5:** the whole game, with player 2 and co-op.
- **Stage 6:** the front end: title, save slots, level select, shop, ending.
- **Stage 7:** opt-in modern extras.

### Known stand-ins (replace them when their real code lands)

- Death restarts the level, and finishing the last level of a world goes to the next world.
  Both wait for the level select in stage 6.
- Boss levels never open the goal (5e).
- The dragon, the shop and the other unbuilt objects show as marker cubes.
- No player 2 (stage 5).
- Only the starting wand's shot pattern exists. The others come with the shop.
