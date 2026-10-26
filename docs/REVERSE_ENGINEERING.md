# What Mage Bros. is made of

Notes from studying the freeware build (SHA-256 `ac191080…8c49`, linked 2022-03-20) and its
matching `MageBros.pdb`, for interoperability. They describe structure and behaviour in our own
words; no game data or disassembly is reproduced here. The PDB is the single most useful input:
it names every function, class, source file and type, so reconstruction can follow the
original's own structure.

## The executable

- 32-bit x86, Windows GUI, Visual Studio 2019-era toolchain (UCRT), ~540 KB of game and engine
  code in about 2,000 functions.
- Imports DirectDraw / Direct3D 7, DirectInput, DirectSound, `d3dxof` (.x loading), WinMM
  (MIDI), plus the usual Win32 DLLs. Same technology as Hopmon.
- All content except the music lives in the `.rsrc` section (7.7 MB).

## Original source layout (from the PDB)

The original tree was `trunk\Source\`. Three groups:

**J\* engine library** (Saito Games' in-house library, shared with Hopmon):
`JObject3D` (3D objects, rect/shadow/particle variants), `JModel` (.x model list), `JSurface`,
`JSprite` (2D), `JSound`, `JMidi`, `JDinput`, `JString`/`JStringList`/`JList`, `JSpline`,
`JAscii` (number display), `JWinReg` (registry), `JStandard`, `JDebugTool`.

**DirectX 7 SDK sample framework**, modified: `d3dapp`, `d3dframe`, `d3denum`, `d3dfile`,
`d3dtextr`, `d3dutil`, `d3dmath`.

**X\* game modules** (with original function counts):

| Module | Funcs | Contents |
|---|---:|---|
| XAppMain | 46 | `XApplication` (window, device, state machine, save data), FPS display, level/hero status |
| XGame | 21 | `XProcessObject` (fade in/out base), `XGameObject` (one level being played) |
| XTitle | 37 | logo, title, slot select, level select, ending, register-info screens |
| XMap | 54 | level grid, floor/wall/bridge/goal construction, map collision, water, warp zone |
| XMapObject | 86 | static objects: pots, barrels, pillars, trees, plants, cannons, rocket launchers, shop, fences, genie lamp |
| XHero | 47 | `XHeroBase`, `XHeroA` / `XHeroB` (player 1 and 2) |
| XHeroFire | 30 | the players' magic shots, incl. homing ("chase") shots |
| XEnemy | 182 | 20 enemy types (slime, bee, squid, wallaby, mushroom, turtle, kiwi, ghost, fish, hammer, needles, fans, stalactite, fence door, barrel, needle ball) |
| XBoss | 121 | genie, dragon (+ body), nest, bird, wizard (+ blade, wand) |
| XBullet | 50 | cannon balls, needle balls, rockets, flames, wizard fire |
| XPlatform | 63 | elevator, auto-moving X/Z, right-only, dropping, staying platforms |
| XParticle | 103 | 17 particle effects |
| XShop | 10 | the shop between levels |
| XCoin, XTorch, XCandle | 60 | collectibles, the torches to light, candles |
| XCamera, XShadow, XCollideBox, XStatusDisplay | 74 | camera, shadows, collision volumes, HUD |
| XRegister | 12 | the old shareware registration (not needed: the current release is freeware) |

**ColDet 1.2** (`coldet`, `coldet_bld`, `box`, `box_bld`, `math3d`, `mytritri`, `sysdep`,
`tritri`): Amir Geva's open-source mesh collision library (LGPL). The game links exactly these
eight objects, and every ColDet function in the PDB is present in the 1.2 release, so the game
used stock 1.2. The port builds that same source, unmodified, from `third_party/coldet`, which
gives collision results identical to the original's. Two build details keep it unmodified:
`tritri.c` is compiled as C (as Visual C++ did), and a forced-include header declares the three
rotation helpers that 2000-era compilers made visible through `friend` declarations.

## Resources

| Type | Count | Format |
|---|---:|---|
| `BITMAP` | 122 | Windows DIBs: 104 × 8-bit, 9 × 24-bit, 9 × 4-bit |
| `WAVE` | 44 | RIFF PCM, all 8-bit mono 22,050 Hz |
| `RCDATA` (`xof `) | 172 | text DirectX .x models (`xof 0303txt 0032`): `Mesh`, `MeshNormals`, `MeshMaterialList`, `Material`; no textures or UVs, colour comes from materials |
| `RCDATA` (grids) | 40 | level text, 4 worlds × 10 levels |
| other | 10 | icons, three dialogs (registration/setup), accelerators, manifest |

Music is six General MIDI files next to the exe (`Music01.mid`–`Music06.mid`).

### Levels

Read by `XMap::_LoadMapArray`, reimplemented in `src/core/level.cpp`:

- The map is always 5 cells wide. Each text line is one row.
- Reading stops at the first empty line or the first line starting with `_` (every level ends
  with such a terminator).
- The **last** line read is row 0; rows count up toward the top of the file.
- Each line is read by character position without a length check: position 0 is a row flag,
  2–6 are five height/terrain cells, 8–12 are five object cells. A shorter line reads missing
  positions as 0; anything after position 12 is ignored. Two shipped levels contain one such
  odd-length row each, which this rule makes harmless.
- Character sets seen: row flags `- C W`; cells `. 0–7 H`; objects use most letters, digits and
  `! # * + - /`. Their meaning is defined by `_ConstructFloorAndWall`, `_ConstructWallObjects`
  and `_ConstructObjects` and is next on the list to document.

`GetMapResId(world, level)` maps the 40 slots to resource IDs; the table is in
`src/core/game_files.cpp`. Level 10 of each world is the boss level.

### Map geometry (XMap)

Units and coordinates:

- A cell is **40 units** square in X and Z; heights step by **25 units**. Cell `(x, z)` has its
  centre at `(x·40 + 20, z·40 + 20)`. X runs across the 5-wide map, Z along the level.
- World → cell (`ConvertPosXToMapX` / `Z`): negative positions give −1. With the "floor" flag
  set it truncates `pos / 40`; otherwise it takes `ceil(pos / 40) − 1`, so a position exactly on
  a boundary belongs to the cell below it. `ConvertPosYToMapY` truncates `y / 25` (−1 below 0).
- `GetFloorLevel(x, z)`: −1 outside the map; the digit value for cells `0`–`9`; −1 for anything
  else (`.` is a gap, `H` a bridge).

How `_ConstructFloorAndWall` builds a level, in order:

1. Side walls for the whole level (`_CreateSideWalls`).
2. Back walls for every row, chosen by the row flag (`_CreateBackWalls`).
3. Underground scenery for every cell (`_CreateUnderground`): 40 × 200 cliff faces from y 0 to
   −200. A cell with a floor gets one under each edge whose neighbour (+X, +Z or −Z) is a hole
   (or outside the map); a hole cell gets only the face against the back wall when it is in the
   first column, and nothing else, so a run of holes is one open pit (with the water showing
   across it on a water level). The −X side is never built: the camera can't see it.
4. For every cell with a height digit: a floor tile (`_CreateFloor`) and walls where a
   neighbouring cell's height differs (`_CreateWalls`).
5. A second pass turns every `H` cell into a bridge at the neighbouring floor height
   (`_CreateBridge`).
6. All floor and wall pieces are merged into one mesh drawn with the level's floor texture, and
   the underground pieces into a second mesh with the underground texture
   (`GetUndergroundTexture`); both use perspective-correct texturing.

Textures per world (`XMap::XMap`, `GetUndergroundTexture`): floor 156 / 251 / 252 / 693 and
underground 347 / 657 / 347 / 657 for worlds 1–4.

The goal (`_CreateGoal`) is a separate 88×88 `JObject3D_Rect` lying flat at the goal cell, 0.1
above its floor, with texture 420, the quad's default UVs (the whole texture, no inset),
bilinear filtering and perspective-correct texturing. Its centre is kept on the map object.

### Models (J library)

- `JModelList` loads each `.x` resource once, then rotates the mesh −90° about X and scales it
  by 20 (`SetInitRotate` / `SetInitScale`, set in `XApplication::OneTimeSceneInit`): the models
  are authored Z-up.
- `JObject3DX` is an object made of one or more models used as animation frames
  (`AppendFrame`, `SetCurrentFrame`). Its world matrix is scale × roll (Z) × pitch (X) × yaw (Y)
  × translation, in that order.
- Player 1's body (`XHeroA`): frames 355 (MageA01), 356, 355, 357, 359 (jump), 358 (fall), with
  model 377 (MageCOL) as its collision model. The wand (`XHeroBase`) is one object whose 12
  frames are the wand variants: 439, 440, 447, 451, 449, 450, 445, 446, 452–455.
- `MakeValueCloseTo(cur, target, factor, minStep, maxStep)`: snaps to the target once closer
  than `minStep`; otherwise moves `factor` of the gap, clamped to [`minStep`, `maxStep`].

### Light and lens (`XApplication::InitDeviceObjects`)

Set once when the device starts and never changed by the game (the scenes only move the view):
dithering and lighting on, z-buffer on, blending off; projection
`D3DUtil_SetProjectionMatrix(45° (0.785398), aspect 0.75, near 10, far 1000)`; light 0 a
directional light from `D3DUtil_InitLight(DIRECTIONAL, diffuse 1, specular 1, -1, -1, 0.5)`
(white at full strength; specular lighting stays off), enabled; ambient `0x4c4c4c`. The strong
light and dim ambient give the models their marked light and dark sides.

Normals: Direct3D 7 transforms them by the inverse transpose of the world-view matrix and, with
`NORMALIZENORMALS` off (its default), never renormalizes them. A model drawn at scale s gets
normals 1/s long, so the coins (scale 0.35 and 0.6) saturate to bright gold from every side.

### Level start, camera and music

- Hero start (`XGameObject::XGameObject`): one player at (100, 0, 60), the middle of row 1;
  two players at (60, 0, 60) and (140, 0, 60). `XHeroBase::Reset` then lifts each hero to
  y = 800, so they drop into the level.
- Camera (`XCamera`): looks at (140, 120, z) from 340 units away, 35° (0.610865 rad) above the
  horizontal, horizontal angle π/2, so the eye is on the +x side. The look-at z starts at 160 and
  stays within [160, (rows − 4) × 40]; it eases toward its target (`MakeValueCloseTo`, factor 0.1,
  step 5). In play the target is the z of the one hero alive, or with both alive
  max(zA, zB) − min(|zA − zB|, 240) / 2: their midpoint while they are within 240 of each
  other, and 120 behind the one further along beyond that (the other can leave the screen).
  "Alive" is a live state other than NON, DEAD or COMPLETED. `XCamera::ProcessFrame` (game state) counts
  `localTime` every tick, follows only heroes whose live state is not NON, DEAD or COMPLETED, then
  runs `_UpdateShake` and the view matrix whether it followed or not.
