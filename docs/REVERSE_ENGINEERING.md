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
3. Underground scenery for every column (`_CreateUnderground`).
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

### Level start, camera and music

- Hero start (`XGameObject::XGameObject`): one player at (100, 0, 60), the middle of row 1;
  two players at (60, 0, 60) and (140, 0, 60). `XHeroBase::Reset` then lifts each hero to
  y = 800, so they drop into the level.
- Camera (`XCamera`): looks at (140, 120, z) from 340 units away, 35° (0.610865 rad) above the
  horizontal, horizontal angle π/2, so the eye is on the +x side. The look-at z starts at 160 and
  stays within [160, (rows − 4) × 40]; it eases toward its target (`MakeValueCloseTo`, factor 0.1,
  step 5). In play the target is the hero's z, or with two heroes alive their midpoint, with at
  most 240 units of separation taken into account.
- Music (`XGameObject::InitLevel`): Music06 when the level has a boss, otherwise Music02–05
  for worlds 1–4 (Music01 is the menus'). The test is `XGameObject::boss` (+0xcc, an
  `XBossBase*`), which `_ConstructObjects` sets for the boss codes `6` Dragon, `7` Genie, `8` Nest
  and `9` Wizard. Bosses appear in level 10 of every world and also in World 4's levels 2
  (Dragon), 5 (Genie) and 8 (Nest), which therefore play the boss music too.

A floor tile is a flat 40×40 `JObject3D_Rect` at height × 25, mapped to the top-left quarter of
the floor texture with a half-texel inset. The J library is the same one Hopmon uses, one
version newer: texture and UVs are set by separate calls (`SetTexture`, `SetUV`).

## Behaviour to preserve

- Fixed-step game logic (tick rate to be confirmed from `XApplication::FrameMove`), with any
  random-number generator reproduced exactly.
- Two-player co-op: `XHeroA` and `XHeroB` read separate inputs (`_UpdateInput`).
- Save slots (`XSelectSlot`, `XSaveData`) and settings via the registry (`JWinReg`). The port
  will keep its own saves and import the original's, as Hopmon Native does.

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

### Controls (player 1, `XHeroA::_UpdateInput`)

Arrow keys move. Fire: Right Ctrl, `[` or joystick button 1, plus Left Ctrl in one-player games.
Jump: Right Shift, `]` or joystick button 2, plus Left Shift in one-player games. The stick is
only read while no arrow key is held, and not at all when the joystick is disabled in settings.

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
Escape makes a hero in NORMAL give up (`StartDying`).

A level runs as a process with a type:

- A hero COMPLETED (the warp finished): if this isn't the world's last level, type 1 (after 200
  ticks fade out, then the next level of the world); after the last level of a world, type 2
  (advance the world, back to level select); after the last world, type 3 (advance, save, the
  ending).
- A hero DEAD: type 4 (after 300 ticks fade out and return to level select). There are no lives;
  the player picks a level again.

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

A fire press (with the wand down and magic to spare, see the hero) makes shots by wand type:
0 one shot; 1 two; 2 three, spread ±6°; 3–5 the homing ("chase") versions. A shot starts 20
ahead of the hero along its yaw at hero height + 12, radius 8 (80 % for the small variant), drawn
as an 18 × 18 glowing sprite (texture 348 for player 1, 388 for player 2). Each tick it moves 2.6
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
pillar `Q` 20 × 78 (0x1db), barrel `O` 27 × 42.8 (0x1f6, a 37 shadow). They stop heroes (heights
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

**Barrel** (`j`): model 0x1f6, collision 0x204, shadow 37, box 27 × 42.8, plus a static box of
the same size that stops heroes and enemies but not shots. 10 health, KING size, never hurts.
At 0 it bursts at once: explosion, the box disabled, XGameObject's barrel count + 1 (kept for
the whole game), a heart every third barrel, else a medium coin; sound 0x267.

**Fence door** (`v` across X, `w` along Z and turned −90°): model 0x1dc, collision 0x1dd, box
40 × 82 × 10, with a matching static box that doesn't stop shots. 5 health; bursts like a barrel
without a coin.

**Needle fan** (`m`): a spiked bar, model 0x17f, collision 0x17d, box 200 × 80, shadow 42,
turning (0.6 + 0.1 × world)° a tick on a post (model 0x17e, a static 19 × 87 box). Can't be hurt;
drawn without the visibility test.

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

Still to come with their owners: BulletExplode, CandleFire1/2, LampSteam, HammerEffect,
WaterWave, WizardFireFlash.

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
