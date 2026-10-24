# Mage Bros. Native

An unofficial native C++ port of **Mage Bros.**, the 3D side-scrolling platformer by Saito Games
(2003–2009), in the same style as [Hopmon Native](https://github.com/Exo-MDR-CD2000/HopmonNative):
SDL3 and OpenGL 3.3 instead of DirectX 7, with modern display, input and audio options, and the
original game logic unchanged when every option is at its default.

> **Status: stage 5 of 7 in progress (version 0.2.23).** The engine runs (window, OpenGL
> renderer, sound, MIDI music, keyboard and gamepad, options menu) and all 40 levels can be
> explored in the level viewer. All four worlds and all 40 levels can be played, every enemy and
> boss included, by one player or two (F2 or `--two-player`). Next: player 2's shadow on
> platforms and the checks that finish
> stage 5, then the title, save slots and level select (stage 6). See [docs/ROADMAP.md](docs/ROADMAP.md) for the plan and [CHANGELOG.md](CHANGELOG.md)
> for what changed in each version.

> **You need your own copy of the original game.** This repository contains no game content: no
> images, models, levels, sounds, music or text. Everything is read at runtime from the original
> `MageBros.exe` and its `Music01–06.mid` files. Without them nothing runs.

## Requirements

- **Mage Bros. 2.0, the freeware build** from [saitogames.com](http://www.saitogames.com/).
  The program checks the SHA-256 of `MageBros.exe`:
  `ac191080af002b8ad0615e432faa92a2a47ce76a9aaa667bacf628836c098c49`.
  Other builds are refused unless you pass `--allow-unverified`.
- Windows 10/11 x64 (the game, once it lands). The asset tool also builds on Linux and macOS.

## What works today

**`MageBrosNative.exe`**: put it next to `MageBros.exe` (or pass `--game PATH`) and run it. It
starts in the **game view**: World 1-1 with player 1's mage, who drops in from above and plays
with the original controls (arrow keys; jump Right Shift or `]`; fire Right Ctrl or `[`; in a
one-player game Left Shift and Left Ctrl too; or a gamepad). Shots cost magic, which refills over
time; jump-fire over a torch post to light its torch; collect coins; once every torch is lit the
goal opens: stand on it to warp to the next level. Lit torches burn if you stand in them, and the
mage flashes white after a hit. Escape gives up the level (losing some gold, as in the original).
The original status display shows the level, torches, life, gold and magic (and a boss's
hearts), sounds are panned left and right as the mage moves, and the original's shadow and
particle effects are in.

**Two players**: press F2 in the game view (it restarts the level), or start with
`--two-player`. Player 2 is the second mage: D, A, W, S move, Left Ctrl or `1` fires, Left Shift
or `2` jumps, or the second gamepad. Each player has their own life, gold and magic, shown in two
panels; the camera follows the point between the mages. The level is cleared once one mage has
warped and the other has warped or died, and lost only when both have died. (In two-player games
the 1 to 6 music keys are off, since 1 and 2 belong to player 2.) The mages bump into each
other, and one can stand on the other's head and be carried along, or up and down on a lift.

- **World 1** plays through: slimes, bees, wallabies, spikes, needle fans, breakable barrels and
  fence doors, pillars and fences, cannons, moving platforms, water in the pits with leaping
  fish, and in 1-10 the dragon, who swims round its pools and crosses the floor breathing fire.
- **World 2** plays through: trees, potted plants, hopping mushrooms, rolling needle balls,
  squids, hammers that slam down and shake the screen, fans that blow the mage along, turtles
  that hide in their shells, and in 2-10 the genie, who rises from its steaming lamp and fires
  spiked balls that roll after the mage.
- **World 3** plays through: pots, kiwis that run from the mage, spikes that jump up from the
  floor, stalactites that drop and shake the screen, rocket launchers whose rockets home in on
  the mage, floor fans that float the mage up to higher ledges, ghosts that drift through walls
  and swoop at the mage, and in 3-10 the nest, which sends a bird after the mage with every hit.
- **World 4** reuses the first three worlds' enemies and bosses; in 4-10 the wizard rises from the
  pit, circles the arena, and attacks with a great spinning blade whose wind blows the mage towards
  the edge or a wand that fires homing fireballs.

Defeated enemies drop coins. Dying restarts the
level and finishing a world's last level goes on to the next world, until the level select of
stage 6 exists.

In the game view N / B change level and F2 switches between one and two players. Tab switches to the **level viewer**: every one of the 40
levels, built by the original's own map code with each world's textures. The goal, coins and
torches use their real models; other objects are colour-coded markers (blue platforms, brown objects, grey
fences, red enemies, large magenta bosses).

| Key | Level viewer | Model viewer |
|---|---|---|
| Left / Right | scroll along the level (hold Shift: faster) | previous / next model (all 172) |
| Up / Down | zoom in / out | zoom |
| PgUp / PgDn | camera height | |
| N / B | next / previous level | |
| Home | back to the level start | |
| Tab | switch to the model viewer | switch to the level viewer |
| Space | next sound effect | next sound effect |
| 1 to 6 | play Music01 to Music06 | play Music01 to Music06 |
| F10 / gamepad Start | options menu | options menu |
| Alt+Enter or F11 | fullscreen | fullscreen |

The first run creates `magebros_native.ini` (every option, with comments) and
`magebros_native.log` next to the program.

**`MageBrosAssets.exe`**: inspects your copy from the command line:

```
MageBrosAssets [--game PATH] [--allow-unverified] --inventory      # what's in MageBros.exe
MageBrosAssets [--game PATH] [--allow-unverified] --selftest       # decode every resource
MageBrosAssets [--game PATH] [--allow-unverified] --export FOLDER  # write them out for reference
```

Put it next to `MageBros.exe`, or point `--game` at the exe or its folder. `--selftest` decodes all
122 bitmaps, 44 sounds, 172 `.x` models and 40 levels, and checks that every world/level slot
resolves to a readable level. Exported files come from your copy and are Saito Games' work: don't
share them.

## Building

Any C++17 compiler and CMake 3.20+:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release        # format tests, no game files needed
```

On Windows, double-click `make_release.bat` (Visual Studio 2022+ with the "Desktop development
with C++" workload): it raises the patch version in `CMakeLists.txt`, builds, runs the tests and
writes `build\MageBrosNative-<version>-windows-x64.zip`. Run it as `make_release.bat minor`,
`major` or `none` (rebuild at the current version) to choose the version step; a failed build
puts the version back. With `MageBros.exe` in the folder above the repository it also runs the map
and hero tests.

**Releases.** What changed in each version is in [CHANGELOG.md](CHANGELOG.md). Every push to `main` runs the GitHub Actions workflow in
`.github/workflows/release.yml`: it builds the same zip on a Windows runner, runs the tests that
need no game files, and publishes it as the GitHub release `v<version>`, the version being the
one in `CMakeLists.txt`. A push that leaves the version unchanged (docs only, say) makes no new
release, so raise the version (`make_release.bat` does) when a push should produce one. The map
and hero tests can't run there, since `MageBros.exe` is never uploaded; run them locally.

From Linux or macOS, cross-compile with llvm-mingw or MinGW-w64:
`cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake`.
SDL3 is downloaded and built automatically the first time. To build only the tools and tests (no
SDL), configure with `-DMBN_BUILD_GAME=OFF`. Add `-DMBN_GAME_EXE=/path/to/MageBros.exe` to also run
the map and hero tests on all 40 levels of your copy.

For scripted test runs the game takes `--auto "ms:action=arg;..."` (times in milliseconds) (keys, screenshots, level and
position jumps); see [CLAUDE.md](CLAUDE.md) for the actions and for building and running the game
headless on Linux.

### Collision (ColDet)

The original game uses [ColDet](https://sourceforge.net/projects/coldet/), an open-source
collision library. The port builds ColDet's own published source rather than a rewrite, so
collision behaves exactly like the original. ColDet is LGPL-2.0-or-later and lives in
`third_party/coldet` with its license; it is compiled as a separate static library, and since
this project's source is public, anyone can rebuild it with a modified ColDet as the LGPL requires.

## How it's put together

```
src/core     reading MageBros.exe (PE resources, SHA-256, bitmaps, sounds, .x models, levels),
             config, logging, mods (stage 2: original game only)
src/gfx      OpenGL loader and the Direct3D 7 / DirectDraw emulation layer
src/audio    sound-effect mixer, MIDI sequencer, MP3/FLAC music streaming
src/input    SDL3 keyboard/gamepad -> DirectInput-style state
src/ui       options menu overlay
src/framework  the DirectX 7 SDK framework the game was built on (d3dapp, d3dfile, d3dtextr, d3dutil)
src/game     the reconstructed game logic, one file per original module (XHero, XLevelLoop,
             XEnemy, XStaticObj, XParticle, XStatusDisplay, ...)
third_party  ColDet 1.2 (the original's collision library), TinySoundFont, dr_libs, stb, cgltf,
             font8x8
tools        MageBrosAssets
tests        format and collision tests; map and hero tests on your copy (-DMBN_GAME_EXE)
docs         REVERSE_ENGINEERING.md (what the original is made of), ROADMAP.md
CLAUDE.md    contributor handoff: project rules, build/test/RE workflow, current status, next steps
```

The engine layers (`src/gfx`, `src/audio`, `src/input`, `src/ui`, `src/framework`) and parts of
`src/core` come from Hopmon Native, by the same author: both games run on the same Saito Games
engine and DirectX 7 framework.

## Legal

Mage Bros., its name, artwork, levels, sounds and music are © 2003–2009 Saito Games; the MIDI
music is © Yamaha Corporation. This project isn't affiliated with or endorsed by Saito Games and
distributes none of the game's files. You supply your own copy of the freeware game.

The port's own code is MIT-licensed. The reconstructed game logic that will live in `src/game` is
a reconstruction of the original program and isn't covered by the MIT license. See
[LICENSE](LICENSE).