- Shake (`StartShaking(m)` sets `shakeMagnitude`; only the constructor zeroes it, so a shake
  carries over a level change): while it is above 0 the look-at y is
  120 + sin((localTime mod 8) / 8 · 2π) × m, and m falls by 0.02 a tick, clamped at 0. The
  last y is left as it is when it stops (within 0.02 of 120). The hammer and the stalactite call
  it. (The genie calls its lamp's own `StartShaking`, which rocks the lamp, not the camera.)
- Music (`XGameObject::InitLevel`): Music06 when the level has a boss, otherwise Music02–05
  for worlds 1–4 (Music01 is the menus'). The test is `XGameObject::boss` (+0xcc, an
  `XBossBase*`), which `_ConstructObjects` sets for the boss codes `6` Dragon, `7` Genie, `8` Nest
  and `9` Wizard. Bosses appear in level 10 of every world and also in World 4's levels 2
  (Dragon), 5 (Genie) and 8 (Nest), which therefore play the boss music too.

A floor tile is a flat 40×40 `JObject3D_Rect` at height × 25, mapped to the top-left quarter of
the floor texture with a half-texel inset. The J library is the same one Hopmon uses, one
version newer: texture and UVs are set by separate calls (`SetTexture`, `SetUV`).

## The application and its screens (`XApplication`, `XProcessObject`, `XLogoObject`)

`XApplication::FrameMove(n)` caps a frame at 10 ticks. While `state` (+0x1ac) is 2 (running) it
calls the current screen's `ProcessFrame(n)` (+0x1c8). Any other state is a request: the old
screen is deleted, the state's screen is made, and the state becomes 2; the new screen runs from
the next frame. `Create` sets state 3, so the game opens on the logo. `Render` clears to the
background colour (black) and, while the state is 2, draws the running screen, then the FPS
display (below).

**`XApplication::MsgProc`** (before `CD3DApplication::MsgProc`):

- WM_SYSCOMMAND SC_KEYMENU (0xf100): swallowed, so Alt doesn't open the window menu.
- WM_KEYDOWN `'P'` (0x50): posts WM_COMMAND 0x9c44. That is the DX7 framework's
  IDM_TOGGLESTART (entry 3 of `CD3DApplication::MsgProc`'s command table at 0x410a30). It toggles
  `m_bFrameMoving` (+0x10) and moves the time base on resume. This is the manual's "P: Pause":
  nothing ticks and the last picture stays. The music plays on. It is a window message, so it
  works on every screen, and the key's auto-repeat toggles it again. The port toggles once per
  press (`AssetViewerApp::OnKeyDown`) and lets the key be rebound (DIK 0x19).
- WM_ACTIVATEAPP (0x1c): with wParam 1, `JMidiList::Resume` and `JDInput::SetAcquireKeyboard` /
  `SetAcquireJoystick(1)`; otherwise `JMidiList::Pause` and both set to 0. Losing the focus
  pauses the music and the input; the game itself keeps ticking.
- MM_MCINOTIFY (0x3b9) with wParam 1 (successful): `JMidiList::Replay`, the music's loop.

**`XFPSDisplay`** (+0x1cc, made in `XApplication::Create` after model 0x18d): a
`JSprite` of bitmap 0xbf, the "FPS" label (one frame, colour-keyed), and a `JNumber` of digits
0xbb (ten frames) with `SetDisplayZero(1)` and `SetNumDigits(3)`. `Render`, when
`RegInfo_GetDisplayFPS` (byte +0x2c of the settings) is set: the label at (552, 454), then the
number at (552 + label width + 4, 454) showing `CD3DApplication::GetFPS`. The port's setting is
`[video] show_fps`. The number is the framework's frames per second, so with the port's Smooth
motion on it counts the frames drawn, not the game's ticks (always 100 a second).

| State | Screen | Asks for next |
|---|---|---|
| 1 | (quit) | |
| 3 | `XLogoObject` | 4 at tick 950 |
| 4 | `XTitleObject` | 5 (one or two players: sets +0x1a4), 1 (quit) |
| 5 | `XSelectSlot` | 6, or 4 (back) |
| 6 | `XSelectLevelObject` | 7, 8 (when `RegInfo_Registered` is 0 and world ≥ 1: never, it returns 1 in the freeware build), or 4 (back) |
| 7 | `XGameObject` | 6, or 9 after the last world |
| 8 | `XRegisterInfoObject` | 6 |
| 9 | `XEndingObject` | 3 |

**`XProcessObject`**: `state` (+0x08: NON 0, FADEIN 1, FADEOUT 2, PROCESS 3), `tick` (+0x0c,
counted by each screen), 30 fade sprites (+0x10, bitmap 0x1f7, which `XApplication::Create` makes
without a colour key) and `processType` (+0x88, what the screen does next). `StartFadeIn`,
`StartFadeOut` and `StartProcess` set the type and the state and zero the tick. The fade is a
venetian blind: strip i is the 640-wide bitmap stretched over rows 16i to 16i + 15. With
t = max(tick − 20, 0) and k = (t > 5i ? min(t − 5i, 64) : 0) / 4, fading in puts the strip at
row 16i + k with height 16 − k (it opens downwards), fading out gives it height k from row 16i
(it closes downwards). Everything is covered or clear by tick 229; the screens wait for tick 254.
`RenderFade` draws the strips while fading in or out, last of all.

**The game's flow** (`XGameObject::ProcessFrame(n)`): `tick += n`, then

- FADEIN: `ProcessFadeIn`; at tick 254 `StartProcess(0)`.
- FADEOUT: `ProcessFadeOut`; at tick 254, by type: 0 → state 6 (level select); 1 →
  `IncreaseLevel` and `InitLevel` (which fades the next level in); 2 → `IncreaseWorld`, state 6;
  3 → `IncreaseWorld`, state 9 (the ending); then `DoSaveData`, and nothing else that frame.
- PROCESS type 0 (playing): a level cleared (as in "Player 2") → `StartProcess` 1, or 2 after a
  world's last level, or 3 after the last world's; lost → `StartProcess(4)`; Escape (pressed this
  frame) makes each hero in NORMAL give up. Types 1–3: after 200 ticks `StartFadeOut(type)`.
  Type 4: after 300 ticks `StartFadeOut(0)`.
- Then, unless the shop is open, `n` ticks of play. So a level is played while it fades in and
  while it fades out, and Escape does nothing until the fade-in is over.

**`XLevelStatus`** (11 bytes): `currentWorld`, `currentLevel`, `maxSelWorld`, `maxSelLevel[4]`,
`bossDefeated[4]`. `Reset`: world and level 0, `maxSelWorld` 2 (worlds 1–3 open), the rest 0.
`IncreaseLevel`: the level + 1 (back to 0 after 10), raising that world's `maxSelLevel`.
`IncreaseWorld`: level 0, `bossDefeated[world] = 1`, world + 1 (back to 0 after 4); then if worlds
1–3 are all cleared `maxSelWorld = 3`, else if the new world is 4 it becomes world 1, else
`maxSelWorld` rises to it. `IsLastLevel`: level 9; `IsLastWorld`: world 3.

**`XLogoObject`**: stops the music (`JMidiList::Stop`), makes the logo bitmap 0x2d1 without a
colour key, an `XCamera` with `ResetForTitle` (distance 500, vertical angle 0, horizontal angle
π, looking at the origin: the eye at (0, 0, −500)), the logo sprite and a 600 × 600
`JObject3D_Rect` turned −π/2 about X (upright, facing the camera), without perspective
correction, texture 0x13c; then `StartProcess(0)`. Each tick it counts; at tick 50 it plays sound
0x2d0 unpanned (`PlayMono`, volume 1); at tick 950 it asks for state 4. `Render`: the camera, the
backdrop, then the logo centred on the 640 × 480 screen (`DrawScale(1)`). It has no fade and
takes no keys.

**`XTitleObject`**: plays Music01 (`JMidiList::Play(0)`), makes its bitmaps (all colour-keyed
but the background tile 0x1fc), an `XCamera` with `ResetForTitle`, and:

| Field | What | Bitmap / texture |
|---|---|---|
| +0x94, +0x98 | the left and right arrows, 2 frames (blink) | 0xd0, 0xd1 |
| +0x9c[3] | the menu items 1 PLAYER, 2 PLAYERS, EXIT, 2 frames (chosen, not) | 0x233, 0x232, 0xc9 |
| +0xa8 | a caption centred at (320, 100) | 0x1fa |
| +0xac | the 120 × 120 background tile | 0x1fc |
| +0xb0 | the copyright line, centred at (320, 180) | 0xf8 |
| +0xb4 | the "unregistered" line at (1, 462), drawn only when `RegInfo_Registered` is 0: never | 0xf7 |
| +0xb8 | the title logo, a 555 × 208.125 particle (555 × 0.375) at (0, 120, 0), perspective-correct, linear filter | 0x1fb |
| +0xbc, +0xc0 | the two mages, 240 × 240 particles at (−140, −50, 0) and (140, −50, 0), alpha 0 | 0x208, 0x207 |
| +0xc4 | the chosen item, −1 until the fade-in ends, then 0 | |
| +0xc8 | a counter of every tick since the screen opened | |

Then `StartFadeIn(0)`. `ProcessFrame(n)`: tick and counter += n, `JDInput::UpdateInputState`;
FADEIN: at tick 254 the choice becomes 0 and `StartProcess(0)`; FADEOUT: at tick 254, type 0
sets the one-player flag (+0x1a4 = 1) and state 5, type 1 clears it and state 5, type 2 state 1
(quit); PROCESS: both arrows show frame (tick mod 50 < 25 ? 0 : 1); Escape makes the choice 2
and counts as the button; then up (`CheckUpPressed`) moves the choice up if it is above 0, else
down moves it down if below 2 (both with sound 0xec, unpanned), else the button plays 0xed and
`StartFadeOut(choice)`.

`Render`: the camera; the background tile in a 7 × 5 grid at (120x − s, 120y − s), s =
(int)(counter × 0.25) mod 120 (DirectDraw dropped the tiles crossing the screen edge; the port
clips them, as all its blits); the mages' alpha 0 up to counter 200, (counter − 200) / 500 up
to 700, then 1, times 0.8, each drawn facing the camera (`Render2`); the logo's alpha
(sin((counter mod 300) / 300 · 2π) × 0.5 + 0.5) × 0.4 + 0.5; the two captions; the items
centred at x 320 from y 386, each the next item's centre after its height + 4, on frame 0 when
chosen; while PROCESS with a choice, the left arrow at (item x − arrow width / 2 − 2, item y)
and the right at (item x + item width + 2, item y); then `RenderFade`.

**Menu input** (`CheckUpPressed`, `CheckDownPressed`, `CheckButtonPressed`, from
`JDInput`'s state updated once a frame; "pressed" = held now, not at the last update): up is the
Up arrow or W, or gamepad 1 or 2 pushed up; down the Down arrow or S, or a gamepad pushed down;
the button Enter, keypad Enter, either Shift, either Ctrl, or button 1 or 2 of either gamepad.
The gamepads are skipped when the joystick is disabled in the settings (`RegInfo`). A stick
direction (`GetJoyDirState`) is the axis beyond 800 of 1000: up Y < −800, right X > 800, down
Y > 800, left X < −800.

### Saving (`XSaveData`, `XApplication::DoSaveData`)

The application plays from one `XSaveData` (+0x1d4, 26 bytes): `month`, `day` (the date it was
last saved, which the save slots show), the `XLevelStatus` (+2; `GetLevelStatus` returns it) and
`heroStatusA` / `heroStatusB` (+14, +20; `GetHeroStatus(isA)` returns them). `SetSaveData` copies a
slot's save in; the slot number is +0x1a8 (−1 at start). `XHeroStatus` (6 bytes): `gold` (short),
`life`, `magicPoint`, `magicLevel`, `wandType`.

| Function | What it does |
|---|---|
| `XSaveData::Reset` | month 1, day 1, `XLevelStatus::Reset`, both `XHeroStatus::Reset` |
| `XSaveData::Valid` | month 1–12, day 1–31, and the level status and both hero statuses valid |
| `XHeroStatus::Reset` | gold 0, life 5, magic 10, magic level 0, wand 0 |
| `XHeroStatus::Valid` | gold 0–9999, life 0–5, magic 0–10, magic level 0–7, wand 0–5 |
| `XLevelStatus::Valid` | world 0–3, level 0–9, `maxSelWorld` 0–3, each `maxSelLevel` 0–9, each `bossDefeated` 0 or 1 |

`DoSaveData` (after every level's fade-out, and from level select) puts today's month and day
(`GetLocalTime`) into the save, then writes all 26 bytes as one binary registry value under
`HKCU\Software\SaitoGames\MageBros`, named `1P_SLOT%d` in a one-player game or `2P_SLOT%d` in
a two-player one with the slot number. `XSelectSlot::_LoadSlots` reads 8 slots of the current
game mode (`1P_SLOT0`–`7` or `2P_SLOT0`–`7`), each reset when missing or not `Valid`.

**The port** keeps the same slots in its own save file, `<save dir>/original/progress.ini` (the
save dir is `%APPDATA%\MageBrosNative\saves`, or the ini's `save_path`), one section per slot
named as the registry value, with `month`, `day`, `world`, `level`, `max_world`, `max_level`
and `boss_defeated` (four numbers each), and `a_` / `b_` `gold`, `life`, `magic`, `magic_level`,
`wand`. A slot with a key missing or out of its field's range, or not `Valid`, is reset, as
`_LoadSlots` does. The original's registry values are not read (the owner's decision).
`core/saves` (from Hopmon Native) writes the file safely: a temporary file read back and swapped
in, the previous version kept as `progress.bak`, a damaged file replaced by its backup (kept as
`progress.damaged`), unknown keys kept, and existing saves copied over when `save_path` changes.
The port does not save while no slot is chosen (the original would write a slot named `-1`), so
the test hooks that start a level directly never touch the save file.

### The save slots (`XSelectSlot`)

Fields: +0x108 the choice (0–7 a slot, 8 "back"), +0x10c the delete answer (0 yes, 1 no),
+0x110 a counter of every tick, +0x114 the delete question open, +0x118[8] each slot empty (1)
or holding a save (0), +0x138[8] the slots' `XSaveData`.

The constructor plays Music01, makes its bitmaps (all colour-keyed but the background tile 0x1fc
and the slot bar 0x23b), an `XCamera` with `ResetForTitle`, and lays out, centred on x 320: the
heading (0x23d one player, 0x23c two players) at y 36; then, from 24 below it, the 8 slot bars
(0x23b, 2 frames) each 2 below the last; "back" (0xc8, 2 frames) 2 further; the keys help (0x23a)
at y 464. The delete question (0x27f) is centred at (320, 427), OKAY (0x281) at (264, 457) and
CANCEL (0x280) at (376, 457), over a 600 × 70 bar (texture 0x218, alpha state 3, alpha 0.9) at
(0, −177, 0). Also: the month names (0x239, 12 frames), the slot numbers (0x23e, 8 frames), the
labels EMPTY (0x238), DATE (0x237), LEVEL (0xb9) and the dash (0x1a3), and three numbers on the
digits 0xba (the day, 2 digits; the world, 1; the level, 2). Then `_LoadSlots` and `StartFadeIn`.

`ProcessFrame`: input, tick and counter += n. FADEIN: at 254 the choice is 0, `StartProcess`.
FADEOUT: at 254, type 0 sets the application's slot (+0x1a8) and `SetSaveData(slot's save)` and
goes to state 6 (level select); type 1 to state 4 (the title). PROCESS: the arrows blink as on the
title; then

- while the question is open: Escape answers CANCEL and counts as the button; left moves to OKAY,
  right to CANCEL (sound 0xec when it changes); the button deletes the slot on OKAY (sound 0x284,
  `_DeleteSlot`) or plays 0xed on CANCEL, and closes the question;
- otherwise Escape chooses "back" and counts as the button; Delete on "back" or an empty slot
  plays 0x283, on a saved one plays 0x282 and opens the question with CANCEL chosen; else up and
  down (sound 0xec) move the choice round 0–8, and the button plays 0xed and fades out with type
  0 (a slot, saved or empty) or 1 ("back").

`_DeleteSlot` deletes the slot's registry value, resets its save and marks it empty. `_LoadSlots`
marks every slot empty and reset, then reads each value: a slot read and `Valid` holds a save,
one read but not valid is reset and empty.

`Render`: the camera, the scrolling background as on the title, the heading of the game mode,
then each slot: its bar (frame 0 when chosen); the slot number (frame i) at (bar x + 8, bar y −
5); from 8 right of the number and 8 below the bar's top, EMPTY, or DATE, the month (frame month
− 1), the day, 10 right of it LEVEL, 4 right the world + 1, the dash (advancing a third of its
width), the level + 1. Then "back" (frame 0 when chosen) and the keys help. While PROCESS: with
the question open, the bar (facing the camera), the question, OKAY and CANCEL (frame 0 the one
chosen) and the arrows 2 either side of it; else the arrows 6 either side and 6 below the top of
the chosen slot, or 2 either side of "back". Then `RenderFade`.

**Music** (`JMidiList`): `Play(i)` does nothing when track i is already the current one, so
Music01 carries on from the title into the slots, and a level replayed after a death keeps its
music going; `Stop` clears the current track, so the next `Play` starts again.

### Level select (`XSelectLevelObject`)

Fields: +0xe0 the menu item (0 START, 1 SELECT LEVEL, 2 BACK; −1 until the fade-in ends), +0xe4
a counter of every tick, +0xe8 picking a level.

The constructor plays Music01 and makes, all colour-keyed but the background tile 0x1fc: the
arrows 0xd0 / 0xd1 (left, right) and 0x236 / 0x235 (up, down), 2 frames each; the menu items
0xcd, 0xcc, 0xc8 (2 frames: chosen, not); the heading WORLD MAP (0x222); the four world
banners 0x226–0x229; the level dot (0x224, 2 frames: frame 1 reached), the link between dots
(0x223), the cleared tick (0x22a), the scroll panel (0x225) and the marker (0x1fd, the status
display's). Then `XStatusDisplay::InitForSelLevel` (as `InitForGame`, with no game object, so it
draws no magic) and `StartFadeIn`.

`ProcessFrame`: input, tick and counter += n. FADEIN: at 254 the item is 0, `StartProcess`.
FADEOUT: at 254, type 0 goes to state 7 (the game; or 8, the register screen, when unregistered
and the world is past the first: never here), type 1 calls `DoSaveData` and goes to state 4
(the title); that frame ends there. PROCESS: all four arrows blink; then

- picking: Escape counts as the button; left takes the level down (below 0 it becomes the
  world's `maxSelLevel`), right up (past `maxSelLevel` back to 0); up takes the world down (below
  0 it becomes `maxSelWorld`), down up (past `maxSelWorld` back to 0), each then bringing the
  level down to that world's `maxSelLevel` if it is higher; all with sound 0xec. The button plays
  0xed and ends picking. The choice is made in the save's level status itself;
- otherwise Escape chooses BACK and counts as the button; left and right move the item (sound
  0xec, no wrapping); the button plays 0xed and on START fades out (type 0), on SELECT LEVEL starts
  picking, on BACK fades out (type 1).

Then (except on the frame that ends the fade-out) the status display runs n ticks.

`Render`: the camera; the scrolling background as on the title; the panel centred at (320, 250);
WORLD MAP centred at (320, 116); the banners centred on x 320 from 10 below it, each the next
34 below the last's bottom. For each world: the tick at (banner right − 10, banner top − 4) when
it is cleared (`bossDefeated`); ten dots from x 130 at 4 below the banner, frame 1 when the world
is open (≤ `maxSelWorld`) and the level reached (≤ its `maxSelLevel`); after each but the last,
x moves on half a dot, the link is drawn there, and x moves on half a link; the marker at (dot x
− 9, dot y − 12) on the current level. The menu from (136, 436), each item 40 after the last
(frame 0 chosen). While PROCESS: picking, four arrows centred around the marker's centre (+2
down): left and right at x ± 20 shifted right by a quarter of the left arrow's width, up and down
at y ± 20 shifted right by a quarter of the up arrow's width; otherwise the left and right arrows
2 either side of the chosen item. Then the status display and `RenderFade`.

With level select in, the game's level ends are the original's: a lost level saves and returns
to level select at the same level; a cleared world (`IncreaseWorld`) saves and returns to level
select at the next world's first level; only the last world's end goes on (to the ending).

**`JSprite`**: `Draw` blits the current frame stretched to the draw size (`SetDrawSize`, −1 for
the bitmap's own); `DrawScaleXY(sx, sy)` blits it scaled about the sprite's centre
(`GetCenterIntX` = position + width / 2), sizes rounded half away from zero.

### The shop (`XShop`)

`XGameObject` owns one `XShop` (+0x9c); `InitLevel` calls `XShop::Reset`. Fields: +0x4 the game
object, +0x8 `localCounter`, +0xc `state` (NON 0, FADEIN 1, FADEOUT 2, PROCESS 3), +0x10 `pHero`,
+0x14 four `XShopItem`s of 16 bytes (`type` (`XShopItemType`), `pSprite`, `price`, `enabled`),
+0x54–0x9c the sprites, +0xa0 the magic-level number, +0xa4 the back panel, +0xa8 `cursorPos`
(0–3 the items, 4 exit), +0xac–0xbc the keys pressed this frame (right, left, up, down, button).

`XShopItemType`: 0 single wand, 1 double, 2 triple, 3 single X, 4 double X, 5 triple X (the
same numbers as `XHeroStatus::wandType`), 6 heal potion, 7 magic potion.

The constructor makes the bitmaps, not colour-keyed: the heal potion 0x212, magic potion 0x214,
double wand 0x210, triple 0x216, single X 0x215, double X 0x211, triple X 0x217 (2 frames each:
on, greyed out) and exit 0x219; colour-keyed: the board MAGIC SHOP 0x221, the shopkeeper 0x213,
the cursor 0x21b and the small cursor 0x21a (2 frames: blink), WAND 0x21e / 0x21f and MAGIC LEVEL
0x220 / 0x21d (player 1's, player 2's), the wand names 0x21c (6 frames, by `wandType`); the
mage icons are the status display's 0x1fd / 0x1fe. The number uses the digits 0xba, 1 digit. The
back is a `JObject3D_Particle` 300 × 140 with texture 0x218, alpha state 3, alpha 0.75.

`Reset`: item 0 the heal potion (20), item 1 the magic potion (100), then by the level status's
world: World 1 single X (100) and double (200); World 2 double (200) and double X (300); World 3
double X (300) and triple (400); World 4 triple (400) and triple X (500). `pHero` null, counter 0,
NON.

`Start(hero)` (from `XStaticObj_Shop`): the back at (220, 150, camera look-at Z), `pHero`, every
item on, `_CheckEnabledItems`, the cursor on exit, counter 0, FADEIN.

`_CheckEnabledItems` only turns items off: price above the gold; the heal potion at life ≥ 5; the
magic potion at magic level ≥ 7; item 2 or 3 when it is the wand held. (Buying the better wand
leaves the cheaper one on sale.)

`_UpdateInput` (pressed this frame, through the application's `JDInput`; the joystick unless
`RegInfo_GetDisableJoystick`): hero A (`IsHeroA`, vtable +4) the arrows and gamepad 0, button
Right Shift, Right Ctrl, or pad 0's button 0 or 1; hero B D, A, W, S and gamepad 1, Left Shift,
Left Ctrl or pad 1's buttons. Enter does nothing in the shop.

`ProcessFrame(n)` (after the game's ticks, see below): nothing when NON; counter += n. FADEIN /
FADEOUT: at 40 the counter is 0 and the state PROCESS / NON. PROCESS: `_UpdateInput`; Escape
puts the cursor on exit and sets the button. Then only the first of: left (1 → 0, 3 → 2), right
(0 → 1, 2 → 3), up (2 → 0, 3 → 1, 4 → 3), down (0 → 2, 1 → 3, 2 or 3 → 4), each playing 0xec even
when the cursor stays; the button: on exit 0xed, counter 0, FADEOUT; on an item that is on, 0xed,
then life + 1, magic level + 1, or `wandType` = the item's type, `XHeroBase::UpdateMagicLevel`,
`DecreaseGold(price)` (not below 0) and `_CheckEnabledItems`. A greyed item does nothing.

`Render` (after the status display, before `RenderFade`; only when not NON): `t` = counter / 40
fading in, (40 − counter) / 40 fading out, else 1, kept to 0..1. The back at (220, 150, look-at
Z), alpha 0.75 t, drawn facing the camera. Left x = 50 − (int)((1 − t) · 330): the board at
(x, 152); below it (y = 152 + board height + 20) the shopper's mage icon at x, MAGIC LEVEL at
x + 42 and the number (magic level + 1) 4 after it; 6 below that WAND at x + 42 and the wand's
name 4 after it. Right x = 640 − (int)(t · 260): the items at (x, 147), (x + 107, 147),
(x, 271), (x + 107, 271), frame 1 when off; exit at (x + 107, 395). While PROCESS the cursor (or
the small cursor on exit) 11 up and left of the item, frame `counter % 40 < 20 ? 0 : 1`. The
shopkeeper at (100, 480 − (int)(height · t)).

In `XGameObject::ProcessFrame`, after the level's own logic: when `XShop::GetState` is NON, `n`
ticks of play; otherwise `n` ticks of only the status display and the camera, then
`XShop::ProcessFrame(n)`. The state is read once a frame, so a shop opened during a tick lets the
rest of that frame's ticks run. The Escape check of the game itself still runs while the shop is
open, so Escape in the shop also makes the shopper (and the other hero, if NORMAL) give up the
level once play resumes. The port keeps that quirk. The port also reads the heroes' keys during
the paused ticks, so that a key held from the shop isn't taken as freshly pressed when play
resumes. The original reads them through its once-a-frame `JDInput`, which behaves the same way.

### The ending (`XEndingObject`, `XCamera::ResetForEnding`)

Fields (after `XProcessObject`): +0x90 the camera, +0x94 / +0x98 the film strips (upper 0x2cb,
lower 0x2ca, not colour-keyed), +0x9c four credit sprites (0x2cc, 0x2cd, 0x2ce colour-keyed;
0x2cf, the Saito Games logo, not), +0xac / +0xb0 the left and right walls, +0xb4 the floor
(`JObject3D_Rect`s), +0xb8 / +0xbc the mages' bodies, +0xc0 / +0xc4 their wands
(`JObject3DX`), +0xc8 / +0xcc their shadows (`JObject3D_Shadow`), +0xd0 `textureOffset`, +0xd4
`localCounter`, +0xd8 `creditIndex`, +0xdc `creditPosX`, +0xe0 `creditState` (`CREDITSTATE`:
NONE 0, WAIT1 1, APPEAR 2, WAIT2 3, DISAPPEAR 4, WAIT_FOR_INPUT 5).

The constructor plays Music01 (`JMidiList::Play(0)`), makes the bitmaps and the camera
(`ResetForEnding`: distance 500, vertical angle 0, horizontal angle π, looking at (0, −60, 0),
eye base the origin, so the title's view lowered by 60). The floor is a 200 × 12800 rect moved
50 down, texture 0x2c8. Each wall is 12800 × 200, turned −90° about X, then −90° (left) or +90°
(right) about Y, moved (∓99, 50, 0), texture 0x2c7. All three use perspective correction and
linear filtering. Mage A is body 0x163 with frames 0x164, 0x163, 0x165 at (−40, −50, −280).
Mage B is 0x168 with 0x169, 0x168, 0x16a at (40, −50, −280). Each has wand model 0x1b7 at the
same place. All face the camera (yaw π), at scale 1. A 40 × 40 shadow (texture 0xa3) sits 0.2
above each mage's feet. `creditIndex` −1, `creditPosX` 0, `creditState` 1, `StartFadeIn(0)`.

`ProcessFrame(n)`: tick and `localCounter` += n, `textureOffset` += 0.008 (once per call, so per
frame, not per tick: see "Stage 7: frame-rate dependence" in `ROADMAP.md`), `JDInput::UpdateInputState`. FADEIN: at 254 `StartProcess(0)`. FADEOUT:
at 254 state 3 (the logo). PROCESS: with no credit yet and tick ≥ 300, credit 0 in WAIT1 (tick
0). Then for credits 0–3: WAIT1 sets x 820 and after 200 ticks APPEAR; APPEAR sets x =
820 − (int)(tick / 200 · 500) and after 200 ticks WAIT2, or WAIT_FOR_INPUT for the last credit;
WAIT2 sets x 320 and after 500 ticks DISAPPEAR; DISAPPEAR sets x = 320 − (int)(tick / 200 · 500)
and after 200 ticks moves to the next credit at x 820 in WAIT1. WAIT_FOR_INPUT: `CheckButtonPressed`
or Escape plays 0xed and `StartFadeOut(0)`. Each change of state sets the tick back to 0.

`Render`: the camera; the floor's UV (u 0–5, v `off`–320 + `off`), the left wall's (u −`off`–
320 − `off`, v 0–1) and the right wall's (u `off`–320 + `off`, v 0–1), each drawn after; the two
shadows; the two wands; the bodies on frame ((`localCounter` − 1) / 9 + 1) % 4 (unsigned); the
film strips, 9 tiles 80 apart, x = i · 80 − ((int)(`localCounter` · 0.25) % 80), the upper at y 0
and the lower at y 360; the current credit centred at (`creditPosX`, 240); `RenderFade`.

## Behaviour to preserve

- Fixed-step game logic (tick rate to be confirmed from `XApplication::FrameMove`), with any
  random-number generator reproduced exactly.
- Two-player co-op: `XHeroA` and `XHeroB` read separate inputs (`_UpdateInput`); see "Player 2".
- Save slots (`XSelectSlot`, `XSaveData`) and settings via the registry (`JWinReg`). The port
  keeps its own save files in the per-user folder (`%APPDATA%\MageBrosNative\saves`, or the
  ini's `save_path`) and does not import the original's registry saves (the owner's decision).

## The hero (XHeroBase, XHeroA)

Member names and offsets come from the PDB's type records (`XHeroBase`, 172 bytes).

States: `liveState` NON 0, NORMAL 1, WARP 2, DYING 3, DEAD 4, COMPLETED 5; `jumpState` LAND 0,
JUMP 1, FALL 2. `XHeroBase::Reset` puts the hero at its start position lifted to y = 800, facing
yaw 0 (along +Z) with no pitch, in NORMAL / LAND, with every counter, push and knockback cleared.
The game runs `ProcessFrame` once per 10 ms tick.

Per-player status (`XHeroStatus`): gold (short), life, magicPoint (0–10), magicLevel, wandType.

### One tick of `XHeroBase::ProcessFrame`

1. Nothing happens in NON, DEAD or COMPLETED. Otherwise `localCounter` counts up.
2. After damage the body and wand flash white until `damageCounter` runs out: white while
   (counter / 12) is even, so 12 ticks white, 12 normal. `JObject3DX::UseWhiteMaterial` passes the
   flag to `CD3DFile`, whose `CD3DFileObject::Render` then sets a white material (diffuse and
   ambient 1, emissive 0.3, made in the `CD3DFileObject` constructor) in place of each mesh
   material.
3. **NORMAL:**
   - The new position starts as the old one plus any `push` (fans, a hero standing on the
     other), which is then cleared. Knockback moves it by `knockBackSpeed` along `knockBackDir`,
     turns the hero to face against it (easing factor 0.1), and slows by 0.06 per tick.
   - Input is read (a virtual call). Steering needs knockback below speed 1. The keyboard gives
     strength 0.9; the stick its length, capped at 0.9. Below 0.5 the hero stands still and the
     walk counter resets.
   - Direction: keys give one of 8 directions in 45° steps (right = 0 = +Z, down = 90° = +X
     towards the camera, left = 180°, up = 270°); the stick gives `atan2(y, x)` of its axes
     (each the DirectInput value / 1000). The hero moves along that direction at once by
     1.2 × (0.2 + 0.8 × (strength − 0.5) / 0.4) units, while the facing eases towards it
     (factor 0.15, at least 3° per tick). The walk counter counts up while moving.
   - Gravity: vertical speed falls by 0.08 per tick, down to −4.
   - X then Z are resolved separately, each against the other hero, static objects, candles,
     platforms and the map, in that order.
   - A hero standing on this one is pushed by this one's movement.
   - Falling: the hero lands on the other hero (unless a fan is pushing it), static objects,
     platforms, the map or candles; landing on a platform registers the rider with it. Rising:
     only the other hero, static objects and candles stop the head.
   - Fire: on a fresh press with the wand down and at least 1 magic point, the hero's own fire
     sound plays, one point is spent and a shot is made; the wand stays up while the button is
     held. Points come back one per `mpRecoveryFrames` ticks, up to 10.
   - Jump: on the ground pitch eases back to 0; pressing jump plays the jump sound and sets the
     vertical speed to 3.5. Rising tilts towards +8°; a head hit stops the rise; it turns into a
     fall when the speed reaches 0. Falling tilts towards −16°; landing at full fall speed (−4)
     plays a landing sound and a dust effect.
   - Below y = −200 the hero dies (in water levels below −58, with a splash). Within 20 units of
     the goal once it is open, the warp starts.
4. **WARP:** a sound, then the hero rises at 0.8 per tick while spinning (8° per tick to start),
   with sparkles every 25 ticks; above y = 320 the level is completed.
5. **DYING:** a sound and a hop (speed 5) while spinning 14° per tick; once the fall passes −3
   the hero bursts: an enemy-explosion effect at the hero's centre (y + 14.5),
   `XHeroFireList::Create8Way` (despite the name, twelve ordinary shots at 15° + 30° × i), sound
   0x26e, and the gold loss. With a boss in the level the hero loses a tenth of the gold;
   otherwise everything up to (5 − life) × 3 = 15 gold, plus a tenth of the rest
   (`(int)((gold − 15) × 0.1) + 15`). `XStatusDisplay::ShowLostGoldMessage` shows the amount,
   life goes back to 5 and magic to 10, and the hero is DEAD.

### Drawing

Body frames: 0 standing; walking cycles frames 1, 2, 3, 0, changing every 9 ticks; 4 rising;
5 falling. The wand uses frame 2 × wandType, or 2 × wandType + 1 while raised. Both models get
the hero's pitch, yaw and position.

### Controls (`XHeroA::_UpdateInput`, `XHeroB::_UpdateInput`)

Player 1: arrow keys move. Fire: Right Ctrl, `[` or joystick 1's button 1, plus Left Ctrl in
one-player games. Jump: Right Shift, `]` or joystick 1's button 2, plus Left Shift in one-player
games (the one-player flag is `XApplication` + 0x1a4).

Player 2: D, A, W, S move (right, left, up, down: the same directions as the arrows). Fire: Left
Ctrl, `1` or joystick 2's button 1. Jump: Left Shift, `2` or joystick 2's button 2.

For both, the stick (joystick 1 or 2) is only read while no direction key is held, and not at all
when the joystick is disabled in settings.

### Player 2 (`XHeroB`)

`XHeroB` is `XHeroA` with other models and keys: body MageB (0x168, then frames 0x169, 0x168,
0x16a, 0x16c, 0x16b), the same MageCOL (0x179) collision mesh and the same wand. `IsHeroA` (a
virtual returning 1 or 0) picks everything else that differs:

- the status (`XApplication::GetHeroStatus(IsHeroA())`): each player has their own gold, life
  and magic, kept across levels; `Damage`, `IncreaseGold`, `IncreaseLife` and the gold lost on
  death change the hero's own;
- the shot (`XHeroFire` keeps its `ownerHero`): texture 348 / 388, smoke 0x15d / 0x185, the
  flash likewise; an enemy hit by a shot flinches away from the shot's owner;
- the warp photons (they circle their own hero), the lost-gold message (top or bottom line) and
  the platform mark (`SetHeroAAbove` / `SetHeroBAbove`).

`XGameObject::XGameObject` always makes both heroes. In a one-player game it sets only hero A's
start (100, 0, 60) and `InitLevel` never resets hero B, so B stays NON for the whole game: not
processed, not drawn, and skipped by every check below (they all require NORMAL, or skip NON).

Everything that looks for a hero checks both, A first:

- every enemy, boss, bullet and the wizard's blade test A, then B (`_CollisionCheck_Hero`,
  `_PushHero`, the fans' `_PushHero`); bullets that stop on a hit stop on whichever hero they hit
  first;
- `XGameObject::GetCloserHero(pos)`: the distances to each hero in NORMAL (others count as
  `FLT_MAX`); B only when strictly closer, so with neither in NORMAL it returns A.
  `FindDistanceToCloserHero` / `XZ` return the smaller distance, `FindDirToCloserHero` the
  direction to `GetCloserHero`. The positions passed are the enemy's own (the stalactite: its
  start; the jump needle: its trigger point; the bird: the nest's);
- the dragon's battle starts when either hero passes the line; the shop door opens for each hero
  separately (each has its own "finished" flag);
- coins: `XCoin::ProcessFrame` tests A, then B, so a coin both touch goes to player 1; torches
  burn either hero;
- a shot stops on either hero's box (`XHeroBase::CollisionCheck`, NORMAL only);
- platforms: `_UpdatePosX` / `Z` are stopped by either hero's box; `_PushHeroX` / `Y` / `Z`
  carry each hero whose flag is set, and `_PushHeroY` also moves the other hero up or down when
  the carried one's `HeroAbove()` is set; `_CollisionCheckY_Hero` lifts each hero in NORMAL out
  of the platform's box and then lifts the other hero out of that hero's box (the boxes' own
  `CollisionCheckY`, with no state test on the other hero).

#### Hero on hero (`XHeroBase::ProcessFrame(otherHero)`)

`XGameObject::ProcessFrame` runs `heroA->ProcessFrame(heroB)`, then `heroB->ProcessFrame(heroA)`.
The other hero's box is a solid like any other, through `XHeroBase::CollisionCheckX` / `Y` / `Z`,
which test only while that hero is NORMAL (X and Z with heights counted):

- X and Z: the other hero is tested first, before static objects, candles, platforms and the map.
- `heroAbove` (+0x40) on a hero means "the other hero stands on me". After its own X and Z move,
  a hero with `heroAbove` set calls `other->IncreasePushX` / `Z` with how far it moved this tick
  (from its position at the start of the NORMAL tick, so its own push, a platform's carry, is
  passed on too). The rider moves by that amount on its next tick.
- Falling: `onHero = other->CollisionCheckY(newY, myBox) && !pushedByFan`; the box snaps newY
  even while a floor fan lifts this hero, but that does not count as landing. `onHero` counts as
  landed like the other solids, and `other->SetHeroAbove(onHero)`. Rising: the other hero's box
  stops the head like the others, and `other->SetHeroAbove(0)`.
- A hero standing on the other is falling each tick (speed −0.08 after gravity), so it renews
  `heroAbove` every tick; walking or jumping off clears it. `Reset` clears `heroAbove`.

The status display draws player 2's panel (face, LIFE, GOLD, MP at x 260) when the one-player
flag (`XApplication` + 0x1a4) is clear. `XStatusDisplay::Render`'s code for it is player 1's,
instruction for instruction, with player 2's sprites, `GetHeroStatus(0)` and hero B's DEAD test
(no hearts while dead; the magic lights all off).

`XShadowOnPlatform::Render` draws `RenderShadow(heroA, shadow[0])`, then
`RenderShadow(heroB, shadow[1])`: each hero's own clipped shadow on every platform top under it.

### Helpers

- `NormaliseAngle` wraps into [0, 2π). `MakeAngleCloseTo` normalises both angles, goes the short
  way round, then eases with `MakeValueCloseTo` and normalises the result.
- `ApproxDistance` takes the Manhattan distance and refines it with two Newton steps of the
  square root; the warp check uses it, so the port keeps it rather than an exact distance.

## Collision with the map

`XCollideBox`: an axis-aligned box. `pos` is the bottom centre; `node1` and `node2` are its low
and high corners (x ± half width, z ± half width, y and y + height). The hero's box is 26 wide
and deep and 29 tall.

Cell tests: `IsBlocked(x, z, level)` is true outside the map or where the floor is higher than
`level` (gaps never block); `IsInvisibleBlock` is the `!` object code.

- **X** (Z is the same with the axes swapped): no movement, no test. Otherwise the cell under
  the leading edge is tested in three rows (the box's low edge, centre and high edge in Z) at the
  box's current height step. A blocked cell puts the box flush against it. After a hit, all six
  cells under both edges are tested again; if any is still blocked the move along this axis is
  cancelled.
- **Y**, only while moving down: the 3 × 3 cells under the box (both edges and the centre, in X
  and Z) at the new height step. A solid one lands the box on top of that step, at
  (step + 1) × 25; the same nine cells are then tested at the corrected height, and if any is
  still solid the vertical move is cancelled. A single-point version does the same for one spot.

## The level loop (XGameObject, XMap, torches, shots, coins)

### Game object (`XGameObject::ProcessFrame`)

Order of updates each tick: status display, map, water, particles, hero shots, enemy bullets,
hero A, hero B, enemies, boss, coins, torches, platforms, static objects, candles, camera.
Escape makes each hero in NORMAL give up (`StartDying`). Each hero's `ProcessFrame` gets the
other hero (for the hero-on-hero collision).

A level runs as a process with a type:

- A hero COMPLETED (the warp finished): if this isn't the world's last level, type 1 (after 200
  ticks fade out, then the next level of the world); after the last level of a world, type 2
  (advance the world, back to level select); after the last world, type 3 (advance, save, the
  ending).
- A hero DEAD: type 4 (after 300 ticks fade out and return to level select). There are no lives;
  the player picks a level again.

With two players the level is cleared (types 1 to 3 as above) when A and B are both COMPLETED, or
one COMPLETED and the other DEAD; it is lost (type 4) only when both are DEAD. A hero who dies
stays out until the next level while the other plays on.

### The goal (`XMap::ProcessFrame`)

The map counts ticks. Once more than 300 have passed and the goal isn't open yet, it opens when
all torches are lit, or, in a level with a boss, when the boss is defeated (torches are then not
needed). While open, a warp ring rises every 60 ticks and the warp sound (WAVE 234) plays every
120 ticks at the goal. `CloseWarpZone` (called when a hero starts warping) resets the counter
and closes the goal.

### Torches (`XTorch`, up to 30)

`*` makes a large torch, `+` a small one. Both stand at their cell's floor, turned 90°, and are
also solid posts for the heroes (a static collide box 28 wide, 76 tall for large, 50 for small).
Their flame point is above the post (y + 76 or + 50), with the flame sparks spawned 8 further
along X and 14.5 higher. A hero shot within 40 along Z and within 14 + the shot's radius
(`ApproxDistance`) of the flame point lights the torch: WAVE 620 and a flame burst; the torch
list counts the lit torch. A lit torch throws a spark every 12 ticks and **burns** a hero in
NORMAL who comes within 80 along Z and 24 of the flame point (damage and knockback).

### Shots (`XHeroFire`, up to 30)

A fire press (with the wand down and magic to spare, see the hero) calls
`XHeroFireList::CreateFire`, which picks the pattern by the shooter's `wandType` (each angle
through `NormaliseAngle`):

| wandType | Function | Shots |
|---|---|---|
| 0 | `_WandNormal` | one along the yaw |
| 1 | `_WandNormal2` | two, yaw ± 0.10472 (6°) |
| 2 | `_WandNormal3` | one along the yaw, then two small ones at yaw ± 0.20944 (12°) |
| 3 | `_WandChase` | one homing shot along the yaw |
| 4 | `_WandChase2` | two homing, yaw ± 0.139626 (8°) |
| 5 | `_WandChase3` | one homing along the yaw, then two small ones at yaw ± 0.279253 (16°) |

`_CreateFire_Normal(hero, angle, large)` / `_CreateFire_Chase` take the first free slot (none:
no shot) and make an `XHeroFire` / `XHeroFire_Chase`. `Create8Way` (the hero's death burst) is
12 plain large shots whatever the wand. A shot starts 20 ahead of the hero along its angle at hero
height + 12, radius 8, drawn as an 18 × 18 glowing sprite (texture 348 for player 1, 388 for
player 2). A small shot (`large` 0) has its sprite scaled by 0.8 and radius 6.4, and its smoke
and flash are the small ones (`large` is passed to `CreateHeroFireSmoke` / `Flash`).

**`XHeroFire_Chase`** (+0x38 `targetEnemy`, +0x3c `targetBoss`): when made, the target is
`XEnemyList::GetClosestEnemy(point 100 ahead of the shot along its angle, 200)`, which looks at
the enemies up to the first empty slot that are `Chasable` (slime, bee, wallaby, mushroom, squid,
kiwi, turtle, ghost) and NORMAL, skips those more than 200 from the point along Z, and keeps the
strictly nearest by `ApproxDistance`. With none, the boss, if it exists, is NORMAL and is less
than 200 from the shot along Z. `ProcessFrame`: once `localTime` > 10 (before the base class counts
the tick), the angle turns towards the target (`FindDirectionXZ` from the shot) by at most
0.0279253 (1.6°) a tick with `MakeAngleCloseTo_Constant`, the enemy first, else the boss. The
target is never dropped, so a shot keeps turning towards a defeated enemy's last position. Then
`XHeroFire::ProcessFrame`. Each tick it moves 2.6
along its angle; after 20 ticks it also falls (vertical speed −0.06 per tick); every 2 ticks it
leaves a smoke puff. It ends, with a flash, on the first of: the map (`IsBlockedFloat`), the
other hero, static objects that block fire (their box tested against the shot's centre point,
the radius not counted; a torch post blocks fire, so a torch is lit by jump-firing over it), platforms, enemies, enemy bullets, candles or the boss; on a torch
(which lights, no flash); below y = −200; or in water below −38 (a splash).

### Coins (`XCoin`, up to 200)

Types: small (model 384 at 0.35 scale), medium (384 at 0.6) and heart (684 at 0.6). Gold: small 3
and medium 30 in a one-player game, 5 and 50 with two players; a heart gives a life instead. The
radius comes from the model's size; each coin starts at a random angle and spins at 1.5° per tick
easing towards 1.8°. Pickup, for a hero in NORMAL: a quick box test, then the original's exact
test, a sphere (the coin's radius, at its centre) against the hero's mesh with ColDet. Then WAVE
616, a sparkle, the gold or life, and the coin is gone.

### Magic

`UpdateMagicLevel` clamps the magic level to 0–7 and sets the recovery time per magic point to
80, 70, 60, 50, 40, 32, 24 or 16 ticks.

### Positioned sounds (`XApplication::PlaySound`, `JSound::PlayStereo`)

Sounds are panned by their Z offset from the camera's look-at point: pan = (sound z − camera
z) / 240, positive to the right (the camera looks down −X, so +Z is screen right). Beyond ±2
(480 units) they aren't played. `JSoundList::PlayStereo(id, pan, volume 1)` then:

- clamps the volume to 0–1 and the pan factor to ±3, and plays nothing at volume 0 or pan ±3;
- fades the sound past one unit of pan: factor (3 − |pan|) / 2, and sets the DirectSound
  volume to factor × volume × 10000 − 10000 hundredths of a decibel (a linear share of 100 dB,
  so the fade is steep: half the factor is −50 dB);
- sets the DirectSound pan to |pan| (at most 2) × 5000, negative for the left, which takes that
  many hundredths of a decibel off the other channel;
- plays the next of the sound's copies. `XApplication` creates 1 to 6 copies of each sound (6 of
  0x262, 4 of 0x270, 0x278, 0x279 and 0x2a5, 2 of eleven others), so a sound can overlap itself
  that many times before the oldest is cut off.

## Enemies and static objects (`XEnemyList`, `XStaticObjList`)

**Level codes** (`XMap::_ConstructObjects`, each cell in turn, at the cell centre and
y = floor level × 25, negative over a hole): `#` goal, `$` coin, `*` `+` torches, `/` fence,
`A`–`G` platforms, `H`–`Z` static objects, `a`–`z` and `0`–`5` enemies, `6`–`9` bosses.

| Code | Class | Code | Class |
|---|---|---|---|
| a / 1 / 5 | Slime (small / king / queen) | b / 3 | Bee (small / king) |
| c / 4 | Squid | d / 0 | Wallaby |
| e | Turtle | f | Kiwi |
| g | Ghost | h / 2 | Mushroom |
| i | Fish | j | Barrel |
| k | Hammer | m | NeedleFan |
| o / p | Needle (fixed / moving) | q r s | Fan (directions 4, 0, 2) |
| t | FloorFan | u | Stalactite |
| v / w | FenceDoor | x / y | NeedleBall |
| z | JumpNeedle | H I J | Cannon (directions 4, 0, 2) |
| K | CannonR | M N | Pot (large, small) |
| O | Barrel (object) | P Q | Pillar (large, small) |
| R | RocketLauncher | S | Shop |
| T U | Tree, Plant | 6 7 8 9 | Dragon, Genie, Nest, Wizard |

World 1 uses slimes, bees, wallabies, fish, barrels, needle fans, needles, fence doors, the
object barrel, pillars, cannons, the shop, fences, platforms A C D E G and the dragon.

**Fences** take their shape from the four neighbouring cells, counting `/`, `v` and `w` as
fence: left and right (x ± 1) and the rows either side (z ± 1). One of a side and one of a row
gives a corner ('1'–'4'), three sides a T ('5'–'8'), only left/right a straight piece along X
('X'), only rows one along Z ('Z'). Each is 100 tall and 10 thick: the straight pieces 40 long
(model 0x1d9), the corners (0x1d8) and Ts (0x205) a 30-long box half a cell (10) to one side
plus one or two extra 30-long invisible boxes (`CreateExtraCollideBox`).

**Static objects** (150 slots, loops stop at the first empty one): pillar `P` 27 × 146.5
(model 0x170; 165 and 0x292 in world 3; over a hole 180 tall from y = −38, model 0x1ef), small
pillar `Q` 20 × 78 (0x1db), barrel `O` 27 × 42.8 (0x1f6, a 37 shadow), and in World 2 the tree
`T` (0x1d6 turned −90°, a 35 × 5 box at its foot plus an invisible 12 × 134.4 trunk box, no
shadow) and the plant `U` (0x1d7 turned −90°, 24 × 20, a 28 shadow); in World 3 the large
pot `M` (0x1d4, 32 × 35, a 40 shadow, plus an invisible 17 × 80 box from
`CreateExtraCollideBox`) and the pot `N` (0x1d5, 30 × 40, a 38 shadow). The rocket launcher `R` (0x197, 34 × 40, no
shadow) calls `XBulletList::CreateRocket(pos)` whenever its counter is a multiple of 1000
(first at tick 1000). `CreateObject` calls each
new object's `CreateExtraCollideBox` (a no-op except for the tree, the large pot and the turning cannon); fences
get theirs from `CreateFence`. They stop heroes (heights
counted) and enemies (heights ignored); enabled ones that block fire stop shots that are
strictly inside the box.

**`XCollideBox::CollisionCheckX/Z(newValue, mover, ignoreY)`**: a loose prefilter on the Z
distance (the two full Z widths; the Z test uses the Z distance too), then the mover's box at
the new value against this box. On overlap the mover goes to the far side if its low edge was
inside, else the near side; if it still overlaps it stays where it was. `CollisionCheckY` is
the same with landing on top / bumping the underside.

**Enemies** (110 slots, never removed). `XEnemyBase` holds an ID, size (SMALL, KING, QUEEN),
live state, counter, start and current position, direction and angles, health, floor level,
frame, damage counter, the flags damagable (default on), chasable, collidable (on) and
"no collision while the hero flashes", vertical speed, the model (with a ColDet collision mesh),
a simple shadow and a box.

- Moving (`_UpdatePosX/Z`): against the other enemies (boxes, heights ignored, skipping itself
  by ID), static objects, platforms and the map with `XMap::CollisionCheckX_Floor`: the cells
  ahead must be on the enemy's own floor level and not `!` (invisible blocks in the object
  field).
- Hero contact (`_CollisionCheck_Hero`, both live): boxes overlap, then the meshes collide
  (the hero's is MageCOL 0x179); the hero is damaged and knocked back from the enemy.
- Shots (`CollisionCheck_Fire`): |Δz| and |Δx| each within the box's **Z** width + 8, then the
  shot's sphere against the mesh; the shot stops (and flashes) even if the enemy can't be hurt.
- `_Damage` (damagable only): sound 0x270 at the shooting hero, white for 12 ticks, health − 1;
  at 0 DEAD: sound 0x27d and a hop at speed 4 spinning 14° a tick (gravity 0.08); below −3 it
  bursts at mid-height (explosion, large for KING/QUEEN), drops a coin and plays 0x26b.
- Only enemies within 480 of the camera's Z move (`XCamera::IsUpdatable`); within 400 are drawn
  (`IsVisible`).

**Enemy coins** (`XCoinList::CreateEnemyCoin`): SMALL drops a small coin (3 gold, 5 in two-
player games, model 0x180 at 0.35), KING a medium one (30 / 50, 0.6), QUEEN a heart (0x2ac, a
life). It spins at 15 slowing by 0.02 a tick to 1.8, and bounces: up at 3, gravity 0.08 (to −4),
off the floor at 0.8 of its speed (sound 0x27b above 1.6), settling below 0.8. Every coin has
radius = scale × 20 and a shadow of scale × 40; level coins spin at 1.5.

**Slime** (`a` 34 × 18, 1 health, model 0x16f + frames 0x16e 0x16d 0x16e, collision 0x177, shadow
34; `1` king 68 × 36, 20 health, 0x1e1..0x1e4; `5` queen 163.2 × 86.4, 120 health, 0x2c2.., starts
at x = 100). A random direction; walk 40 ticks at 0.5 (king 0.75, queen still), frames every 10
ticks; wait 10; after 2–6 waits a long wait of 60 ticks that turns the direction up to 120°
towards a random point (x 20–180, within 140 of the start along Z); after 3–6 long waits a
jump instead, at the closer hero if within 120: 100 ticks (queen 120), up to 60 high (king 90,
queen 150) following sin, slow (0.2) going up and fast (1.1) coming down (king × 1.5, queen in
place), tilting −20° then +20°. Its yaw eases towards the direction (0.06; queen 0.02).

**Needle** (`o` fixed, `p` moving): model 0x18a, collision 0x1c8, box 34 × 70, sunk 40.5. A
moving one sinks 30 over 50 ticks (sine), waits 200 − 20 × world ticks, rises over 50, waits
again. It can't be hurt, and doesn't hurt a hero who is still flashing.

**Stalactite** (`u`): model 0x1cf, collision 0x1d0, box 40 × 120 at the start, shadow 40, SMALL,
not damagable, a random yaw, `_SetPosY(220)` (an absolute height). No `IsUpdatable` test.
`MOVESTATE` NON / WAIT / DROP / STAY, starting in WAIT. WAIT: with the closer hero (whatever its
live state), |Δz| ≤ 60, |Δx| ≤ 60 and `ApproxDistanceXZ(start, hero)` ≤ 60 → DROP. DROP: tick 1
its own verticalSpeed 0; the yaw + 4° every tick; after tick 80 verticalSpeed − 0.08 (at least
−4) and `_SetPosY(y + verticalSpeed)`, sound 0x277 at tick 100; below start.y − 45: sound 0x276,
`XCamera::StartShaking(2)`, `CreateLandEffect` at (x, start.y + 0.5, z) → STAY, which does
nothing (it stays in the floor and still hurts).

**Jump needle** (`z`): model 0x293, collision 0x1c8 (the needle's), box 34 × 70 at the start, no
shadow, SMALL, not damagable, "no collision while the hero flashes" on, no `IsUpdatable` test;
checkPos = its start. `_SetPosY(y − 40)`: sunk. `MOVESTATE` NON / WAIT / WAIT2 / MOVEUP /
MOVEDOWN, starting in WAIT. WAIT: once the counter passes 150, if the closer hero is within 64
of checkPos along Z and NORMAL → MOVEUP. MOVEUP (30): y = start + sin(t/30 · π/2) · 40 − 40,
sound 0x2ae at 10 → WAIT2 (200) → MOVEDOWN (150): y = start − 40 · t/150 → WAIT. (`movable`
is never used.)

**Bee** (`b` 31 × 55, 2 health, frames 0x171 0x174 0x173 0x174, collision 0x178, shadow 31;
`3` king 62 × 110, 51 health, 0x2b8 0x2b7 0x2b6 0x2b7, 0x2b9, shadow 62). Hovers 5 above the
floor, bobbing ±4 over 80 ticks (its own counter, started at a random 0–79; frame = counter / 4
mod 4). It flies at 0.5 (king 0.6) in 120-tick legs, turning at most 1° a tick towards a random
point (x 20–180, within 140 of its start along Z) or, if the last leg left it within 10 of where
it began, a point 40 away 120–240° off its course (`_GenerateBehindPos`); within 120 of a hero
(and not turning round) it turns towards the hero instead. Yaw = direction.

**Wallaby** (`d` 35 × 44, 2 health, frames 0x1a7..0x1a9, collision 0x1aa, shadow 35; `0` king
70 × 88, 45 health, 0x1e5..0x1e7, 0x1e8, shadow 70). Waits `RandomI(60, 240)` ticks, picks a
target (random, or behind after being stuck), notes whether a hero is within 120, then makes
`RandomI(2, 6)` hops of 80 ticks: on the first tick it turns up to 40° (king 48°) towards the
target, or 60° (72°) towards the hero; for 70 ticks it moves 0.65 (king 0.975) a tick along its
direction at floor + sin(t/70 · π) × 44 (king 66), frame 1 tilting to +10° then frame 2 to
−12°; then 10 ticks on the ground (frame 0, pitch easing back). A hop that moved it less than 10
makes it wait and turn round. The yaw eases towards the direction (0.1).

**Mushroom** (`h` 37 × 44, 3 health, frames 0x25d..0x25f, collision 0x260, shadow 37; `2` king
74 × 88, 30 health, 0x285..0x287, 0x288, shadow 74). The wallaby's hop with its own numbers
(same `MOVESTATE`, NON / HOP / WAIT): waits `RandomI(50, 200)`, then `RandomI(3, 5)` hops of 85
ticks: 80 in the air at floor + sin(t/80 · π) × 48 (king 72) moving 0.65 (king 0.975), frame 1
tilting to +10° for the first 40 then frame 2 to −12°, and 5 on the ground. The first tick turns
it up to 40° (king 50°: × 1.25, where the wallaby uses × 1.2) towards the target, or 1.5 times
that towards a hero who was within 120. Stuck hops (< 10) turn it round as for the wallaby. It
checks contact with both heroes.

**Squid** (`c` 40 × 60, 4 health, frames 0x162 0x161 0x162 0x160 0x175, collision 0x176, shadow
40; `4` king 80 × 120, 60 health, 0x2ba 0x2bb 0x2ba 0x2bc 0x2bd, 0x2be, shadow 80). `MOVESTATE`
NON / WALK / JUMP / WAIT, starting in WAIT. Wait: `Random(150) + 100` ticks, frame 4 (crouched)
once the counter passes waitFrames − 40, then JUMP. Jump: 90 ticks (king 110) in place at
floor + sin(t/n · π) × 55 (king × 1.7), frame 0 for the first half then 4; on the first tick
the direction turns 120–240° round after a stuck walk, else up to 120° towards a random point
and then straight at a hero within 120; the yaw eases to it at 0.05 (king 0.03). Then WALK:
105 ticks at 0.9 (king × 1.2) along the direction, frame (t/15 + 1) mod 4, the yaw unchanged.
A walk that moved it less than 15 sets turnAround and jumps; otherwise `Random(2)` picks JUMP
or WAIT.

**Kiwi** (`f`): frames 0x1ad 0x1b4 0x1ad 0x1af, collision 0x1b5, box 35 × 35, shadow 35, SMALL,
4 health, chasable, a random direction. `MOVESTATE` NON / WAIT / WALK / RUN, starting in WAIT.
`_IsHeroClose`: `FindDistanceToCloserHero` < 80. WAIT: tick 1 frame 0 and waitFrames =
`RandomI(100, 200)`; a close hero → RUN, else after waitFrames → WALK. WALK (127 ticks): tick 1
notes the position and a target (behind after a stuck leg, else random); frame (t/16 + 1) mod 4;
the direction turns at most 0.8° towards the target; it moves 0.5. A close hero → RUN; at 127
turnAround = (moved < 10) and then WALK if turning round, else `Random(3)` = 0 → WAIT, otherwise
WALK. RUN (95 ticks): tick 1 notes the position; frame (t/6 + 1) mod 4; the direction turns at
most 3° towards the closer hero's direction + π (away), or straight at it when turnAround; it
moves 1.3. At 95 turnAround = (not turnAround and moved < 10); RUN again if turnAround or a hero
is close, else WAIT. `_Damage`: the base's, then RUN from tick 0.

**Turtle** (`e`): frames 0x1b0 0x1b1 0x1b0 0x1b2 0x1b3 with the model scaled 0.9, collision
0x1b6, box 30.6 × 24.3, shadow 34, SMALL, health 99999, chasable. `MOVESTATE` NON / WALK / WAIT
/ SHIELD, starting in WALK. WALK repeats 150-tick legs: on the first tick it notes its position
and picks a target (`_GenerateBehindPos` after a stuck leg, else `_GenerateRandomPos`); every
tick walkCounter + 1, frame (walkCounter/10 + 1) mod 4, the direction turns at most 0.8°
(0.0139626) towards the closer hero if within 120 and not turning round, else towards the
target; yaw = direction; it moves 0.4. At 150 turnAround = (moved less than 10) and the counter
restarts. `_Damage` is overridden: no sound, flash or health, just counter 0 and SHIELD. SHIELD:
frame 4, after 600 → WAIT: frame 0, after 50 → WALK with walkCounter 0.

**Fan** (`q` `r` `s`: `dir` 4, 0, 2 eighths of a turn, so they blow along −Z, +Z and +X): model
0x1ca, collision 0x1c9, box 70 × 85 and shadow 32 at the start; the fan itself sits 10.8 along
the yaw (dir · 45°) and 44.5 up. Base model 0x1cb at the start with the same yaw, plus a static
20 × 52 box. SMALL, not damagable, "no collision while the hero flashes" on, no `IsUpdatable`
test; drawn with the visibility test, the shadow at the start. ROTATE: roll −9° a tick, then
`_PushHero` for each hero. `_PushHero` (fan NORMAL, hero NORMAL): with c = the hero's centre
(pos + 14.5 up), constants 30, 56, 320. `s`: |Δz| and |Δy| ≤ 56 and fan.x ≤ c.x ≤ fan.x + 320;
then `IncreasePushX(1.0 · cos(min(|Δx|, 320)/320 · π/2) · k)` with d = `ExactDistanceZY` and
k = 1 below 30, 1 − (d − 30)/26 below 56, else 0. `r` / `q`: the same across X and Y, the hero
within 320 ahead along +Z / −Z, `ExactDistanceXY`, and `IncreasePushZ(±1.2 · …)`.

**Floor fan** (`t`, 3-4, 3-5, 3-7 and 4-9): model 0x1cd, collision 0x1cc, box 80 × 20 and shadow
28 at the start, the fan not offset; base model 0x1ce at the start plus a static 16 × 16 box (the
hero can stand on it, inside the blades). SMALL, not damagable, "no collision while the hero
flashes" on, no `IsUpdatable` test; drawn with the visibility test, the shadow at the start.
ROTATE: yaw −10° (0.174533) a tick, then `_PushHero` for each hero; then the world matrix and
the hero collision checks. `_PushHero` (fan NORMAL, hero NORMAL), using the hero's feet (`GetPos`,
not the centre): |Δz| and |Δx| ≤ 58 and hero.y ≥ fan.y + 10; along = cos(min(|Δy|, 320)/320 ·
π/2); d = `ExactDistanceXZ`, k = 1 below 30, 1 − (d − 30)/28 below 58, else 0; the push is
(4.8 + 0.8 · sin((localCounter mod 120)/120 · 2π)) · along · k, and when it is above 0
`IncreasePushY(push)` and the hero's `pushedByFan` (offset 0x4) is set. (`_PushHero` also
loads 30, 58, 320 into locals the code never reads.) Against gravity's −4 cap the hero floats
where 4.8 · cos(h/320 · 90°) ≈ 4, about 120 up, bobbing between roughly 107 and 130 over the
120-tick cycle; standing on the floor beside the fan (y = fan.y) it isn't lifted.

`XHeroBase::pushedByFan` is cleared at the end of each NORMAL tick that leaves the hero NORMAL
(not on falling out or warping), and by `Reset`; its only reader is the falling check against the
other hero: a hero lifted by a fan doesn't land on hero B.

**Ghost** (`g`, 3-5, 3-7, 3-8 and 4-9): models 0x1de (frame 0) and 0x1df (frame 1, mouth open),
collision 0x1e0, scaled 0.8 (set again every tick), box 27.2 × 36 at the start, and an `XShadow`
27.2 (`ghostShadow`, the floor-following shadow, not `XShadowSimple`). SMALL, health 99999,
chasable, not collidable (other enemies pass through it), random `dir` = yaw. `ghostPos` = the
start + 80 up; `offsetCounter` = Random(120), `startAttackTimer` = RandomI(600, 1000). `IsUpdatable`
test; drawn with the visibility test (`Render` also calls the box's debug draw). Each tick: the
state, then in WALK `offsetY` = 5 · sin((++offsetCounter mod 120)/120 · 2π) (frozen in the other
states); pos = ghostPos with y + offsetY, scale, world matrix, hero checks. `_UpdatePosX_Ghost` /
`_UpdatePosZ_Ghost` just store ghostPos.x / z: no collision with anything.
- WALK: at t = 1, dir = yaw, frame 0, prevPosForWalk = ghostPos, target = `_GenerateBehindPos`
  if turnAround else `_GenerateRandomPos`. dir eases to `FindDirectionXZ(ghostPos, target)` by
  0.6° (0.010472) a tick, yaw = dir, ghostPos moves 0.4 along dir. The timer counts down to 0.
  Not turnAround, timer 0 and `FindDistanceToCloserHero(pos) < 120` → WARNING; else at t ≥ 120,
  turnAround = `ApproxDistance(pos, prevPosForWalk) < 10` (pos includes the bob) and t = 0.
- WARNING: dir = towards the closer hero, yaw eases (0.08); at t ≥ 80 → ATTACK.
- ATTACK: at t = 1 sound 0x2b4, frame 1, dir = towards the closer hero (fixed from then on). Moves
  0.75 a tick; ghostPos.y = (1 − sin(t/160 · π)) · 80, an absolute height (on a raised floor it
  snaps to 80 and dips to 0); yaw eases (0.08). At t ≥ 160: timer = RandomI(600, 800), WALK.
- `_Damage(heroPos)` (overrides the base: no sound, flash or health): damagePos = heroPos, t = 0,
  FLINCH. A shot still stops on it.
- FLINCH: at t = 1 frame 1, flinchSpeed 2.8, dir = away from damagePos. Speed −0.07 a tick (not
  below 0), moves along dir; ghostPos.y = `MakeValueCloseTo(y, 80, 0.1, 0.5)`; yaw eases to face
  the shooter (dir + π, 0.08). At t ≥ 40 with ghostPos.y exactly 80: timer = RandomI(400, 600),
  WALK. It drifts about 55 back.

**Hammer** (`k`): hammer model 0x17b (collision 0x17c, box 180 × 120 at the start, never moved)
on a base, model 0x17a; the constructor also adds static boxes 60 × 7 and 35 × 35 at the start.
SMALL, not damagable, yaw 90°, no `IsUpdatable` test and drawn without the visibility test
(`Render` also calls the box's debug draw). `MOVESTATE`: WAIT → MOVEUP when a hero is within 160;
MOVEUP / MOVEDOWN move pos.y (not the box) 0.5 a tick for 60 ticks, to ALERT / WAIT. ALERT turns
the yaw towards the closer hero 1.5° a tick; beyond 160 → MOVEDOWN; within 100 and
`AngleDifference` under 6° → ATTACK_START: pitch = (1 − cos(t/50 · π/2)) · π/2, sound 0x277 at
t = 20, and at 50 sound 0x276, `StartShaking(2.4)` and `CreateHammerEffect` at the start + 60
along the yaw (y + 0.5) → ATTACK_WAIT. There a hero within 12° of straight behind (yaw + π) →
BEHIND_START, else after 150 → ATTACK_END: pitch eases linearly from π/2 to 0 over 120 →
ALERT. BEHIND_START: pitch = cos(t/80 · π/2) · π − π/2 (π/2 to −π/2), the same sounds, the thud
at 80 with the effect 60 behind → BEHIND_WAIT (150) → BEHIND_END (−π/2 to 0 over 120) →
ALERT. The base turns with the yaw. `AngleDifference(a, b)`: both normalised, |a − b|, taking
2π off the larger if that is over π.

**Hammer effect** (`XParticle_HammerEffect`): the landing dust's flat ring (texture 0x295, not
billboarded) at 60, growing by 4 × 0.92 a tick, alpha × 0.92 a tick after 15 ticks, gone at
0.03.

**Needle ball** (`x` X_AXIS, yaw 90°; `y` Z_AXIS, yaw 0): model 0x1f1, collision 0x1f5, box
40 × 40, shadow 38, SMALL size, not damagable. It rolls 0.75 a tick along its axis, pitch
turning 0.0453786 (2.6°) a tick, and when `_UpdatePosX/Z` reports a hit both speeds flip sign.
No `IsUpdatable` test: it moves everywhere. The box is placed at the start position, then pos.y
is raised 20, so the ball's centre is 20 up while its box stands on the floor. The
`XBullet_NeedleBall` class is not this enemy's: only `XBoss_Genie` calls
`XBulletList::CreateNeedleBall`.

**Barrel** (`j`): model 0x1f6, collision 0x204, shadow 37, box 27 × 42.8, plus a static box of
the same size that stops heroes and enemies but not shots. 10 health, KING size, never hurts.
At 0 it bursts at once: explosion, the box disabled, XGameObject's barrel count + 1 (kept for
the whole game), a heart every third barrel, else a medium coin; sound 0x267.

**Fence door** (`v` across X, `w` along Z and turned −90°): model 0x1dc, collision 0x1dd, box
40 × 82 × 10, with a matching static box that doesn't stop shots. 5 health; bursts like a barrel
without a coin. A wooden gate, shorter than the fence. `XMap::_ConstructObjects` builds a fence
piece only for `/`; `v` and `w` are enemies, and count as fence only when a fence piece picks
its shape from its neighbours.

**Shop** (`S`, `XStaticObj_Shop`): model 0x1ab turned 90°, a 112 × 1000 × 120 box, and a sign
(texture 0x1ac, an 84 × 36.96 rect turned −90° about Y and −90° about Z, moved 56 out and 127.4
up, opaque). A NORMAL hero within 15 of the doorstep (60 in front of it in X) calls
`XShop::Start` once; it can start again after the hero has been more than 25 away. Hero A is
tested before hero B, so if both arrive in the same tick, B is the shopper.

**Needle fan** (`m`): a spiked bar, model 0x17f, collision 0x17d, box 200 × 80, shadow 42,
turning (0.6 + 0.1 × world)° a tick on a post (model 0x17e, a static 19 × 87 box). Can't be hurt;
drawn without the visibility test.

## Cannons and bullets (`XStaticObj_Cannon`, `XStaticObj_CannonR`, `XBulletList`)

**Cannon** (`H` `I` `J`: directions 4, 0, 2, in eighths of a turn, so `H` fires along −Z, `I`
along +Z and `J` along +X towards the camera): model 0x18c turned to its direction, shadow 38,
box 33 × 34. Every 550 ticks (`localCounter % 550 == 0`) it fires from 21 above its foot: a
rolling ball (`CreateCannonRBall`, at the direction's yaw) in worlds 1 and 2
(`XLevelStatus::currentWorld <= 1`), a straight ball (`CreateCannonBall`, by direction) in
worlds 3 and 4. Firing plays 0x262 and kicks the barrel's pitch to −8° (−0.139626); on every
other tick it eases back to 0 by 0.15° (0.00261799) a tick.

**CannonR** (`K`): barrel 0x1f4 (yaw starts at 90°) on base 0x1f3, shadow 38, a 32-wide,
22-tall box from 30 above its foot, and an extra 18-wide, 40-tall box at its foot
(`CreateExtraCollideBox`). ROTATE: turns towards the closer hero (`FindDirToCloserHero`) by
0.6° (0.010472) a tick; after 430 ticks, FIRE with the counter reset. FIRE: at tick 60 a
rolling ball from 5 ahead along its yaw and 40 up, sound 0x262 and the kick; at 120 back to
ROTATE. The pitch eases back every tick.

**`XBulletList`** has 50 slots; a new bullet takes the first free one (none is made when all
are busy). `ProcessFrame` runs every bullet and deletes those that reached NON; the loops skip
empty slots. It runs after the hero shots and before the heroes, and is drawn after the static
objects (and candles), before the enemies.

**`XBulletBase`**: position, yaw, pitch, radius, a model, an `XShadow` (the split shadow) and a
debug sphere. `Render` draws the model and the shadow (no visibility test).
`_CollisionCheck_Hero`: the bullet and the hero NORMAL, |Δz| ≤ hero box width Z + radius,
|Δx| ≤ width X + radius, then `CheckCollision_Sphere(pos, radius)` on the hero's mesh
(MageCOL); the hero is damaged and knocked back from the bullet. `CollisionCheck_Fire` (a
hero's shot): |Δz| and |Δx| ≤ both radii, then `ApproxDistance` < both radii. The shot stops;
the bullet carries on.

| Bullet | Model, shadow, radius | Each tick | Bursts |
|---|---|---|---|
| Cannon | 0x18d, 17, 8 | pitch + 4° (0.0698132); 1.4 along its direction; smoke every 11 ticks | after 15 ticks, on the map (`IsBlockedFloat`), a static object, a platform or a hero |
| CannonRBall | 0x1f2, 19, 9 | pitch + 3.5° (0.0610865); falls 0.016 a tick (at most 4), its bottom put on the floor by the point form of `XMap::CollisionCheckY` (which zeroes the fall); 1 along its yaw; smoke every 15 ticks | after 25 ticks, the same; gone below −200; on a water level, below −38 it makes a wave (above −43) and bursts |

A burst makes a `BulletExplode` where the bullet was before the tick's move, and ends the
bullet. The point form of `XMap::CollisionCheckY(y, x, z)`: if the cell at (x, z) is blocked
at the height of y, y becomes (cell level + 1) × 25.

**Rocket** (`XBullet_Rocket`): model 0x196, pitch −90° (nose up), `XShadow` 19, radius 9
(and a collide sphere), `SUBSTATE` MOVEUP / LAUNCH. MOVEUP: y + 0.085 a tick; the yaw turns at
most 1.3° (0.0226893) towards `FindDirToCloserHero`; touching a hero bursts it where it is; at
500 sound 0x262, smoke, speed 3 → LAUNCH (counter 0). LAUNCH: with c = the closer hero's
`GetCenterPos`, before tick 450 the yaw turns at most 0.75° (0.01309) towards c, and from tick 11
to 449 the pitch moves at most 1° (`MakeValueCloseTo_Constant`) towards −`FindPitch(pos, c)`; it
moves speed along yaw and pitch (x/z by sin/cos(yaw) · speed · cos(−pitch), y by sin(−pitch) ·
speed); speed − 0.017 (at least 0.9); smoke every 15 ticks. Then the map, objects, platforms or a
hero burst it at the previous position; below −200 it is gone; at 3000 it bursts; on a water
level below −38, a wave (above −43) and a burst.

## Platforms (`XPlatformList`, `XPlatformBase`, `XPlatform_*`, `XShadowOnPlatform`)

**`XPlatformList`** has 35 slots, filled in order; every loop stops at the first empty slot.
`CreatePlatform` gives each platform the next OID: `A` Elevator, `B` Elevator (up only), `C`
AutoZ, `D` AutoX, `E` RightOnly, `F` Drop, `G` Stay. It is made at the cell centre and y =
floor level × 25 (−25 over a hole), then `SetFloorLevel`. It runs after the torches and before
the static objects, and is drawn after the torches, before the static objects.

The list's checks: `CollisionCheckX` / `Z` (a box, with the caller's ignoreY: the heroes pass
0, the enemies 1), `CollisionCheckX` / `Z` for a platform (skipping itself by OID, heights
ignored), `CollisionCheckY` (clears `lastCollidePlatform`, then remembers the last platform
hit) and `CollisionCheck` (a point inside: bullets and shots).

**`XPlatformBase`**: model, box, `XShadowOnPlatform`, the flags `heroAAbove` / `heroBAbove`.
`Render`: within the visible range, the model then the hero shadows on it. `_UpdatePosX` /
`Z`: `XMap::CollisionCheckX_Floor` (its result is the return value), the other platforms, the
static objects (heights ignored), then each hero in the NORMAL state as an obstacle
(`XHeroBase::CollisionCheckX`: the hero's box against the platform's, heights counted); then
the position. `_CollisionCheckY_Hero`: a NORMAL hero inside the box is put on its top
(`XCollideBox::CollisionCheckY` with y = the hero's y). `_PushHeroX` / `Y` / `Z` add the move to
the push of each hero marked above (and, for Y, of the hero standing on that hero).

| Platform | Model, box | Behaviour |
|---|---|---|
| Elevator `A` / `B` | 0x181 / 0x275 on base 0x182 / 0x274 (both turned 90°, the base fixed at the cell), 40 × 200 | Starts 170 below the cell (its top 30 above the floor). WAIT: a hero on it → MOVEUP. MOVEUP: +0.5 a tick (sound 0x273 when the counter % 20 == 1), clamped at the start height; lifts the heroes; at the top, or when no hero is on it, `A` goes to MOVEDOWN and `B` to WAIT. MOVEDOWN: falls (0.08 a tick, at most 4) to the bottom, carrying the heroes, then WAIT. |
| AutoZ `C` | 0x183 (90°), 40 × 18 | Moves 0.35 + 0.05 × world a tick along Z, starting towards +Z, turning back when the floor's edge stops it; carries the heroes. |
| AutoX `D` | the same | 0.35 a tick along X, the same way. |
| RightOnly `E` | the same | Waits for a hero, then moves 0.35 + 0.1 × world a tick along +Z, carrying the heroes, until the floor's edge stops it for good. |
| Drop `F` | the same | Waits for a hero; 20 ticks later falls (0.08 a tick, at most 4), carrying the heroes; stops below −500. On a water level it stops accelerating below −38 and makes a wave as it crosses −38. |
| Stay `G` | the same | Never moves. |

The elevator, like every state machine here, runs one state a tick (`else if`): a new state
starts on the next tick. Every platform clears both "above" flags at the end of its tick, so they only carry what the
heroes reported in this tick (the heroes run before the platforms). Over a hole a platform sits
at −25, its top 7 below the floor, in the open pit.

**The hero and platforms** (`XHeroBase::ProcessFrame`): the X and Z checks run against the
other hero, the static objects, the candles, the platforms and then the map. Falling, the Y
check runs against the other hero, the static objects, the platforms, the map and the candles;
landing on a platform marks it (`SetHeroAAbove` / `SetHeroBAbove`, by `IsHeroA`). Rising, only
the other hero, the static objects and the candles stop the head: a jump passes up through a
platform. Enemies' `UpdatePosX` / `Z` (heights ignored) and the shots also stop against
platforms.

**`XShadowOnPlatform`**: two 35 × 35 `JObject3D_Shadow` rects (texture 0xa3), one per hero.
For a hero NORMAL or DYING whose y is not below the box top, the hero's 35-unit square is
clipped to the platform's 40 × 40 top (box centre ± 20); the visible part is drawn 0.2 above
the box top, with the matching share of the texture (u from the left, v flipped) and scaled to
it.

## Wall candles (`XCandleList`, `XCandle`)

`XMap::_ConstructWallObjects` walks the rows: a row flagged `C` gets a candle (made at
(0, 0, row × 40 + 20)), `W` turns the water on, `-` nothing. `XCandleList` has 30 slots, filled in
order; every loop stops at the first empty slot.

**`XCandle`**: placed on the back wall at x 19.8, y 175; model 0x1f8 turned 90°; a 25-wide,
12-tall box. Its flame point is 22 above; it makes a `CandleFire1` at the flame point + (6, 6, 0)
and starts its counter at `Random(15)`. Each tick it counts up and, every 15, makes a
`CandleFire2` within 2 of the flame point (`RandomPosXZ`). Drawn within the visible range. It
runs after the static objects and is drawn after them, before the bullets.

The heroes treat the candles like static objects: X and Z checks after the static objects, the
falling Y check after the map, the rising check after the static objects. A hero's shot stops at
a candle (after the bullets, before the boss).

| Effect | Look | Life |
|---|---|---|
| CandleFire1 | particle, the torch flame 0x187, alpha 0.7, size 80 × (0.9–1.1) re-rolled every 2 ticks | stays |
| CandleFire2 | particle 20, 0x188, rising 0.8 a tick, size × 0.965 | alpha −0.05 a tick, gone at ≤ 0.05 |

## Water and fish (`XWater`, `XEnemy_Fish`)

**Which levels have water** (`XMap::_ConstructWallObjects`, `XMap::UseWater`): any level whose
row flag (the first character of a line) is `W`. Row 0 (the start row) of every level in worlds
1, 3 and 4 carries it, and no World 2 level does, so all of worlds 1, 3 and 4 have water in
their pits. (The row flag `C` makes a candle.)

**`XWater`**: two `JObject3D_Rect` sheets over the whole map (5 × rows cells), scaled to it and
centred on it, with one texture repeat per cell. `water1` is drawn at y −38, blended
(TRANSPARENT, alpha 0.4), texture 0x18e (World 3: 0x28f); `water2` at −39, added (PARTICLE,
alpha 0.6), texture 0x1a6 (World 3: 0x290). Both use perspective and linear filtering. Each tick
the texture offsets drift: water1 by (0.0006, 0.0014), water2 by (0.0003, 0.0007), wrapping at
1. `XGameObject::InitLevel` calls `Deinit`, then `Init` on a water level. It runs right after
the map and is drawn after the enemies, before the particles: water2 first, then water1.
Nothing is drawn under the sheets: the pit shows the cleared back buffer, which
`XApplication::Render` clears to its background colour (`+0x1b0`), set to 0 (black) by the
constructor and never changed (`SetBackgroundColor` has no callers). That black is what makes
the pits look deep.

**On a water level**:

- the hero dies below −58 instead of −200, with a large wave (and its sound) at (x, −38, z);
- a hero's shot below −38 ends, with a small wave if it is above −43;
- a rolling cannonball below −38 bursts, with a small wave if above −43;
- a dropping platform stops speeding up below −38 and makes a small wave as it crosses −38.

**Fish** (`i`): type A (models 0x203, frames 0x200 0x203 0x201, jump frame 0x202) in worlds 1–2
(`currentWorld < 2`), type B (0x2af, 0x2b0 0x2af 0x2b1, jump 0x2b2) in worlds 3–4; collision
mesh 0x1ff. It is centred at (100, −62, cell z) with a 200 × 120 box there that never moves (its
position is written directly), so the mesh decides what it touches. It can't be hurt, chased
or pushed. `GetPosAtAngle(a)`: (100 + 80 sin(a − 90°), −62, z + 80 cos(a − 90°)), so angle 0 is
at the back (x 20). One state a tick (`else if`):

| State | What it does |
|---|---|
| SWIM1 | From angle 0, 1° a tick clockwise to 180° or anticlockwise to −180°; yaw eases (0.1) along the move, pitch to 0, frame (counter / 12) % 4. At the end the jump countdown drops by one: at 0 → JUMP2, else SWIM2. |
| SWIM2 | The same from 180° to 360° or 0°; at the end → JUMP1 or SWIM1. |
| JUMP1 / JUMP2 | From x 20 to 180 (JUMP1) or back, 0.008 of the way a tick, y + 92 sin(180° × travel); yaw and pitch (−`FindPitch`) ease along the move; frame 4 while travel is between 0.3 and 0.7. A small wave where it crosses −38. At the end: a new random direction, a new countdown, then SWIM2 after JUMP1, SWIM1 after JUMP2. |

The countdown (half turns before a jump) is 2–5 for type A and 1–2 for type B, picked anew
after each jump; the direction is random (`Random(2) == 0` is clockwise).

## Bosses and the dragon (`XBossBase`, `XBoss_Dragon`, `XBoss_DragonBody`, `JSpline`)

**The boss** (`XGameObject::boss`, +0xcc): `XMap::_ConstructObjects` makes one for the codes `6`
Dragon, `7` Genie, `8` Nest, `9` Wizard, at the cell centre and y = floor level × 25. It runs
right after the enemies and is drawn right after them (before the water). A hero's shot tests it
last, after the candles. The goal (`XMap::ProcessFrame`) opens, after the usual 300 ticks, when
every torch is lit on a level without a boss, or when `BossDefeated()` (the boss has been NON
for more than 500 ticks) on a boss level. The status display shows BOSS and `GetHealthMain()`
hearts instead of the torches.

**`XBossBase`** (vtable: Render, ProcessFrame, CollisionCheck_Fire, `_Damage`,
`_CheckBattleStarted`): main health 10 by default, with a sub health per heart.
`CollisionCheck_Fire`: scale > 0 and NORMAL, |Δz| and |Δx| ≤ the box's **Z** width + the shot's
radius, then the shot's sphere against the mesh; a hit calls `_Damage` and stops the shot.
`_Damage`: if it can be hurt, sound 0x270 at the boss's Z, a white flash for 12 ticks
(`_ProcessDamage`), one sub-health point off; at 0 one heart goes and the sub health refills,
and when the last heart goes `JMidiList::Stop` ("close all": the music stops), the explosion
centre is set to the boss and it turns DEAD. `_CollisionCheck_Hero`: |Δz| and |Δx| ≤ the two
boxes' half widths, then mesh against mesh; damage and knockback from the boss. The base
`ProcessFrame` (used by the other bosses) bursts it over 300 ticks: a large explosion at the
centre, then every 5 ticks a small one within 0.2–1 × the explosion radius in a random
direction (sound 0x26b every 15), and finally 0x27c unpanned (`JSoundList::PlayMono`).

**`XBoss_Dragon`** (`6`): head model 0x1d2 with the jaw frames 0x20a and 0x20d, mesh 0x20b,
an `XShadow` of 38, a 60 × 40 box; 10 hearts of 4 hits (in World 4 `healthSubMax` becomes 6
after the first heart is already set to 4). Points, from its cell (x, z):

- floor points (`_InitFloorRefPos`), all at y 20: `F0..F3` at x − 80 and z + 200, + 280, +
  360, + 440; `F4..F6` at x and z + 240, + 320, + 400; `F7..F10` at x + 80 and z + 200 … + 440;
- swim pool 1 (`_InitSwimRefPos1`), round (x, z): (x − 80, −30, z − 80), (x − 80, −70, z),
  (x − 80, −30, z + 80), (x, −70, z − 160), (x, −70, z + 160), (x + 80, −30, z − 80),
  (x + 80, −70, z), (x + 80, −30, z + 80); pool 2 the same round z + 640.

It starts at pool 1's first point in SWIM1. The battle starts when a NORMAL hero passes pool 1's
point 4 (z + 160). Each move builds a `JSpline` on its first tick and follows it at a constant
speed (1 / total length a tick), one state a tick:

| State | Path (nodes) | Next |
|---|---|---|
| SWIM1 | pool 1: 0 1 2 4 7 6 5 3 0, closed (`MergeEnds`) | SWIM1 until the battle has started, then FLOORWAVE1 or FLOORSTRAIGHT1 (`Random(2)`) |
| SWIM2 | pool 2: 2 1 0 3 5 6 7 4 2, closed | FLOORWAVE2 or FLOORSTRAIGHT2 |
| FLOORWAVE1 | here, pool 1 point 1, a leap (pool 1 point 2, z + 60, y 80), F0 F8 F2 F10, pool 2 points 5 6 7 4 2 | SWIM2 |
| FLOORWAVE2 | here, pool 2 point 1, a leap (pool 2 point 0, z − 60, y 80), F3 F9 F1 F7, pool 1 points 7 6 5 3 0 | SWIM1 |
| FLOORSTRAIGHT1 | here, pool 1 point 1, (x, −70, z), pool 1 point 4 at y 60, F4, a random centre, F6, pool 2 point 3 at y 20, (x, −70, z + 640), pool 2 points 7 4 2 | SWIM2 |
| FLOORSTRAIGHT2 | here, pool 2 point 1, (x, −70, z + 640), pool 2 point 3 at y 60, F6, a random centre, F4, pool 1 point 4 at y 20, (x, −70, z), pool 1 points 5 3 0 | SWIM1 |

The random centre (`_GetRandomCenterPos`) is F5 moved by `Random(5)`: 0 none, 1 x − 40, 2
x + 40, 3 and 4 both x + 80. On the straight crossings the flame starts as the travel passes 0.2
and ends as it passes 0.5. Every tick: yaw eases (0.1) along the move and pitch (0.05) to
−`FindPitch`; a large wave with its sound where it crosses −38 (`_CheckSplash`); the hero test;
the position, yaw and pitch go into a 500-entry ring (`posRec`, `yawRec`, `pitchRec`). The flame
(`flameCounter` counts every tick): STARTFLAME jaw frame 1 for 15 ticks, frame 2 until 50, then
sound 0x27a and DOFLAME; DOFLAME frame 2 and a flame every 5 ticks (when the counter % 5 == 1)
from 20 ahead along the yaw and pitch; ENDFLAME frame 2 until 35, frame 1 until 50, then
NONFLAME (frame 0). Dying: a large explosion and 0x26b, and 16 ticks later the body starts
dying.

**`XBoss_DragonBody`**: 16 segments chained from the head (`firstBody`, each making the next):
1–15 model 0x1d3 (mesh 0x20c), 16 the tail 0x1d1 (mesh 0x209); an `XShadow` of 38, a 45 × 40
box; they can't be hurt but stop shots (each passes the test on to the next). Segment n takes
the head's record from 28 × n ticks before (`(recIndex − 28n + 500) % 500`), with a silent wave
where it crosses the water. Dying: each bursts (small explosion, 0x26b) and 16 ticks later the
next starts; after the tail, 0x27c unpanned.

**`JSpline`** (a rounded non-uniform spline): nodes of position, unit velocity and the distance to
the next node. `AddNode`: the first node gets velocity 0 and distance 1; each new node's velocity
is the direction of the new segment, and the segment's length is added to the total; the node
before it gets the normalised average of the directions on either side (the incoming one divided
by its length); the new end gets normalize(3 × its direction − the previous node's velocity),
and when the third node arrives the start is redone the same way. `MergeEnds` (three or more
nodes) moves the last node onto the first and gives both the normalised average of the first
and last segment directions. `GetPosition(t)` walks the segments to the distance t × total, and
evaluates that segment's cubic Hermite curve with the velocities scaled by the segment length
(`Spline_GetPosition`: (t³, t², t, 1) × POSITION_MIXER × the rows (start velocity, start, end,
end velocity), with POSITION_MIXER rows (1, 2, −2, 1), (−2, −3, 3, −1), (1, 0, 0, 0),
(0, 1, 0, 0), filled by a static initializer).

**`XBullet_Flame`**: a `JObject3D_Particle` of 50 with the explosion sheet 0x192 (4 × 4, 4 ticks
a frame), radius 10, flying at 4.5 along its yaw and pitch and slowing by 0.03 a tick (not below
0); the map, an object or a platform stops it where it was before the move. It hurts a hero only
while its frame is below 9, and ends after 64 ticks.

## The genie (`XBoss_Genie`, `XStaticObj_GenieLamp`, `XParticle_LampSteam`)

**Lamp** (`XStaticObjList::CreateGenieLamp`, made by the genie's constructor at its start; the
list does not call `CreateExtraCollideBox` for it): model 0x1ea turned 180°, an `XShadowSimple`
of 36 placed with `SetPos` (y + 0.2), a 30 × 24 box, and `holePos` (the spout) at (x, y + 28,
z − 30). `MOVESTATE` NORMAL / SHAKE. `StartShaking` zeroes the counter and sets SHAKE; then each
tick the pitch is sin(t/500 · 100 · π) · 4° (0.0698132), a `CreateLampSteam` at the spout every
16 ticks (counter mod 16 = 0), and at 500 it goes back to NORMAL (the last pitch is left).

**Lamp steam**: a billboard of the smoke texture (0x198) scaled 6, rising 1.3 a tick; alpha −
0.007 and scaleIncr (from 0.1) × 0.962 a tick, the rect scaled by 1 + scaleIncr each tick while
alpha > 0.05.

**Genie** (`7`): frames 0x1ee (closed), 0x1ec, 0x1ed (open), collision 0x1eb, `XShadowSimple`
140, box 200 × 200, scale 0, explodeRadius 80, healthSubMax = healthSub = 15, then 20 in World 4
(set after healthSub, so the first heart there takes 15). NORMAL, `MOVESTATE` HIDE. Not drawn in
HIDE. It runs no `IsUpdatable` test. `_Damage` is the base's, then in IDLE idleFrames − 50.
`_UpdatePosY`: offsetCounter + 1, pos.y = refPosY + sin((offsetCounter mod 200)/200 · 2π + π)
· 8. `_GetMouthPos`: 34 along the yaw and 62 up.

- HIDE → STEAM when `FindDistanceToCloserHeroXZ` (NORMAL heroes, `ApproxDistanceXZ`) < 80.
- STEAM: tick 1 sound 0x2a3 and the lamp's `StartShaking`; → APPEAR once the lamp is NORMAL.
- APPEAR (500): tick 1 sound 0x2a1, yaw 0.654503, scale 0, `_SetPos(holePos)`; with t =
  counter/500, scale = sin(t · π/2), pos.y = start.y + (h + (80 − h) · t) + sin(t · π) · 120
  (h = hole.y − start.y), yaw += (1 − t) · 15°. At 500 refPosY = pos.y → IDLE.
- IDLE: tick 1 idleFrames = `RandomI(4, 8)` · 100; bob; yaw eases to the closer hero (0.05,
  at most 0.8°); when (signed) counter ≥ idleFrames → OPENMOUTH.
- OPENMOUTH (60): frame 1 at 10, 2 at 40; bob; yaw (0.05, at most 1.6°) → ATTACK.
- ATTACK: tick 1 fireCount 0, attackCounter 0, ROTATE. Each tick attackCounter + 1 and bob.
  ROTATE: yaw (0.05, no limit); at 30 → FIRE. FIRE: at 5 sound 0x2a2,
  `XBulletList::CreateNeedleBall(_GetMouthPos(), yaw)` and fireCount + 1; at 20 → ROTATE. Then
  fireCount ≥ 30, or a hero within 80 (XZ) → CLOSEMOUTH.
- CLOSEMOUTH (100): frame 1 at 10, 0 at 40; bob; yaw (0.05, at most 1.6°) → MOVE.
- MOVE: tick 1 sound 0x2a4, moveDest = (moveDest + `Random(2)` + 1) mod 3 (CENTER, LEFT
  z − 380, RIGHT z + 380 from the start), destYaw towards it; bob; pos.z eases
  (`MakeValueCloseTo` 0.05, 0.1, 2.5) and the yaw (0.1); when pos.z equals the destination →
  IDLE. Only pos changes: the box stays where `_SetPos` put it at the spout.
- Defeat is the base boss's: 300 ticks of explosions within 80 of where it was, then gone.

**Needle-ball bullet** (`XBullet_NeedleBall`, from `XBulletList::CreateNeedleBall(pos, yaw)`):
model 0x1f0, `XShadow` 19, radius 9 (and a collide sphere of 9), speedFactor 1, targetHero =
`GetCloserHero` at the moment it is made. Each tick: verticalSpeed − 0.04 (at least −4); the
bottom (y + verticalSpeed − radius) against `XMap::CollisionCheckY`, and on the floor
verticalSpeed 0 and speedFactor + 0.01 (at most 5); y = bottom + radius. Before tick 500 the yaw
turns at most 0.3° (0.00523599) towards the target's `GetCenterPos`. Pitch + 0.0331613 ·
speedFactor; x/z + 0.5 · speedFactor along the yaw; smoke every 15 ticks. After 20 ticks the
map (`IsBlockedFloat`), static objects, platforms or either hero burst it at the previous
position; below −200 it is gone; on a water level below −38 a wave (if above −43) and a burst,
as the rolling cannonball.

## The nest and its birds (`XBoss_Nest`, `XBoss_Bird`)

World 3's boss (3-10 at (100, 1580), over the water; also 4-8). The nest is the boss: it has the
hearts and takes the shots. Its 40 birds are `XBossBase` objects the nest owns (`birdArray[40]`),
updated and drawn by the nest; they aren't in any list, so shots never meet them.

**Nest** (`8`): model 0x28d, collision 0x28e, box 140 × 60, no shadow. pos.y + 12, `refPosY` =
that. `rotSpeed` 0.1° (0.00174533) a tick. Makes all 40 birds at once. `explodeRadius` 85,
`healthSubMax` 15 (= healthSub), then 20 in World 4 (`GetLevelStatus` world 3), set after
healthSub, so the first heart takes 15, as the genie. NORMAL, WAIT.
- `ProcessFrame`: the base's (flash, dying); if NORMAL: SHAKE if shaking, yaw −= rotSpeed, world
  matrix, the hero checks. Then every bird's `ProcessFrame`, whatever the nest's state.
- `Render`: every bird, then the base's (the nest itself, not while DEAD or NON).
- `_Damage`: the base's hit (sound 0x270, flash, health). If that killed it, every bird's
  `StartDead` and nothing else. Otherwise sound 0x2a6, `birdArray[currentBirdIndex]->StartAttack()`,
  currentBirdIndex = (index + 1) mod 40, rotSpeed += 0.02° (0.000349066), t = 0, SHAKE.
- SHAKE: at t = 1 three large water rings (`CreateWaterWave(p, true, false)`) at y −38, 40 from
  the nest at random angles. pos.y = refPosY + sin(2π · (t/100) · 10) · (1 − t/100) · 3.5 for 100
  ticks, then WAIT.

**Bird**: made with the nest's pos (after the +12). Models 0x289 / 0x28a / 0x28b / 0x28a (a wing
beat), collision 0x28c, an `XShadow` 12 (`bodyShadow`), box 20 × 20, not damagable, NORMAL, WAIT.
- `ProcessFrame` (not the base's): t++; NON: nothing. NORMAL: birdCounter++, the state, frame =
  (birdCounter / 3) mod 4, world matrix, the hero checks (also while it sits in the nest). DEAD:
  a small `CreateEnemyExplode` at pos, then NON.
- `Render`: whenever not NON, with no visibility test: model, shadow at pos (the box's debug
  draw too). So the waiting birds are drawn at the nest's centre.
- `StartAttack`: only from WAIT: t = 0, GO. `StartDead`: from WAIT, NON at once; otherwise DEAD.
- WAIT: nothing.
- GO: at t = 1 a 4-node `JSpline`: the nest, nest + 60 up, nest + 120 up, and a point 60 from the
  nest at a random angle, 100 up; travel 0, travelSpeed = 1.8 / total length. Each tick travel +=
  speed (capped at 1), pos = the spline point; yaw eases to the direction moved (0.16), pitch to
  −`FindPitch`(previous, new) (0.08). At travel 1: `Deinit`, ATTACK_HERO.
- ATTACK_HERO: at t = 1 chirpTimer = RandomI(100, 300) and the hero target. Then the flight (below);
  chirpTimer counts down to 0: sound 0x2a5 and a new RandomI(100, 300). A new hero target when
  t mod 130 = 0. At t ≥ 1800: FLY_ABOVE_NEST.
- FLY_ABOVE_NEST: at t = 1 the nest target; the flight; a new nest target when t mod 130 = 0; at
  t ≥ 400: BACK_TO_NEST.
- BACK_TO_NEST: at t = 1 a 5-node spline: pos, 30 ahead along yaw and −pitch, nest + 100 up, nest
  + 60 up, the nest; travelSpeed = 1.2 / length. As GO, but each spline point goes through
  `_LimitPosition_Wall(5)` and `_LimitPosition_Floor(5)` before `_SetPos`. At travel 1: WAIT.
- The flight (written out in both ATTACK_HERO and FLY_ABOVE_NEST): yaw steps 2° (0.0349066) a
  tick towards `FindDirectionXZ(pos, target)`; pitch steps 1° (`MakeValueCloseTo_Constant`, not
  an angle) towards −`FindPitch(pos, target)` clamped to ±45°; speed = 0.8 + 1.0 · min(XZ distance
  / 160, 1); the new pos = pos + speed · (sin yaw · cos(−pitch), sin(−pitch), cos yaw · cos(−pitch)),
  then `_LimitPosition_Wall(5)`, `_LimitPosition_Floor(5)`, `_SetPos`.
- `_UpdateTargetPos_Hero`: the closer hero's (to the nest) `GetCenterPos` (pos + 14.5 up) plus 50
  along a random direction (a = rand · 2π, b = rand · π − π/2: 50 · (sin a cos b, sin b, cos a
  cos b)); then wall limit 30 and floor limit 5.
- `_UpdateTargetPos_Nest`: the nest's pos plus 60 along a random direction in the upper half (b =
  RandomF(0.2, 1) · π/2) plus 80 up; wall limit 30 only.
- `_LimitPosition_Wall(p, m)`: x kept within m .. 200 − m, and z ≥ m (no limit at the far end).
  `_LimitPosition_Floor(p, m)`: y ≥ m. Both ignore the level's real floors and walls.

## The wizard, its blade and its wand (`XBoss_Wizard`, `XBoss_WizardBlade`, `XBoss_WizardWand`)

World 4's boss (`9`, 4-10 at (140, 940); the arena is the floor around z 860..1020, with pits on
every side). The wizard owns a blade and a wand, both `XBossBase` objects it runs and draws itself
during the matching state; the wand fires `XBullet_WizardFire` and glows with
`XParticle_WizardFireFlash`.

**Wizard**: models 0x298 / 0x299 / 0x298 / 0x29a, collision 0x296, box 200 × 80, an `XShadow` 39.9
(`bodyShadow`). yaw π; pos = `GetRefPos(RIGHT)` with y −300 (under the floor). `explodeRadius` 80,
healthSubMax 40 (= healthSub), healthMain the base's 10. NORMAL, HIDE, posState RIGHT.
- `GetRefPos(s)`: x 100, y 87.5 (absolute), z = start.z, −200 (LEFT), +200 (RIGHT), −100
  (LEFT_CENTER), +100 (RIGHT_CENTER); UP_CENTER / DOWN_CENTER: x 0 / 200 at start.z (unused here).
- `ProcessFrame`: the base's; if NORMAL: frame = (t / 3) mod 4, the state, then (not in HIDE or
  APPEAR) `_UpdatePosY`, world matrix, the hero checks.
- `Render`: not NON or DEAD: model, shadow at pos (box debug draw), then the blade in BLADE or the
  wand in WAND.
- `_UpdatePosY`: offsetCounter++; pos.y = 87.5 + sin((offsetCounter mod 100)/100 · 2π + π) · 4.5
  (pos only; the box keeps the y the last `_SetPos` gave it).
- HIDE: the first time the closer hero is within 300 (XZ) of pos, `PlayMono(0x2b3)` (a roar); within
  110 (XZ) → APPEAR. pos is the RIGHT spot, 200 past the arena's middle, so the hero has to reach the
  arena's far edge.
- APPEAR: at t = 1 `PlayMono(0x2ab)`. k = min(t/100, 1), y = −300 + sin(k · π/2) · 387.5, `_SetPos`;
  from t = 100 also `_UpdatePosY`. At t ≥ 300: posState RIGHT, MOVEIN.
- MOVEIN / MOVEOUT: at t = 1 fromPos and toPos = the ref spots (LEFT → LEFT_CENTER, RIGHT →
  RIGHT_CENTER; out the other way). pos = `D3DVectorInterpolate(from, to, t/150)`, `_SetPos`; yaw
  eases (0.05) to the direction moved. At t ≥ 150 posState becomes the new spot; MOVEIN → ROTATE,
  MOVEOUT → `Random(2)` 0: WAND, else BLADE.
- ROTATE: at t = 1 angle π (from LEFT_CENTER) or 0 (RIGHT_CENTER), toPos = the CENTER spot, radius
  100, speed 1.2 (degrees a tick), subLocalCounter 0, GET_FASTER. Each tick subLocalCounter++; unless
  STOP, angle = `NormaliseAngle(angle + speed°)`, from t > 10 `_PlayRotateSound(old, new)`, pos =
  toPos + radius · (sin angle, 0, cos angle), yaw = `FindDirectionXZ(pos, toPos)` (facing the middle).
  Then: GET_FASTER speed +0.016 to 6; GET_CLOSER radius −0.2 to 40; STAY 300 ticks; GET_FURTHER
  radius +0.2 to 100; GET_SLOWER speed −0.016 to 1.2; STOP: angle += speed° without normalising,
  capped at 2π (from LEFT_CENTER) or 3π (from RIGHT_CENTER), placed and faced as above; on reaching
  it posState flips to the other centre spot and MOVEOUT. So it always stops opposite where it came
  in.
- `_PlayRotateSound(a, b)`: sound 0x2a7 when a > b (wrapped past 0) or a ≤ q ≤ b for q = π/2, π,
  3π/2.
- BLADE: at t = 1 sound 0x2aa, `blade->Start(windDir)` with windDir 0 from LEFT or 4 from RIGHT,
  toPos = CENTER. yaw eases (0.05) towards the middle; the blade's `ProcessFrame`; when its liveState
  is NON → MOVEIN.
- WAND: the same with the wand (`wand->Start(wandDir)`, 0 from LEFT, 4 from RIGHT), until the wand
  is NON.

**Blade**: base made with the wizard's pos; model 0x29c only: no collision model and no box (it
never hurts and never meets shots; its pos is written directly). windDir −1.
- `Start(windDir)`: scale 0, rotateSpeed 0, `_UpdatePosAndYaw`, world matrix, t = 0, NORMAL,
  APPEAR.
- `ProcessFrame`: the base's; if NORMAL: the state, `_UpdatePosAndYaw`, `_PushHero` for each hero,
  world matrix. Drawn by the base `Render` (model only).
- `_UpdatePosAndYaw`: yaw = the wizard's; pos = the wizard's + 38 · (sin yaw, 0, cos yaw).
- APPEAR: scale = sin(t/120 · π/2); at 120 → ROTATE_FASTER. ROTATE_FASTER: at t = 1 speed 0;
  speed +0.02 to 10; roll = `NormaliseAngle(roll − speed°)`; at 10 → ROTATE_STABLE. ROTATE_STABLE:
  roll −10° a tick; at t ≥ 800 → ROTATE_SLOWER. ROTATE_SLOWER: at t = 1 speed 10; speed −0.02 to 0;
  roll −speed°; at 0 → DISAPPEAR. DISAPPEAR: scale = cos(t/40 · π/2); at 40: NON (moveState and
  liveState).
- `_PushHero` (blade NORMAL, hero NORMAL, not NON, APPEAR or DISAPPEAR): push = speed/10 · 1.375;
  windDir 0: `IncreasePushZ(push)`, 4: `IncreasePushZ(−push)`. So it blows the hero away from the
  wizard's side, towards the pit beyond the arena.

**Wand**: base made with the wizard's pos; model 0x29b only (no collision model, no box; pos
written directly). wandDir −1, fireTimer 0.
- `Start(wandDir)`: wandDir stored (never read), scale 0, pitch 0, `_UpdatePosAndYaw`, world matrix,
  fireTimer 120, t = 0, NORMAL, APPEAR.
