# Mage Bros. Native

An unofficial native C++ port of **Mage Bros.**, the 3D side-scrolling platformer by Saito Games
(2003–2009), in the same style as [Hopmon Native](https://github.com/Exo-MDR-CD2000/HopmonNative):
SDL3 and OpenGL 3.3 instead of DirectX 7, with modern display, input and audio options, and the
original game logic unchanged when every option is at its default.

> **Status: stage 7 of 7 in progress (version 0.4.15).** The whole original game is in and plays
> from start to finish: all four worlds and all 40 levels, every enemy and boss, one player or
> two, with the original's logo, title, save slots, level select, shop, saving and ending.
> Stage 7 adds optional modern extras: the options menu is made for Mage Bros. (the original's
> pause and FPS counter, rebinding both players' keys, replacement music); the window, monitors,
> exclusive fullscreen and VSync are done; widescreen, 16:9 and 21:9 included, shows the levels
> without pop-in at the edges, with the sound panned to match, and the title, save slots and level
> select with their green background filled out to the edges, the shop sliding in from the edges
> over a dimmed level, the start-up logo white to the edges, and the ending's film strips across the
> whole width; the HUD can be moved to the screen's edges and resized, and the field of view
> zoomed. The game's logic and speed are the original's at any resolution, refresh rate and frame
> rate. Smooth motion (drawing between the game's steps) is being built and tested: the camera,
> the mages and everything with a model so far. Next: the rest of smooth motion, then cheats and
> mods. See
> [docs/ROADMAP.md](docs/ROADMAP.md) for the plan and [CHANGELOG.md](CHANGELOG.md) for what
> changed in each version.

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
opens on the original's **logo** with its jingle, then the **title screen**: choose 1 PLAYER,
2 PLAYERS or EXIT with the arrow keys (or W / S, or a gamepad) and Enter (or Shift, Ctrl, a
gamepad button); Escape picks EXIT. Then the **save slots**: 8 for each game mode, each showing
the date it was saved and the level reached; choose one (an empty one starts a new game), or
press Delete to delete a saved game. Then **level select**, the world map: START plays the
level marked, SELECT LEVEL lets you move the marker to any level you have reached (Left / Right
along a world, Up / Down between worlds), BACK saves and returns to the title. Then the **game
view**: World 1-1 to start, with player 1's mage, who drops in from above and plays
with the original controls (arrow keys; jump Right Shift or `]`; fire Right Ctrl or `[`; in a
one-player game Left Shift and Left Ctrl too; or a gamepad). Shots cost magic, which refills over
time; jump-fire over a torch post to light its torch; collect coins; once every torch is lit the
goal opens: stand on it to warp to the next level. Lit torches burn if you stand in them, and the
mage flashes white after a hit. Escape gives up the level (losing some gold, as in the original),
once the level has finished fading in. Each level fades in and out with the original's venetian
blind.
The original status display shows the level, torches, life, gold and magic (and a boss's
hearts), sounds are panned left and right as the mage moves, and the original's shadow and
particle effects are in.

**Two players**: choose 2 PLAYERS on the title screen (or, as test shortcuts, press F2 in the
game view, which restarts the level, or start with `--two-player`). Player 2 is the second mage: D, A, W, S move, Left Ctrl or `1` fires, Left Shift
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

**The shop**: walk up to the door of a shop building (levels 3, 6 and 9 of worlds 1 to 3, and 4-2,
4-5, 4-8) and the original's MAGIC SHOP opens, pausing the level. The shopper buys with their own
gold: a heal potion (20 gold, one more heart), a magic potion (100, a faster magic refill) and
two wands that depend on the world. What you can't afford or already have is greyed out. Move
with the shopper's own direction keys and buy with any of their fire or jump keys or Enter
(player 1: arrows, Right Shift, Right Ctrl, `[` or `]`, and in a one-player game Left Shift or
Left Ctrl too; player 2: W A S D, Left Shift, Left Ctrl, `1` or `2`; or their gamepad; changed
keys count). The original took only Right Shift / Right Ctrl (player 2: Left Shift / Left Ctrl).
Choose EXIT to leave. As in the original, Escape also leaves the shop, but it gives up the level too.
Each wand fires its own pattern: the double wand two shots side by side, the triple wand a
shot and two smaller ones spread wider, and the X wands the same in homing shots that curve
towards the nearest enemy (or the boss).

**The ending**: clearing 4-10 plays the original's ending. The two mages walk down an endless
corridor between scrolling film strips while the credits slide past. On the last one, press a
button (Enter, Shift, Ctrl, a gamepad button or Escape) to return to the logo.

Defeated enemies drop coins. After a lost level, or a world's last level, the game saves and
returns to level select; world 4 opens once worlds 1 to 3 are cleared.

**Pause and options**: P pauses and resumes, as in the original. F10 (or gamepad Start) opens
the options menu whenever you can play (not during the logo, the ending, a fade or a death): video,
graphics, audio and controls, Main menu (back to the title), Level select in a level (leaving it
as you were when it started) and Quit. The controls page rebinds
player 1's keys, player 2's keys, the general keys (select, pause, back) and the gamepad's fire and
jump buttons. Options > Video > Show FPS shows the original's frames-per-second counter. Smooth
motion (on by default) draws at the monitor's refresh rate; off, the picture is drawn only when the
game ticks, at most 100 times a second, as the original. The game's speed is the same either way.
Settings not ready for Mage Bros. yet (cheats and mods) are greyed out. Put `Music01.mp3` (or `.flac`) to `Music06` in a `music` folder next to `MageBros.exe`
to replace the MIDI music.

**The window** opens where you left it, at the size you left it (maximized too), on the same
monitor; the first start opens the largest 4:3 window that fits the monitor. Alt+Enter or F11
switches between the window and the fullscreen mode you used last (borderless or exclusive), and
is remembered. Exclusive fullscreen can run at any resolution and refresh rate your monitor has
(Options > Video > Resolution and Refresh rate). Window sizes are real screen pixels: with Windows display scaling above 100% the
window looks smaller than its numbers suggest. Monitors are remembered by name, so the game finds
the right one even after they are plugged in or out.

**VSync** (Options > Video) is Off, On or Adaptive (on, but a late picture shows at once instead of
waiting); the line under it tells you if your graphics driver's own setting seems to override it.
With Smooth motion on, Frame limit caps the frames a second.

**Smooth motion** (Options > Video, off by default) draws the level between the game's 100 steps a
second: the camera, the mages, enemies, bosses, platforms, coins, cannonballs and everything else
with a model, with their shadows, so they move smoothly at any refresh rate. The game itself runs
exactly the same; the picture trails it by at most 10 ms. Particle effects, the mages' shots and
the dragon's flame still move in the game's steps for now (more to come). Off, the game draws on
its steps only, like the original.

**Widescreen** (Options > Video > Aspect): 4:3 is the original picture with black bars, Stretch
pulls it wider, and Widescreen widens the 3D view of the levels to fill the screen (16:9, 16:10,
21:9 and so on), showing more of the level to the left and right with nothing popping in at the
edges. The camera is the original's (45° lens, 340 away, 35° above) in every mode; widescreen only
widens it. Sounds are panned to match the wider screen too, whatever shape the window is. The
start-up logo is white to the edges, the scrolling green background of the title, save slots and
level select fills the whole width, and the fades between screens cover it. The shop slides in
from the screen's edges and dims the level across the whole width while it is open, and the
ending's film strips run edge to edge with the credits sliding in from the screen's edge. The
menus themselves stay in a 4:3 area in the middle.

**HUD** (Options > Video): HUD position (widescreen) is Centered, the original's place inside the
middle 4:3 area (the default), or Edges: the level and torches at the left edge, the players'
panels at the right edge and the FPS counter in the bottom-right corner. HUD max width keeps the
Edges HUD within 16:9, 16:10, 21:9 or 32:9 on wider screens (21:9 by default). HUD size, 50% to
200%, grows each part from its corner, in any aspect mode; when the level information and the
players' panels would meet (two players on a boss level, or a 4:3 screen), the HUD stops at the
largest size that fits. Level select keeps the original's layout.

**Field of view** (Options > Video) is **experimental: we recommend leaving it at 100%**, the
view the original's levels were made for. It zooms the view in levels, from 50% (in) to 200%
(out); 100% is the original's 45-degree lens. The title, level select, the shop's panel, the HUD and the
ending keep the original view. The original's levels are built only as far as its own view
reaches, so zooming out shows their unbuilt edges: black below the level's near edge from about
105%, and above the back wall and past the ends further out. Far-away enemies, which stay still
until you come near, can be seen standing still when zoomed far out.

In the game view N / B change level, F2 switches between one and two players and F3 jumps to the
ending (test keys: a game started with them is not saved; `--ending` on the command line starts
the game at the ending). Tab switches to the **level viewer**: every one of the 40
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

**Saved progress** is kept after every level, in `%APPDATA%\MageBrosNative\saves\original\progress.ini`
(or the folder set by `save_path` in the ini): the level you have reached, the worlds you have
cleared, and each player's gold, life, magic and wand, in 8 slots for one-player games and 8
for two-player games, as the original. Saves from the original game are not carried over.

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
docs         REVERSE_ENGINEERING.md (what the original is made of, and at its end the
             original's bugs and quirks with what the port does about each), ROADMAP.md,
             SMOOTH_MOTION.md (the plan for smooth motion between ticks)
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
