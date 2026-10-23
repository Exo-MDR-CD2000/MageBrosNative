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
  MageBrosNative-0.1.0-windows-x64.zip   the last test build
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
  7. hero A (*hero B*)
  8. enemies
  9. boss
  10. coins
  11. torches
  12. platforms
  13. static objects
  14. candles
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

## Where we are (last updated: end of stage 4)

Stage 4 (first playable: World 1, player 1) is done. World 1 plays from 1-1 to 1-10 with:

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
  waves (`XParticle_WaterWave`), and fish (`i`) swim and leap out of it;
- the dragon (`6`, `XBoss.cpp`, `JSpline.cpp`): head and 16 body segments on spline paths, its
  flames (`XBullet_Flame`), the boss hearts on the HUD, the music stopping and the goal opening
  when it is defeated.

### Checked in chunk 6

By screenshot: torch flames, coin sparks, warp rings and the goal's
lights, the hit and damage flashes, the turning cannon `K` (2-5), the straight cannonball (3-1),
the up-only elevator `B` (2-4) and the dropping platform `F` (4-10), and the water, splashes,
fish and dragon again after the pit fix. The owner checked in play: barrels breaking with their
coins, the dragon fight, and World 1 from start to finish. Still to compare against the
original game: nothing; the owner has since confirmed the dragon fight and the third-barrel heart
match the original.

Fixed after stage 4, from the owner's play: the fence doors (`v` `w`) were built as metal fence
pieces instead of the wooden gates, which made 1-2 unfinishable; now they stand and break as
in the original. The shop (`S`, in 1-3, 1-6 and 1-9) is built; its screen waits for stage 6.
The game now uses the original's light and lens and D3D7 normal scaling (0.2.2), and the wall
candles (row flag `C`, `XCandle.cpp`) stand on the back wall with their flames.
The game view now clears to black, as `XApplication::Render` does (0.2.4): the pits show the
cleared screen through the water, and the port's navy had made them look bright. The viewers
keep their navy.

### Next: stage 5, the whole game (worlds 2 to 4, every object, two-player co-op)

Split into 20 small chunks (7 is 7a and 7b), in level order: the table is in `docs/ROADMAP.md`, "Stage 5, chunk
by chunk". Chunk 1 is done (0.2.5): the tree `T` and plant `U` (`XStaticObj.cpp`; `CreateObject`
now calls every object's `CreateExtraCollideBox`, as the original) and the mushroom `h` / `2`
(`XEnemy.cpp`, a wallaby-style hopper). Checked by screenshot in 2-1; the king mushroom (2-4,
2-7) was not caught on screen. Chunk 2 is done (0.2.6): the needle balls `x` / `y` (`XEnemy.cpp`).
The needle-ball bullet is fired only by the genie, so it moved to chunk 7. Chunk 3 is done
(0.2.7): the squid `c` / king squid `4` (`XEnemy.cpp`). Chunk 4 is done (0.2.8): the
hammer `k` (`XEnemy.cpp`), camera shake (`XCamera::StartShaking`, reached through
`XGameContext::startShaking`; the camera now always runs its tick and shake and only follows a
hero that is alive), `XParticleList::CreateHammerEffect` and `AngleDifference` (`XMath`). Chunk
5 is done (0.2.9): the fans `q` `r` `s` (`XEnemy.cpp`), which push the hero through
`XHero::IncreasePushX/Z`. Chunk 6 is done (0.2.10): the turtle `e`
(`XEnemy.cpp`; it overrides `Damage` to hide in its shell). Levels 2-1 to 2-9 now have nothing
left unbuilt. Chunk 7 (the genie) was split, as the owner asked: 7a is the
genie and its lamp (`XBoss_Genie` without its shots, `XStaticObj_GenieLamp`, the lamp steam,
`XStaticObjList::CreateGenieLamp`), 7b its needle-ball attack (`_ProcessFrame_Attack`'s
`XBulletList::CreateNeedleBall`, `XBullet_NeedleBall`). Chunk 7a is done (0.2.11): `XBoss_Genie`
(`XBoss.cpp`), the lamp (`XStaticObj.cpp`), `CreateLampSteam`, `ApproxDistanceXZ` and
`XGameContext::DistanceToCloserHeroXZ`. Its attack runs its rounds and counts its 30 shots, but
the marked spot in `XBoss_Genie::Attack` fires nothing yet. Chunk 7b is next; wait for the owner
to start it.
The pieces:

- the remaining enemies (`f` kiwi, `g` ghost, `t` floor fan, `u`
  stalactite, `z` jump needle) and the rocket launcher's rocket;
- the remaining static objects (`M` `N` pots, `R` rocket launcher; the shop's screen waits for
  stage 6);
- the other bosses: the genie's needle-ball bullets (`7`), the nest (`8`, with the bird)
  and the wizard (`9`, with its blade, wand and fire);
- player 2 (`XHeroB`): its input, the camera midpoint, hero-on-hero collision and carrying, the
  second HUD panel and the second platform shadow (the `heroB` slots marked in the code).

After stage 5 come:

- **Stage 6:** the front end: title, save slots, level select, shop, ending.
- **Stage 7:** opt-in modern extras.

### Known stand-ins (replace them when their real code lands)

- Death restarts the level, and finishing the last level of a world goes to the next world.
  Both wait for the level select in stage 6.
- The shop's screen (the building stands; stage 6), the other bosses and the other unbuilt
  objects (marker cubes).
- No player 2 (stage 5).
- Only the starting wand's shot pattern exists. The others come with the shop.