- `ProcessFrame`: the base's; if NORMAL: the state, `_UpdatePosAndYaw`, world matrix. Drawn by the
  base `Render`.
- `_UpdatePosAndYaw`: yaw = the wizard's; pos = the wizard's + 1 · (sin yaw, 0, cos yaw).
- `GetFirePos`: pos + 50 · (sin yaw, 0, cos yaw), 20 lower.
- APPEAR: scale = sin(t/100 · π/2); at 100 → STICK_OUT. STICK_OUT: pitch = sin(t/50 · π/2) · 60°;
  at 50 pitch 60° → ATTACK. STICK_IN: pitch = (1 − sin(t/50 · π/2)) · 60°; at 50 pitch 0 →
  DISAPPEAR. DISAPPEAR: scale = cos(t/40 · π/2); at 40 NON.
- ATTACK: at t = 1 sound 0x2a9; when t mod 4 = 1 `CreateWizardFireFlash(GetFirePos())`; when
  --fireTimer ≤ 0: sound 0x2a8, `CreateWizardFire(GetFirePos(), yaw)`, fireTimer = 160 (t < 666),
  130 (t < 1332) or 105. At t ≥ 2000 → STICK_IN. So fireballs at 120, 280, ... about 16 in all.

**Fireball** (`XBullet_WizardFire`, `XBulletList::CreateWizardFire(pos, yaw)`): model 0x29d scaled 0,
yaw as given, radius 9 (its `XCollideSphere`), an `XShadow` 18 (radius × 2), speed 0.8, NORMAL.
`ProcessFrame` (after the base's count, if NORMAL): keep pos; target = the closer hero's
`GetCenterPos`; after t > 40 speed +0.008 to 3 and `CreateSmoke(pos)` when t mod 12 = 0; yaw steps
1.2° (0.020944) towards the target (`MakeAngleCloseTo_Constant`), pitch 0.8° (0.0139626) towards
−`FindPitch` (`MakeValueCloseTo_Constant`); pos += speed · (sin yaw cos(−pitch), sin(−pitch),
cos yaw cos(−pitch)); scale +0.015 to 1 (`SetScale`); world matrix. Then: the map (`IsBlockedFloat`),
an object (`XStaticObjList::CollisionCheck`) or a hero hit (`_CollisionCheck_Hero`, either hero) →
`CreateBulletExplode` at the kept pos, NON; else below −200 → NON; else on a water level below
−38 → a small ring with sound at y −38 if still above −43, the burst, NON. Platforms don't stop it.

**Fire flash** (`XParticle_WizardFireFlash`, `XParticleList::CreateWizardFireFlash(pos)`): a 1 × 1
`JObject3D_Particle` (camera-facing) `Scale`d 150, texture 0x194 (player 1's shot flash sheet), a
random cell of its 3 × 3 grid (`_SetTextureRandomly`: Random(9), cells a third wide), alpha 1.
Each tick alpha −0.12; while above 0.05 it `Scale`s by 0.95, sets the alpha factor and a new random
cell; then it ends. (The port's `XCollideSphere` is the bullets' `radius_`.)

## Particles and shadows (`XParticleList`, `XShadow`, `JObject3D`)

**`JObject3D_Rect`** is a w × h quad of lit vertices in the XZ plane (v0 −x+z, v1 +x+z, v2 −x−z,
v3 +x−z; triangles 0-1-3 and 0-3-2), all white and opaque. `Scale` and `Rotate` change the
vertices themselves (accumulating); the JObject3DBase transform (scale · roll · pitch · yaw ·
translation) applies when drawn. `SetUV(u0, u1, v0, v1, inset)`; `SetAlphaFactor(f)` sets the
vertex alpha to (int)(f × 255). `Render2` draws it facing the camera: the inverse view matrix
with its translation replaced by the position. The render states, saved and restored around
each draw (lighting off):

| Alpha state | Blend | Used by |
|---|---|---|
| PARTICLE | source alpha / destination alpha, no Z writes, texture × diffuse | particles (`JObject3D_Particle`: turned −90° about X, drawn with `Render2`, no perspective correction), flat effects |
| SHADOW | zero / source colour (multiplies the scene), no Z writes | `JObject3D_Shadow` |
| TRANSPARENT | source alpha / inverse source alpha | — |

The back buffer has no alpha channel, so destination alpha reads as 1 and the particles add up.
Filtering is linear (mip linear) for all of them. All textures are created at start-up with no
transparency flags.

**`XParticleList`** has 300 slots. `GetNullIndex` searches from a rotating index, advancing it
past every slot it looks at, and gives up after one full turn (the effect is then not made).
Each tick: `ProcessFrame` on every particle (the base class counts `localCounter` up first),
deleting the ones that reached state NON. It runs right after `XMap::ProcessFrame` (and
`XWater`), before the shots, bullets and heroes; it is drawn after every object and before the
shots.

| Effect | Made by | Look | Life |
|---|---|---|---|
| HeroFireSmoke | each shot every 2 ticks; each warp photon every 5 | particle 17 (× 0.8 for small shots), 0x15d / 0x185 (player 2) | alpha −0.05 a tick, size × 0.97; gone at ≤ 0.05 |
| HeroFireFlash | a shot stopped by a wall or object, at its position before the move | 1.5 × 1 rect turned −90° about Z (upright, facing the camera side), × 70 (× 0.9 small), 0x194 / 0x195, a random cell of a 3 × 3 sheet every tick | alpha −0.06, size × 0.94 |
| HeroWarpPhoton | the warping hero, when its counter % 25 == 1 | particle 18, the shot texture, circling the hero at height 0–40 | 600 ticks: distance (22 + 48 (1 − sin(t/300 · 90°))) × (0.9–1.2), speed (2 + 6 sin) × (0.6–1) degrees a tick, 22 and 8 after 300 ticks; alpha fades in over 100 ticks and out over the last 100, halved |
| TorchFire1 | a torch lighting up, at its flame point | particle 110 × (0.9–1.1), re-rolled every 2 ticks, 0x187 | stays |
| TorchFire2 | a lit torch every 12 ticks, within 2.5 of the flame check point | particle 34, 0x188, rising 1.6 a tick | alpha −0.04, size × 0.96 |
| WarpRing | the open goal every 60 ticks, 0.5 above it | flat rect, scale 15 growing by 3 (× 0.985 a tick), 0x1a5 | after 50 ticks alpha × 0.975; gone below 0.03 |
| LandEffect | a hero landing at full fall speed, 0.5 above its feet | flat rect, scale 30 growing by 3.5 (× 0.92), 0x295 | after 10 ticks alpha × 0.92 |
| GetCoinEffect | a collected coin, at its centre | three particles of 18, 0x294, flying out along (sin a, 3, cos a) normalised at 1.6 a tick, a = −90°, 30°, 150° | after 20 ticks alpha × 0.9 |
| EnemyExplode | the dying hero (large), enemies, barrels, bosses | particle 180 (large) or 100, 0x190, 4 × 4 frames, 6 ticks each | 96 ticks |
| Smoke | bullets and rockets | particle 10, 0x198, growing by × (1 + 0.12 × 0.9ⁿ) | alpha −0.015 |
| BulletExplode | a bursting bullet, where it was before its move; plays 0x279 when made | particle 60, 0x192, 4 × 4 frames, 4 ticks each | 64 ticks |
| WaterWave | things meeting the water, at −38 | flat rect, scale 24 (36 large) growing by 3 (3.5 large) × 0.95 a tick, 0x18f; sound 0x278 when asked | alpha × 0.97 every tick; gone at ≤ 0.03 |

Still to come with their owners: CandleFire1/2, LampSteam, HammerEffect, WizardFireFlash.

**`XShadow(size)`** (the hero's is 35) draws texture 0xa3 under a position as up to four
`JObject3D_Shadow` rects. Its square is split at the cell edge in X when the floors on either
side differ, and likewise in Z; each part lies on its own cell's floor (level × 25 + 0.2) with
the matching share of the texture, and is drawn only if that height is above 0 (a floor
exists) and below the owner's y + 0.4. The hero draws it at the end of `XHeroBase::Render`.

## The status display (`XStatusDisplay`)

DirectDraw blits on the back buffer after the 3D scene, at fixed 640×480 positions.
`JSprite(resId, frames)` stacks its frames vertically (frame height = bitmap height / frames)
and is colour-keyed on its top-left pixel. `JNumber` uses the ten-frame digit sprite 0xba; it
pads with zeros to its digit count, drops digits beyond it on the left, and advances a third of
the digit sprite's width (17 px) per digit.

| What | Bitmap | Position |
|---|---|---|
| LEVEL, world (1 digit), "-", level (2 digits) | 185, 0xba, 419 | x 8, y 4; each item at the previous one's x + width (+4 after LEVEL; a third of the width after "-") |
| TORCH, lit (2), "/", total (2) | 417, 0xba, 418 | the same along y = LEVEL height + 10 = 39 |
| BOSS and the boss's hearts (boss levels, instead of TORCH) | 527, 526 (4 frames) | x 8, y 39; hearts from x + width + 4, y + 1, each (width / 2 − 8) on |
| Player 1 face | 509 | 450, 4 |
| LIFE and hearts | 411, 416 (4 frames) | 492, 4; hearts from x + width + 4, y + 1, each (width / 2 − 6) on, frame (counter >> 4) % 4; not while DEAD |
| GOLD and gold (4 digits) | 409 | 492, LIFE y + height + 2 (31); number at x + width + 4 |
| MP and 10 lights | 413, 415 (2 frames) | 492, GOLD y + height + 2 (58); lights from x + width + 4, y + 2, each width / 4 on; lit while i < magic points, all dark while DEAD |
| Player 2 (two-player games): face, LIFE, GOLD, MP | 510, 412, 410, 414 | the same at x 260 / 302 |
| Lost-gold messages (500 ticks) | 0x230 (player 1), 0x231 (player 2), amounts in 3 digits | centred on (320, 240) and (320, 267); amount at x + 198, y + 10 |

The counter and the two message timers advance in `ProcessFrame`; `InitForGame` clears them.
The world and level numbers are shown 1-based.
