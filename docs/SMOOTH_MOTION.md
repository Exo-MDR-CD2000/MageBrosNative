# Smooth motion (interpolation)

Drawing the game smoothly at any refresh rate without changing the game. Researched on
2026-10-26 after 0.4.15, built in four chunks on the `interpolation-test` branch: A (0.4.16), B1
(0.4.17), B2 (0.4.18), C (0.4.19) and D (0.4.20), with the owner's play tests along the way, then
**merged into `main` on 2026-10-28 and released as v0.4.20**. **Smooth motion is off by default.**

This file starts with how it works as built. The research and each chunk's plan and "as built"
notes follow, in the order they were written, as the history and the record of what was checked.

## How it works (as built, 0.4.20)

**The idea.** The game runs in ticks, 100 a second, exactly as before. With Smooth motion on, the
framework draws a picture for every refresh of the monitor and tells the drawing code how far the
clock is between the last two ticks (alpha, 0 to 1). Everything that moves is drawn that far
between where it was after the previous tick and where it is after the latest. Nothing the game
reads ever changes: only what is drawn. With Smooth motion off, or at alpha 1, every picture is
the original's, byte for byte (checked at each chunk).

**The clock** (`src/framework/d3dapp.cpp`). Ticks are counted from the frame's millisecond time,
as before, and kept to the nanosecond (`lastTickNs_`, `SetLastTick`). Alpha comes from the
nanosecond clock (`AppTimeNs`). The options menu and P freeze it (`HoldTickPhase`), and the first
frame after them carries on from the held picture (`heldLastFrame_`), so nothing jumps.
`AppFrameRate` gives the measured frame rate (the ending's corridor uses it).

**`Interp`** (`src/framework/interp.*`): `BeginTick` / `CurrentTick` (called by `XGameScene` before
each tick, the shop's ticks included), `BeginFrame` / `FramesDrawn` (each level picture),
`Alpha()` (1 unless Smooth motion is on and a scope is open: `SetScope`, opened only around a
level's 3D, the shop and the ending's credits and corridor), `Lerp`, `LerpAngle` (the short way
round), `BlendPos` (for sprites, with the 40-unit safety net) and the `--auto alpha=` hold.

**Models** (`JObject3DX`, `src/game/JModel.*`): every model keeps its pose from before the latest
tick, taken at its first pose change in a tick (`TouchPose`, `poseTick_`), and is drawn between
the two; its world matrix, and so its collision, is always the tick's own. A model not drawn in
the last frame (new, or shown again) isn't blended in its next tick (`snapTick_`,
`lastDrawFrame_`); a jump of more than 40 in a tick isn't blended; `Snap()` after a reset. A model
drawn twice in one frame is logged once. Shadows take their owner's position plus the model's
`BlendOffset()` (exactly zero when not blending).

**The camera** (`XCamera::BlendedView`): the view between the last two ticks for the level's 3D
and its billboards; the draw range, the sound and the shop's panel use the tick's camera.

**Sprites**: particles, the mages' shots and the dragon's flame keep their owner's position from
before the latest tick (`KeepPrevPos`, `prevPos`) and are drawn at `Interp::BlendPos`. The water
blends its texture drift.

**Other screens**: the shop's slide (`XShop::SlideInBlended`) and the ending's credits
(`XEndingObject::CreditPosXBlended`, placed between whole pixels while blending) blend from their
counters. The ending's corridor, with Smooth motion on, moves once a tick at the speed the frame
rate gave it (`CorridorPerTick`), drawn between ticks.

**Kept as the original on purpose**: animation frames (models, sprite sheets, blinking), particle
sizes and fades, and the 2D that moves a whole 640 x 480 pixel at a time (the menus' background,
the ending's film strips, the venetian-blind fades). The HUD doesn't move.

**The cost**: the picture trails the game by up to 10 ms (5 on average). Input is read on ticks as
before. The frame rate costs nothing measurable (about 1,070 frames a second either way in the
busiest level, VSync off).

**Testing**: `--auto alpha=<0..1>` holds alpha (`alpha=off` frees it); `alpha=1` must give the
same screenshots as Smooth motion off. The checks used at each chunk (byte-for-byte screenshots,
temporary per-object logs at `alpha=0.5`) are described in each chunk's "as built" section.

## Where things stood before (0.4.15, the research of 2026-10-26)

- **The game logic runs at 100 ticks a second.** Every 10 ms the game advances one step
  (`XApplication::FrameMove(n)`, at most 10 ticks a frame): the mages, enemies, platforms, shots
  and the camera move. That is the original's design, and the port keeps it, so speed and
  behaviour are the same at any frame rate. See "The frame loop and the frame rate" in
  `REVERSE_ENGINEERING.md`.
- **The original drew at most 100 frames a second**, normally about 60 (exclusive fullscreen
  with VSync), so it never showed more than one picture per tick.
- **Smooth motion off:** the port draws only on frames with a tick, at most 100 a second, as the
  original.
- **Smooth motion on (the default, from Hopmon Native):** the port draws at the monitor's rate,
  but no object knows where it is between two ticks, so the extra frames repeat the last tick's
  picture. At 144 Hz that is 144 frames a second with 100 different pictures: 44 of every 100
  ticks stay on screen for two frames and 56 for one, a fine judder. 100 Hz and 200 Hz look even;
  60 Hz has the same unevenness the original had.
- **The VSync work of 0.4.4** (no tearing, exact frame limits, adaptive VSync, the driver
  override warning) is about how frames reach the screen. It applies as it is and matters more
  once frames really differ, but it draws nothing new between ticks.

## What interpolation does

Each frame draws every moving thing between its pose at the previous tick and its pose at the
latest tick, by how far the clock is between the two ("alpha", 0 to 1). At 144 Hz that gives 144
different pictures a second, smooth at any refresh rate. The game logic, its speed and timing
are untouched: it is drawing only.

### Input lag

- What is shown trails the newest game state by alpha's distance from 1: between 0 and 10 ms
  (one tick), about 5 ms on average. Right after a tick the picture still shows the previous
  tick's pose and catches up over the next 10 ms.
- The game's response to input doesn't change: the heroes' keys are read every tick, as now
  (see 0.4.4); jumps, shots and their timing are decided exactly as before. Only the picture
  trails by up to a tick.
- For scale: the original at 60 Hz with VSync showed each frame for 16.7 ms, plus VSync's
  queueing. At 144 Hz with interpolation a frame lasts 6.9 ms plus at most 10 ms of blend, so
  the total delay is about the same as the original at 60 Hz or lower, with smoother motion.
- **Extrapolation** (predicting ahead instead of blending behind) avoids the delay but guesses
  wrong whenever something stops, turns, lands or is hit: overshoot and jitter. Not
  recommended; reimplementations that care about feel generally interpolate.
- Smooth motion off keeps today's behaviour exactly, with no added delay.

## What existed already (from Hopmon Native; replaced in chunk A)

- `src/framework/interp.{h,cpp}`: `Interp::Tracked` (a registry of objects with a
  `SnapshotPrev()`), `BeginFrameMove` (snapshots every tracked object), `Alpha()`,
  `SetSuppressed`, `Lerp`.
- `CD3DApplication::Render3DEnvironment` (`src/framework/d3dapp.cpp`): calls
  `Interp::BeginFrameMove()` before `FrameMove(ticks)` and sets `g_tickAlpha` = (time since the
  last tick) / 10 ms.
- `Device::SetWorldOffset` (`src/gfx/device.cpp`): an extra world-space offset added to every
  draw's world matrix (unused).
- Nothing in Mage Bros. derives from `Interp::Tracked`, so `SnapshotPrev` never runs on
  anything.

### What is wrong with it

1. **Snapshots once per frame, not once per tick.** At 60 Hz a frame often runs two ticks, so
   the previous pose would be two ticks old and the blend wrong. The snapshot has to come
   before every tick, inside the screens' own tick loops (`XGameScene::Tick` and the others).
   Calling the screens one tick at a time (`ProcessFrame(1)` n times) is not an option: the
   original reads the menus' keys (`JDInput::UpdateInputState`) and the shop's state once a
   frame, which the port keeps (see the "Stage 7: frame-rate dependence" table in `ROADMAP.md`).
2. **Alpha is measured in whole milliseconds** (`SDL_GetTicks`), giving steps of 0.1 and
   slightly uneven motion at 144 Hz. It should come from the high-resolution clock
   (`SDL_GetTicksNS` or the performance counter), while ticks keep the original's 10 ms rhythm.
3. **Nothing feeds it** (above).

## How objects would be blended (the first outline; the chunk plans below replaced its details)

**Centrally, in the two building blocks.** Nearly everything in a level is drawn through
`JObject3DX` (models: the mages, enemies, bosses, objects, platforms) or `JObject3DRect` (flat
quads: particles, shadows, shots, billboards). Each moving thing owns its own copy, and its pose
is set by the ticks: enemies call `UpdateWorldMatrix` in their tick (`XEnemy.cpp`), particles call
`SetWorldPos(pos_)` from their tick position when drawn (`XParticle.cpp`), the hero's models are
placed in its tick. So these two classes can keep the pose at the last tick themselves and draw
the blend, without touching the roughly 60 object classes one by one:

- position (`pos_` / `worldPos_`) blended in a straight line; yaw, pitch and roll the short way
  round; scale linearly; the world matrix rebuilt from the blended parts;
- a snapshot before each tick copies the current pose to the previous one; a new object starts
  with both the same, so it appears in place (pooled slots are refilled with new objects, which
  start fresh).

**Handled on their own:**

1. **The camera**, the most important: if the world blends and the camera doesn't, everything
   judders. `XCamera` builds `view_` each tick; it would keep the previous look-at point and
   shake, and the view drawn (and the billboards built from it, `Render2`) would use the blended
   camera.
2. **Jumps that must not blend** snap instead: level start, respawn, warp, `place=` (test hook),
   the camera's reset, platform wrap-arounds. Explicit snap calls there, plus a safety net that
   snaps any change over a set distance in one tick (nothing moves more than a few units a
   tick; the camera at most 5).
3. **A model drawn more than once a frame** at different places would break per-copy history.
   A debug check would log any tracked object drawn twice in one frame; each case found gets its
   own handling.
4. **Shadows** placed from a hero's position at draw time (`XShadow::SetPos(h->GetPos())`) take
   the hero's blended position.
5. **Animation frames** (walk cycles, model keyframes) switch on ticks as now: the models are
   separate keyframes and can't be blended.
6. **2D motion** blended from its tick counters where it moves smoothly: the shop's slide
   (`XShop::_SlideIn`), the ending's credits and film strips.

**Kept as they are:** the HUD; the title background's whole-pixel steps (it moves a pixel every
four ticks in the original); the venetian-blind fades; the ending's corridor scroll, which
already moves once a frame (kept as the original, the owner's decision of 2026-10-25).

## Proposed chunks

| Chunk | What | What the owner would notice |
|---|---|---|
| A: timing and camera (done, 0.4.16) | Snapshots before every tick, alpha from the high-resolution clock, snaps at resets; the camera, both mages and their shadows blended | Scrolling and the player's mage smooth at 144 Hz, most of what is seen |
| B: everything in a level (done: B1 0.4.17, B2 0.4.18) | `JObject3DX` blending for enemies, bosses (the dragon's body, the nest's birds), platforms and riders, objects, bullets; sprites (particles, shots, the flame) blended from their owners' positions; the drawn-twice check | All motion in a level smooth |
| C: other screens (done, 0.4.19: the shop's slide and the credits) | The shop's slide, the ending's credits and film strips, moving parts of the title and front-end screens | Menus and the ending match |
| D: polish (done, 0.4.20) | Frame pacing with the frame limit (measured: no change needed; the G-Sync / FreeSync "refresh minus 3" choice was planned, then dropped, see D1), help text, docs, the owner's play test at 60 and 144 Hz | Fine-tuning |

## Chunk A in detail (planned 2026-10-27)

Chunk A blends the camera, both mages and their shadows. Everything else in a level still moves
once a tick, as now, until chunk B. This section goes file by file. Where it differs from the
outline above, this section is the plan.

### Rules chunk A keeps

1. **The blend is for drawing only.** Nothing the ticks read changes. `JObject3DX::world_` stays
   the tick's matrix, because the ColDet collision tests use it (`UpdateWorldMatrix` hands it to
   `col_->setTransform`). The blended matrix is built separately, at draw time, and never stored
   where collision or game code can see it.
2. **Alpha 1 is today's picture, bit for bit.** At alpha 1 each blend returns the tick's value
   itself (not `a + (b - a) * 1`, which can round differently), and a model draws with `world_`
   itself. So "Smooth motion on, alpha held at 1" must give screenshots byte for byte the same
   as 0.4.15.
3. **Smooth motion off changes nothing.** The frame loop draws only on frames with a tick, alpha
   is 1, and the paths above make no difference.
4. **Only what is opted in blends.** Every enemy, object and particle also uses `JObject3DX` /
   `JObject3DRect`. In chunk A only the mages' two models opt in. Chunk B opts the rest in once
   their snaps and pooled slots are handled.

### 1. The clock and alpha (`src/framework/d3dapp.cpp`, `interp.{h,cpp}`)

- Add `AppTimeNs()`: `SDL_GetTicksNS()` from the same base as `AppTimeMs()`, so that
  `AppTimeMs() == AppTimeNs() / 1,000,000` always. The ticks stay on the millisecond clock
  (`m_dwLastFrameTime`, `elapsed / 10`), so their rhythm is exactly as now.
- Alpha = (`AppTimeNs()` − `m_dwLastFrameTime` × 1,000,000) / 10,000,000, clamped to 0..1. It
  is smooth to the nanosecond instead of in steps of 0.1.
- **Pause and the options menu freeze the phase.** Today the menu sets alpha to 1 and, while
  open, resets `m_dwLastFrameTime` to now, so closing it starts again at alpha 0. Both would make
  the blended picture jump. Instead, keep the alpha of the last drawn frame while the menu is
  open or P has paused the game. On resuming, set `m_dwLastFrameTime` so that the next tick comes
  due where the frozen alpha left off. The ticks are not affected: the game was not moving.
- `Interp::Alpha()` returns 1 when Smooth motion is off, when blending is suppressed, or when a
  test hook holds it (below).
- **The tick counter.** `Interp::BeginTick()` raises a global count, `Interp::CurrentTick()`.
  `XGameScene::ProcessFrame` calls it right before each `Tick()`, and before each tick of its
  shop loop (where the camera still moves). Calling it there, not once a frame in
  `Render3DEnvironment`, fixes the once-a-frame snapshot (problem 1 above). The screens keep
  their `ProcessFrame(n)` and read their input once a frame, as the original does.
- Remove `Interp::Tracked`, `BeginFrameMove` / `EndFrameMove` and the registry. Nothing uses
  them, and a global list of objects doesn't fit pooled slots. Remove `Device::SetWorldOffset`
  as well (unused).

### 2. Blending in `JObject3DX` (`src/game/JModel.{h,cpp}`)

- New members: `blend_` (off by default), the previous pose (`prevPos_`, `prevScale_`,
  `prevYaw_`, `prevPitch_`, `prevRoll_`) and `poseTick_`, the tick of the last change.
- **The snapshot is taken on the first change in a tick.** When `SetPos`, `SetScale` or
  `SetYaw` / `SetPitch` / `SetRoll` is called while `poseTick_ != Interp::CurrentTick()`, the
  current pose is copied to the previous one first, then `poseTick_` is set. The mage's moves
  (`SetPosX`, `SetPosZ`, `SetPosY`, `UpdateWorldMatrix`, several in a tick) snapshot only once.
  An object that didn't move in the latest tick has an old `poseTick_` and is drawn at its
  current pose, with no blend.
- `Snap()`: the previous pose becomes the current one. It is called after any jump that must
  not blend (below).
- **The safety net.** If any part of the position moved more than 40 (one map cell) in one tick,
  draw the current pose. The mage moves at most 4 a tick (its fall speed, `kMaxFallSpeed`), and
  knockback and platforms move it less.
- `Render`: if `blend_` is on, `poseTick_` is the latest tick and alpha is below 1, build the
  draw matrix from the blended parts. These are position in a straight line; yaw, pitch and roll
  the short way round (`NormaliseAngle` of the difference); and scale in a straight line. The
  matrix is built in the same order as `UpdateWorldMatrix` (scale × roll × pitch × yaw ×
  translation). Otherwise draw `world_` as now.
- `BlendedPos()` returns the blended position, for the shadow.

### 3. The mages (`src/game/XHero.{h,cpp}`)

- The constructor turns `blend_` on for `body_` and `wand_`.
- `Reset` (level start, the two-player start, the `place=` / `placeB=` test hooks) calls
  `Snap()` on both models after its `UpdateWorldMatrix`, so a mage dropped in at y 800 does not
  blend from where it was.
- `DrawPos()` returns `body_->BlendedPos()`.
- Nothing else changes. The walk frame (`walkCounter_`) and the wand frame still switch on ticks.
  The warp's spin (8 degrees a tick, then slowing) and the death spin blend the short way round.
  Knockback, platforms carrying the mage and hero on hero all change `pos_` in the ticks, so they
  blend with no extra work.

### 4. The shadows (`src/game/XGameScene.cpp`)

- `renderHero` passes `h->DrawPos()` to `shadow.SetPos` instead of `h->GetPos()`. `XShadow` is
  worked out entirely at draw time from that position and the map's floor heights, so it follows
  the blended mage, including where it splits over cells of different heights.
- The mage's shadow on a platform (`XShadowOnPlatform`) is the platform's, so it waits for
  chunk B.

### 5. The camera (`src/game/XCamera.{h,cpp}`)

- New members `prevLookAt_` and `prevEyeBase_`. The first thing `ProcessFrame` does is copy the
  current ones there. The camera runs every tick in a level (in the shop too), so a plain copy is
  enough here.
- `ResetForGame`, `ResetForTitle` and `ResetForEnding` set the previous values to the current
  ones (snap).
- `BlendedView()` builds the view the way `UpdateViewMatrix` does, from the blended look-at
  point and eye base (`distance_` and the angles never change in a level). At alpha 1 it returns
  `view_` itself.
- `XGameScene::Render` uses `BlendedView()` for `SetTransform(VIEW)` and for the `view` passed to
  `RenderObjects` / `RenderEffects`, which turns the billboards to face the camera.
  `DoSetTransform` and every other screen keep `view_`.
- **Kept on the tick's camera:** the draw range test (`|camZ - z| < 400 × ViewReachFactor`) and
  the sound panning. The set of objects drawn and the sound stay exactly as now. The draw range
  reaches well past the screen's edge, so blending the camera by under 5 units can't show an
  object popping in.
- The shake bobs the look-at point over 8 ticks. Blending draws the frames between those
  positions, a little rounder. Its timing and size are unchanged.

### 6. Where blending applies

- `XGameScene::Render` turns blending on for the level's 3D (the same stretch as
  `SetGameplayView(true)`) and off before the shop, the HUD and the fade. Every other screen
  draws exactly as now until chunk C.
- When a level starts or restarts (`XGameScene::Start`), the mages and the camera snap, as
  above.

### 7. Test hooks and checks (`src/main.cpp`)

- `--auto` gets `alpha=<0..1>`, which holds alpha at that value, and `alpha=off` to free it
  again. These are test hooks only, like `level=`.
- Checks before the owner tries it:
  1. Smooth motion off: screenshots of 1-1, 1-6, 2-4 and 3-5 (and the shop open) byte for byte
     the same as 0.4.15; `mbn_hero_tests` unchanged.
  2. Smooth motion on, `alpha=1`: the same screenshots, byte for byte.
  3. `alpha=0.5` while the mage runs, jumps and falls, and the camera scrolls: positions halfway
     between two ticks, checked by a temporary log of the mage's tick and drawn positions.
  4. Snaps: `place=` across a level, a level restart and two-player starts show no frame in
     between.
  5. Pause (P) and the options menu: the picture doesn't jump on opening or closing.
- Then the owner plays at 60 Hz and 144 Hz. Expected: the mage and the scrolling are smooth, and
  everything else still steps once a tick (chunk B). A mage riding a moving platform will drift a
  little against the platform until chunk B blends platforms too.

### Points checked in the code for this plan

- `JObject3DRect::Shift` moves the vertices themselves, but it is used only when the ending
  builds its floor and walls, once. Nothing moves with it in a tick, so it needs nothing.
- The heroes' models are placed only in ticks (`XHero::UpdateWorldMatrix`). `XHero::Render`
  picks the animation frames and changes no pose.
- Particles set their position while being drawn (`SetWorldPos(pos_)`). That is why chunk B
  blends them from their own `pos_`, and why blending is opt-in per object.
- A mage never respawns within a level (a death loses the level), so `Reset` and the level's
  start are the only snaps a mage needs.

## Chunk A as built (0.4.16)

Built as planned above, with these differences found while testing:

- **The ticks are still counted from the frame's millisecond time** (`now`, the time the `--auto`
  steps and the options menu see), with the latest tick kept in nanoseconds (`lastTickNs_`,
  `SetLastTick`, which also keeps `m_dwLastFrameTime`). Only alpha uses `AppTimeNs()`. Counting
  ticks from a nanosecond reading taken a moment later moved some key presses one tick off
  their `--auto` time (the 1-6 screenshot differed at alpha 1), so it was put back. The tick
  clock also starts on a whole millisecond, as before.
- **The first frame after the options menu or a pause** carries on from the held picture
  (`heldLastFrame_`). The menu draws at 60 frames a second, so its last frame could be up to
  16 ms old, and that was caught up in one step (about 1.6 ticks) when it closed. With Smooth
  motion off, the frame loop holds the time exactly as before (`HoldTickPhase` takes `tickNow`).
- **The camera blends only if it ran in the latest tick** (`blendTick_`), like the models'
  `poseTick_`. Otherwise a camera that stopped while ticks went on would swing between the same
  two views.
- After the level's 3D, `XGameScene::Render` sets the tick's own view again, so the shop's panel
  is placed exactly as before.

Checked (2026-10-27, 1280 x 960, 4:3):

- **Byte for byte the same as 0.4.15**, with Smooth motion off and on at `alpha=1`: 1-1 running
  and jumping (two shots), 1-6 after `place=` with the camera settled (two shots), 3-5, a
  two-player 2-4 and the open shop in 1-3 (two shots). 0.4.15 run twice gives the same pictures,
  except a 1-6 shot taken while the camera was still easing, which can land a tick apart in any
  build. `mbn_hero_tests` and the other tests pass.
- **`alpha=0.5`, from a temporary log of tick and drawn positions** (removed): in 1-1, each of
  858 frames drew the mage exactly halfway between its last two ticks. That included the drop-in
  and 452 frames of jumping and falling. In 1-6, each of 1001 frames drew the camera exactly
  halfway, 871 of them while it moved.
- **Snaps:** at `place=` mid-level (twice), in a two-player game, and at a level restart after
  Escape, the first frame drawn shows the mage at its new place, not on the way there.
- **Pause and the menu:** running through P, P and F10, Escape, no frame steps back, and the
  largest step (0.85) is the same as an ordinary frame's (0.83).
- A screenshot at `alpha=0.5` mid-jump in 2-4 shows the mage with its shadow under it.

The owner's first play test (2026-10-27): "feels pretty smooth", nothing off.

## Chunk B in detail (planned 2026-10-28)

Chunk B blends everything else that moves in a level. It is split in two, each a version of its
own with its own checks and play test:

| Part | What | Version |
|---|---|---|
| **B1: models** (done, 0.4.17) | Every `JObject3DX` in a level: enemies, bosses (the dragon's head and body, the genie, the nest and its 40 birds, the wizard with its blade and wand), platforms, moving objects (cannons, the turning cannon, the genie's lamp), coins, enemy bullets drawn as models (cannonballs, needle balls, rockets, the wizard's fireballs). Their shadows, the mage's shadow on a platform, and the drawn-twice check | 0.4.17 |
| **B2: sprites** (done, 0.4.18) | Everything drawn as a sprite from its owner's position: particles (smoke, flashes, explosions, rings, sparkles, the warp photons, the flames' flickers), the mages' shots, the dragon's flame; the water's drifting textures | 0.4.18 |

After B1, everything with a model moves smoothly. Effects and shots still step until B2. That
is the order of what is noticed most.

### What the code showed (read for this plan)

These findings replace guesses in the outline above:

- **Nothing is reused in place.** Enemies, platforms, static objects and the boss are made once,
  when the level loads (`XLevelLoop::Load`), and their slots are never refilled during play.
  Particles (`XParticleList::Add`), enemy bullets (`XBulletList::Create*`) and coins
  (`XLevelLoop::CreateCoin`) are each a new object when made. The mages' shots (`Fire`) are the
  only reused slots (`XLevelLoop::NewFire` overwrites a dead one). So the risk is not a reused
  slot. It is **a new object's first tick**: its constructor sets its pose with setters, which
  would take the model's default pose (the origin) as the earlier pose.
- **No platform wraps around.** The seven platform kinds move continuously (up and down,
  back and forth, the elevator, the drop). There are no jumps to snap.
- **No enemy respawns or teleports.** A dead enemy hops, spins and bursts, then is gone. Every
  move is continuous (the jumpers interpolate their own arc each tick).
- **Some models are hidden and shown again.** The wizard's blade and wand are drawn only while
  it attacks. The genie is hidden while in its lamp. Any object can leave and re-enter the draw
  range at the screen's edges. When one shows again, its earlier pose is from when it was last
  seen.
- **Sprites carry no history.** Particles, the shots and the dragon's flame are rectangles placed
  at their owner's position while being drawn (`SetWorldPos(pos_)` in `Render`). The shots even
  share one sprite per colour and size, drawn at each live shot in turn. So sprites blend from
  their owner's position history, not inside `JObject3DRect`.
- **Particle sizes and fades change the vertices** (`JObject3DRect::Scale`, `SetAlphaFactor`)
  every tick. They can't be blended without rewriting each effect, and the steps are small (3% or
  less a tick). They stay per tick (see "Kept per tick" below).
- **Shadows are worked out at draw time** from their owner's position: `XShadowSimple::SetPosXZ`
  (enemies, bosses, coins), `XShadow::SetPos` (the dragon, the birds, the wizard, the ghost,
  bullets), and `XShadowOnPlatform` (the mage's shadow on a platform, from the mage's position
  and the platform's box).
- **Enemies outside the update range (480) don't tick**, so they don't blend; they are drawn
  where they stand, as now.

### B1.1 Blending on for every model (`src/game/JModel.{h,cpp}`)

- `blend_` becomes **on by default** for every `JObject3DX`. This is safe because blending only
  ever applies inside `XGameScene::Render`'s 3D (`Interp::SetScope`). The title, level select,
  the ending and the Tab viewers draw exactly as now, and so does everything with Smooth motion
  off. This replaces opting in object by object, which would mean touching some 60 classes.
  `XHero`'s two `EnableBlending(true)` calls go; its `Snap()` calls in `Reset` stay.
- **A model not drawn in the last frame doesn't blend in the next tick.** `Interp` gets a frame
  count: `Interp::BeginFrame()`, called at the start of `XGameScene::Render`, and
  `Interp::FramesDrawn()`. `JObject3DX::Render` records `lastDrawFrame_`. In `TouchPose` (the
  first change in a tick), if `lastDrawFrame_` isn't the latest frame, the model is marked
  `snapTick_ = CurrentTick()`, and `Blending()` is false for that tick. This one rule covers:
  - **new objects** (coins, bullets, the level's objects): never drawn yet, so their first tick
    shows them where they are made, not on the way from the origin;
  - **objects shown again** (the wizard's blade and wand, the genie, anything coming back into
    the draw range);
  - at 60 Hz, two ticks in one frame still blend, because the model was drawn in the last
    frame.
- **The drawn-twice check:** if `Render` finds `lastDrawFrame_` already equal to this frame (the
  same model drawn twice in one frame, at different places), it logs the model's resource id
  once. Its history would be wrong. Nothing found in the code does this (the shots' shared
  sprite is a rectangle, handled in B2); the check is there to catch what reading missed. It is a
  log line only and stays in the release build (once per model id).
- The 40-unit safety net stays.

### B1.2 Shadows at the blended position

Each shadow placed at draw time takes its owner's blended position:

- `XEnemyBase::Render`, `XBossBase::Render`, the coins in `XLevelLoop::RenderObjects`:
  `SetPosXZ(model_->BlendedPos())`.
- `XBoss_Dragon`, `XBoss_DragonBody`, `XBoss_Bird`, `XBoss_Wizard`, the ghost (`XEnemy.cpp`,
  `ghostShadow_`), `XBulletBase::Render`: `SetPos(model_->BlendedPos())`.
- The fan and the floor fan keep their shadows at `startPos_` (they don't move); static objects'
  shadows don't move either.
- **The mage's shadow on a platform** (`XShadowOnPlatform::RenderShadow`): the mage's
  `DrawPos()` instead of `GetPos()`, and the platform's box moved by the platform's blend
  (`model_->BlendedPos() - pos_`, passed in by `XPlatformBase::Render`). The overlap of the
  mage's square and the platform's top is then worked out between the two blended positions.
  At alpha 1 both offsets are zero, so the shadow is the same as now.
- `XBossBase::Render`'s shadow scale (`scale_`, the genie growing out of its lamp) stays per tick:
  it changes the shadow's vertices, as the particle sizes do.

### B1.3 Riders on platforms

Nothing extra is needed. A platform pushes the mages standing on it by its own move in the same
tick (`PushHeroX` / `Y` / `Z`), so once platforms blend, platform and mage blend together.
That ends the small drift seen in 0.4.16. The mage's model takes the push at its next
`UpdateWorldMatrix`, one tick behind the platform's, exactly as the original draws it. Blending
both keeps that same offset at every tick.

### B1.4 What B1 doesn't change

- Animation frames (walk cycles, the dragon's jaw, the wizard's frames) switch on ticks as now.
- The update and draw ranges use the tick's positions (game logic and the set of objects drawn
  stay the same).
- Objects that never move (pillars, fences, trees, the shop and its sign, torches, candles, the
  fans' and hammers' bases) are untouched: their pose never changes, so they never blend.

### B2.1 Particles (`src/game/XParticle.{h,cpp}`)

- `XParticleBase` gets `prevPos_` and `prevTick_`. `XParticleList::ProcessFrame` copies `pos_` to
  `prevPos_` and stamps `prevTick_ = Interp::CurrentTick()` just before each particle's
  `ProcessFrame`.
- `XParticleBase::Render` places the sprite at `Interp::Lerp(prevPos_, pos_, Alpha())` when
  `prevTick_` is the latest tick, otherwise at `pos_`. A particle made during a tick has no
  stamp yet, so it shows where it is made until its first tick of its own.
- The 40-unit safety net applies here too.
- The warp photons circle the mage using the mage's tick position. Blending their own `pos_`
  makes them circle smoothly, and they follow the mage's blended path to within one tick's
  movement of the mage, as now.

### B2.2 The mages' shots (`src/game/XLevelLoop.{h,cpp}`)

- `Fire` gets `prevPos` and `prevTick`. `ProcessFire` stamps them at its start (it already keeps
  `prev` for the flash). `NewFire` clears the stamp, so a new shot shows at its starting point
  20 out from the mage until its first tick.
- `RenderEffects` sets the shared sprite to each shot's blended position before drawing it. The
  shared sprite has no history of its own, so nothing else changes.

### B2.3 The dragon's flame (`src/game/XBullet.cpp`)

- `XBulletBase` gets `prevPos_` / `prevTick_`, stamped at the start of `XBulletList::ProcessFrame`'s
  call for each bullet. `Flame::Render` uses the blended position. (The model bullets blend in
  B1 through their models.) When the flame stops against a wall it goes back to where it was
  (`pos_ = prev`), which is no move at all, so it simply stands.

### B2.4 The water (`src/game/XWater.{h,cpp}`)

- The four texture offsets (`waterU1_`, `waterV1_`, `waterU2_`, `waterV2_`) keep their values
  before the latest tick. `XWater::Render` sets the UVs from the blended offsets (taking the wrap
  at 1.0 into account: when the new value is smaller, 1 is added before blending), and the next
  tick sets them from the tick's values as now. At alpha 1 the UVs are the tick's own, so the
  pictures are the same as now. The drift is slow (at most 0.0014 of a texture a tick), so this
  is the least noticeable item and is last.

### Kept per tick (not blended)

- **Particle sizes and fades**, and the boss shadow's scale: they change vertices each tick, in
  steps of a few percent at most. Blending them would mean rewriting the 20-odd effects. If the
  owner sees stepping in any effect, it can be looked at on its own.
- **Animation frames** of models and sprite sheets (the flame's explosion frames, the torches'
  flicker every 2 ticks): they are separate pictures.
- **The HUD and 2D screens**: chunk C.

### Checks (as in chunk A, for each of B1 and B2)

1. **Byte for byte** against the build before, with Smooth motion off and on at `alpha=1`: the
   six chunk A scenes (1-1, 1-6 settled, 3-5, two players in 2-4, the shop in 1-3), plus scenes
   with things moving: slimes and bees in 1-1, the moving platforms of 1-5 and 1-6, the cannons,
   the dragon in 1-10, the genie (2-10), the nest (3-10), the wizard (4-10), and fire and smoke
   in the air.
2. **`alpha=0.5` from a temporary log** of tick and drawn positions: every enemy, platform and
   boss part (B1), every particle and shot (B2) drawn halfway between its last two ticks, apart
   from the expected exceptions (first tick of a new object, first tick after reappearing). The
   log also counts those exceptions, so none is unexpected.
3. **The drawn-twice log**: run through every world's boss level and a few busy levels. It
   should log nothing.
4. **A mage on a moving platform**: the shadow on the platform and the mage stay together
   (screenshots at `alpha=0.5` on 1-6's elevator and 1-5's platforms).
5. **The wizard's attacks and the genie coming out**: no blade, wand or genie gliding in from
   where it was last seen.
6. `mbn_hero_tests` and the other tests unchanged.
7. Then the owner's play test.

### Risks

- **Many more models blend at once.** Each builds a matrix a frame. A level draws at most a few
  hundred models, which is cheap at any frame rate, but the frame rate with Show FPS is checked
  in the busiest level (World 4) before and after.
- **Something the reading missed** (a hidden jump under 40 units, a model drawn twice). The
  drawn-twice log and the `alpha=0.5` log are there to find it. Smooth motion off is always the
  way to confirm that a problem is in the drawing only.

## B1 as built (0.4.17)

Built as planned, with these points found while building and testing:

- **Shadows take the owner's position plus the model's blend offset**
  (`JObject3DX::BlendOffset()`, exactly zero when not blending), not the model's blended position.
  An owner's position can differ from its model's: a platform pushes a mage after the mage has
  placed its model for the tick. The original draws the shadow from the owner's position.
- **0.4.16 had this slip for the mage's shadow** (`XHero::DrawPos` was the body model's position).
  It applied with Smooth motion off too, while a moving platform carried the mage. Fixed: the
  mage's own position plus the offset.
- **The 40 nest birds wait at the origin** (their models aren't placed until the nest wakes),
  far out of view, and move to the nest in one tick when it wakes. The 40-unit safety net draws
  that tick as it is. This is the port's existing behaviour, unchanged.
- No model was found drawn twice in a frame in any run (the log stayed empty).

Checked (2026-10-28):

- **Byte for byte**, Smooth motion off and on at `alpha=1`, against 0.4.15 and 0.4.16: the chunk A
  scenes, plus the 1-6 elevator with the mage riding it, the dragon in 1-10, the wizard in 4-10,
  and 1-1 with a shot fired (two shots each). The genie (2-10) and nest (3-10) scenes can't be
  compared by screenshot: 0.4.15 itself gives different pictures run to run there, because the
  time a level takes to load moves the tick a screenshot lands on.
- **`alpha=0.5`, from a temporary log of every model drawn** (removed). Runs: 1-1, the 1-6
  elevator, the dragon fight, the genie coming out of its lamp (the mage within 80 of the lamp),
  the nest woken by shots, and two wizard fights (30 s and 60 s). Over about 650,000 model draws:
  every blended draw exactly halfway, every earlier pose exactly the model's pose in the tick
  before, and no model moving without blending except the expected cases. Those were new or
  reappearing models (the wizard's wand and fireballs, the genie) and the birds' one jump. The
  wizard's blade never came up in these runs (the wizard picks its attacks at random). It is
  shown and hidden by the same code as the wand, which was checked.
- **Frame rate:** the same build in the nest level (3-10, all 40 birds), VSync off, a temporary
  per-second log: about 1,070 frames a second at `alpha=1` (no blending) and at `alpha=0.5`
  (every model blending), two runs each. Blending costs nothing measurable.
- The mage on the 1-6 elevator at `alpha=0.5`: mage, shadow and elevator drawn together.
- The owner's play test (2026-10-28): everything looked fine and felt smooth. To be revisited if
  a bug turns up later.

## B2 as built (0.4.18)

Built as planned. Each sprite's owner keeps its position before the latest tick, stamped just
before the owner's tick (`XParticleList` and `XBulletList` call `KeepPrevPos`; `ProcessFire`
stamps the shot itself). The sprite is drawn at `Interp::BlendPos(prev, cur, prevTick)`, which
also holds the 40-unit safety net. The water keeps its four drift offsets before the tick,
draws with the blended ones (allowing for the wrap at 1), then puts the tick's back.

Checked (2026-10-28):

- **Byte for byte**, Smooth motion off and on at `alpha=1`, against 0.4.15: the same 16 shots as
  B1, including 1-1 with a shot in flight and its smoke, the dragon fight and water levels.
- **`alpha=0.5`, from a temporary log of every sprite drawn** (removed). Runs: 1-1 firing
  repeatedly, the dragon fight (45 s, with its flame), the genie coming out (its steam), the nest
  woken by shots, and the wizard (40 s). Over about 800,000 sprite draws (particles, about 5,000
  shot draws, 12,000 flame draws): every blended draw exactly halfway, every earlier position
  exactly the one from the tick before, and nothing moving without blending.
- **The flash where a shot hits** is made where the shot was a tick before it hit, as in the
  original. The shot's last frames are drawn between that point and the hit, so the flash
  appears where the shot was last seen. The log shows no shot or particle out of place.
- Screenshots at `alpha=0.5`: shots and their smoke in 1-1, the dragon's flame over the water in
  1-10.
- The water's drift wasn't logged (it moves at most 0.0014 of a texture a tick); at alpha 1 it is
  byte for byte the same as before.

## Chunk C in detail (planned 2026-10-28)

Chunk C covers what moves outside a level's 3D: the shop and the screens around the levels.
Reading them showed that little of it slides smoothly in the original. Most of the 2D motion
moves in whole 640 x 480 pixels, a pixel every few ticks.

### What moves, screen by screen (read for this plan)

| Screen | What moves | How | Plan |
|---|---|---|---|
| Shop (`XShop`) | The board and the shopper's line slide in from the left, the items and exit from the right, the shopkeeper rises from the bottom; the panel and the widescreen dim fade with them | `_SlideIn()`: the fade counter / 40, so about 8 pixels a tick across 40 ticks | **Blend** (C1) |
| Ending (`XEndingObject`) | The credits slide in and out | `creditPosX_`: 2.5 pixels a tick | **Blend** (C2) |
| Ending | The film strips scroll left | A whole pixel every 4 ticks (`localCounter_ * 0.25`) | Whole pixels, as the original (see the decision below) |
| Ending | The corridor's textures | Once a frame (`textureOffset_`) | Kept at first (the owner's decision of 2026-10-25); changed while building C, see "Chunk C as built" |
| Ending | The two mages | Walk frames only; they don't move | Nothing to do |
| Title, save slots, level select | The tiled green background | A whole pixel every 4 ticks (`RenderTiledBackground`) | Whole pixels, as the original |
| Title | The pictures fade in; the logo's glow pulses | Alpha, 0.002 a tick; a 300-tick sine | Too small to see as steps; kept |
| Every screen | The venetian-blind fade | Each strip opens a whole pixel every 4 ticks | Whole pixels, as the original |
| Every screen | Blinking arrows and cursors, the HUD's hearts | Picture frames | Kept (frames) |
| Logo screen, HUD | Nothing moves | | Nothing to do |

### C1 The shop's slide (`src/game/XShop.{h,cpp}`, `XGameScene.cpp`)

- `_SlideIn()` gets a blended form. During FADEIN and FADEOUT the counter used is
  `localCounter_ - 1 + alpha` instead of `localCounter_`. `XShop::ProcessFrame(n)` adds `n`
  ticks at once, but each tick adds one, so `localCounter_ - 1` is the count one tick earlier.
  When the fade-in has just turned into PROCESS (the counter reset to 0 in the latest tick),
  the slide is drawn from 39/40 to 1, so the board doesn't jump its last 8 pixels. That case is
  marked by a flag set when the state changes, cleared at the next tick.
- Everything placed from `t` follows: the board's and items' x (still whole pixels, as now), the
  shopkeeper's rise, the panel's alpha (`back_`) and the widescreen dim (`RenderDim`).
- `XGameScene::Render` turns the blending scope on around `RenderDim` and `XShop::Render` (it is
  off there today, so `Interp::Alpha()` gives 1). The shop's panel stays placed by the tick's
  camera, as in chunk A.
- At alpha 1 the counter is the tick's own, so the shop is drawn exactly as now. The blinking
  cursor stays on ticks.

### C2 The ending's credits (`src/game/XEndingObject.{h,cpp}`)

- The credit's x is worked out at draw time from the state and a fractional tick: APPEAR
  `820 - (tick_ - 1 + alpha) / 200 * 500` and DISAPPEAR likewise from 320, rounded down to whole
  pixels as now. In WAIT1 and WAIT2 it stands still. When the state changed in the latest tick
  (`tick_` reset to 0), the earlier value is the end of the phase before, so APPEAR → WAIT2 blends
  from 322 to 320 and the credit doesn't jump. DISAPPEAR → the next credit's WAIT1 is a jump from
  past the left edge to past the right one, off screen, so it isn't blended.
- `XEndingObject::Render` turns the blending scope on for the credits. The corridor, the mages
  and the film strips are drawn as now.
- At alpha 1 the credit's x is the tick's own, as now. (The ending can't be compared screenshot
  by screenshot because of the corridor; the check is a log of the credit's x, as in 0.4.11.)

### Decided (the owner, 2026-10-28): whole pixels, as the original

The film strips, the green background and the fade strips move a whole 640 x 480 pixel every 4
ticks, 25 pixels a second, in the original at any frame rate. On a 1280 x 960 window each of
those pixels is 2 screen pixels; on a 4K screen about 4.5. Blending can't smooth them while the
2D is drawn at whole pixels, because between ticks they have nowhere to go: the blended position
rounds to the same pixel.

- **Keep whole pixels (chosen):** the screens look exactly like the original's, a slow scroll
  in visible pixel steps. Chunk C then only touches the shop and the credits.
- **Sub-pixel 2D (possible later):** `Device::BltToBackBuffer` would take fractional positions,
  so these scroll smoothly at high resolutions. It changes the original's pixel look (the tiles'
  edges would blend across pixels), touches every 2D draw path, and would be a setting of its
  own. Not part of chunk C; not planned.

### What chunk C doesn't change

- The HUD, the blinking arrows and cursors, the heart animation: frames on ticks.
- The corridor's once-a-frame scroll (the owner's decision; changed with Smooth motion on while building C, below).
- With Smooth motion off, and at `alpha=1`, every screen is drawn exactly as now.

### Checks

1. **Byte for byte**, Smooth motion off and on at `alpha=1`, against 0.4.15: the shop scene (open)
   plus new shots **while the shop is sliding in and out** (1-3), the title, save slots and
   level select (their backgrounds and fades), and the earlier level scenes.
2. **`alpha=0.5`, from a temporary log**: the shop's slide fraction each frame halfway between its
   last two ticks, through FADEIN, the turn into PROCESS and FADEOUT; the credits' x through
   APPEAR, WAIT2 and DISAPPEAR for all four credits.
3. Screenshots at `alpha=0.5` of the shop half open and a credit mid-slide.
4. The owner's play test: opening and leaving a shop, and the ending (F3 in a level reaches it).

Version 0.4.19. Chunk C is small enough to build in one part.

## Chunk C as built (0.4.19)

Built as planned. `XShop::SlideInBlended` uses the fade counter one tick earlier plus alpha, and
(39 + alpha) / 40 in the tick the fade-in turns into PROCESS (`justOpened_`). It is used by the
board, items, shopkeeper, panel and widescreen dim. `XEndingObject::CreditPosXBlended` gives the
credit's x on the slide's exact line, 2.5 a tick (from the end of the earlier phase when the state
changed in the latest tick). `XGameScene::Render` turns the scope on around the
shop only, not the status display; the ending around the credit's x only.

Checked (2026-10-28):

- **Byte for byte**, Smooth motion off and on at `alpha=1`: 24 screenshots of 1-3 taken every
  100 ms while the shop slides in, stays open and slides out after Escape. Every one matches a
  0.4.15 run: 0.4.15 itself lands a few of these a tick apart from run to run (load timing), and
  the new build always gives one of its two pictures. The 14 level shots also match.
- **`alpha=0.5`, from a temporary log** (removed): the shop's slide fraction exactly halfway
  between its last two ticks on all 242 frames with a previous tick, through the fade-in, the turn
  into PROCESS and the fade-out. The credit's x was halfway (to the nearest pixel) on 8,109 frames
  across all four credits; the 6 others are new credits starting off screen. The largest step
  between frames was 3 pixels.
- A screenshot at `alpha=0.5` of the shop half open.

**Changed after the owner's first look (2026-10-28), who thought the ending looked slightly
jittery.** A per-frame log at 144 Hz with alpha running freely found two causes:

1. **The corridor stood still on about one frame in three** (721 of 2,360 frames). It scrolls
   once per frame that runs a tick (the original's once a frame); with Smooth motion off every
   drawn frame runs a tick, so it was even at 100 a second, but with Smooth motion on only about
   100 of 144 frames do. The owner first chose the original's 60 Hz speed, then (after trying it)
   the speed it had before: with Smooth motion on, once a tick at `0.008 x min(frame rate, 100) /
   100` (`CorridorPerTick`, from the measured frame rate, `AppFrameRate`), drawn between ticks.
   Measured afterwards: at 144 Hz 0.800 a second, even on every frame (Smooth motion off: 0.800
   overall, 0.69 to 1.00 frame to frame); at a frame limit of 60, 0.480 a second, even. With
   Smooth motion off it is the original's.
2. **The credit stepped 2, 2, 1 whole 640 x 480 pixels a frame** (4, 4, 2 screen pixels in a
   1280 x 960 window), and blending between the original's rounded positions (which step 2, 3, 2,
   3 a tick) made its speed wobble between 2 and 3 pixels a tick. The owner chose: while blending,
   the credit is placed between pixels (the fraction as a shift of the blit,
   `Device::BeginHudScale`'s shift) on the slide's exact line. Measured afterwards: 2.49 to 2.51
   pixels per 10 ms on 99% of frames; the rest are the original's one-tick pause as each slide
   begins. At alpha 1, and with Smooth motion off, the credit is on the original's whole pixels.
3. **Found by the owner after that: the Saito Games logo (the last credit) jumped slightly right
   when the button was pressed** (VSync and Smooth motion on). The blend takes a credit whose
   `tick_` is 0 in WAIT2 / WAIT_FOR_INPUT as just arrived and draws the slide's last 2.5 pixels;
   the fade-out that the button starts also resets `tick_` to 0, so for the first frames of the
   fade-out the logo was drawn up to 2.5 pixels right. Fixed: the credits blend only while the
   ending is in PROCESS. Checked by a log: through the 365 frames of the fade-out the logo stayed
   at x 320.

## Chunk D in detail (planned 2026-10-28)

The last chunk: the words players read, a tidy-up of the docs, the final checks and the owner's
play test, then what the merge into `main` needs. Version 0.4.20. (A G-Sync / FreeSync frame
limit was planned here too and dropped by the owner; it is kept below, D1, in case it is wanted
later.)

### Frame pacing, measured first (2026-10-28, 0.4.19)

A temporary log of every frame (when it was presented, and the game time it showed) in 1-1 with
the mage running, 1280 x 960 window on the 144 Hz monitor, Smooth motion on:

| VSync | Frame limit | Frames a second | Frame time (1% to 99%) | Shown motion against real time |
|---|---|---|---|---|
| On | Unlimited | 144 | 6.86 to 7.04 ms | 0.10 ms (sd) |
| Off | Unlimited | about 1,790 | 0.43 to 1.06 ms | 0.17 ms |
| Off | 141 | 141 | 6.29 to 7.82 ms | 0.03 ms |
| On | 141 | 141 | 6.22 to 7.94 ms | 0.04 ms |
| Off | 60 | 60 | 15.81 to 17.46 ms | 0.04 ms |
| On | 60 | 60 | 15.78 to 17.54 ms | 0.04 ms |

"Shown motion against real time" is how far the game time each frame shows strays from the time
between presents: in every setup it is a fraction of a millisecond, so each picture shows the
game where it should be when it is shown. The frame limit holds its rate exactly on average,
with frames within about 0.8 ms either side (the limiter's sleep). **No pacing change is
needed.** The log can't see the monitor itself: whether a limit below the refresh rate shows
evenly depends on G-Sync / FreeSync (D1).

### D1 The "Refresh - 3" frame limit: dropped (the owner, 2026-10-28), kept here for later

**Not built.** The owner decided it isn't needed, and asked for it to stay documented in case it
is wanted later. Why it was dropped:

- **The gain is small.** It only matters on a G-Sync / FreeSync monitor with VSync on, when the
  game would otherwise reach the refresh rate: there VSync takes over again and can add up to a
  frame of delay (7 ms at 144 Hz). Capping 3 below keeps the monitor in its variable range. In a
  game like this one, a few milliseconds are hardly felt.
- **Players who want it already have it.** The NVIDIA app / Control Panel ("Max Frame Rate") and
  AMD Software cap any game at refresh - 3, which is what most G-Sync / FreeSync owners do, and
  the ini's `max_fps` takes any number (141 on a 144 Hz monitor).
- **It is another setting to explain and test**, and only the owner, on a monitor with variable
  refresh, could check that it does its job.
- **The frame pacing measured above is already good**, so nothing else depends on it.

**How it would be built**, if wanted later (about a day's work, in these files):

On a G-Sync or FreeSync monitor with VSync on, capping the frame rate a little below the refresh
rate keeps every frame inside the monitor's variable range: no VSync wait (less delay) and no
tearing. The drivers' makers recommend about 3 below the refresh rate. It became worth
considering with Smooth motion, where frames really differ (the owner had set it aside for this,
2026-10-25).

- `max_fps = -1` means "the refresh rate less 3", worked out from the monitor the window is on
  (its current mode, so exclusive fullscreen's chosen rate too), each frame from a cached value
  refreshed on a display change. `config.cpp` accepts -1 (it clamps to 0 or more today); the ini
  comment explains it.
- The Frame limit row offers it right after Unlimited, shown as "Refresh - 3 (141)" with the
  number it gives. Its help line: for G-Sync / FreeSync monitors with VSync on.
- `JudgeVSync` skips its verdict with any limit, -1 included (as it does for a number today).
- Like every frame limit it applies with Smooth motion on only (off, the game draws on ticks, at
  most 100 a second).
- Checked by the log above's method at -1 on the 144 Hz monitor (141) and on the 60 Hz one (57).
  Whether the monitor's variable refresh takes it can only be seen by the owner (the NVIDIA
  Control Panel's G-Sync indicator, or the monitor's own refresh readout).
- The words: the Frame limit help line and the ini comment for `max_fps` would mention it, and
  the README's Video section.

### D2 The words players read

- **Options > Video help lines**:
  - Smooth motion: what it does and the cost ("the whole game drawn between its steps, smooth at
    any refresh rate; the picture is up to 10 ms behind").
  - Frame limit: that it only applies with Smooth motion.
  - VSync Off: keep its "with Smooth motion, set a Frame limit".
- **The ini comments** for `interpolation`, `max_fps` and `vsync`.
- **README**: the Video section's Smooth motion and Frame limit paragraphs as they stand at the
  end; the "what works today" lines.
- **CHANGELOG** 0.4.20.

### D3 Tidy the docs

- **SMOOTH_MOTION.md** gets a short "How it works (as built)" section at the top: the clock and
  alpha, `Interp` (ticks, frames, scope, `BlendPos`), `JObject3DX` blending with the not-drawn
  rule and `BlendOffset`, the camera, the sprites, the water, the shop and the credits, the
  corridor, what stays per tick, and the test hooks. The research and the chunk plans stay below
  it as the history, with the outline's superseded parts marked.
- **CLAUDE.md**'s smooth motion paragraphs are shortened to that summary plus a pointer, so the
  handoff file stays readable.
- **ROADMAP**: smooth motion's status; the G-Sync / FreeSync item noted as decided against for now
  (D1 here).

### D4 Final checks

1. The byte-for-byte suite once more, with Smooth motion off and on at `alpha=1`: the 14 level
   shots and the shop sliding (24 shots).
2. `mbn_hero_tests` and the other tests.
3. The owner's play test, the last before the merge:
   - the whole game at 144 Hz with Smooth motion on (a world or a few levels of each, the
     shop, a boss, the ending);
   - a few levels on the 60 Hz screen (the TX-NR646) with Smooth motion on;
   - Smooth motion off once, to see it is the original's.

### D5 For the merge into `main` (the owner's decisions, after D)

- **Release notes (decided, the owner, 2026-10-28): a summary in 0.4.20.** `release.yml` publishes
  the version in `CMakeLists.txt` when `main` is pushed, with that version's CHANGELOG section,
  so only 0.4.20 gets a GitHub release (0.4.16 to 0.4.19 were never released). Its CHANGELOG
  section sums up all of smooth motion for players (bold lines and bullets only, no `## `
  headings: the workflow's extraction stops at the next one; checked, 34 lines up to 0.4.19's
  heading), followed by what is new in 0.4.20 itself. The 0.4.16 to 0.4.19 sections stay below
  as the detail.
- **How to merge (decided, the owner, 2026-10-28): directly**, no pull request: `interpolation-test`
  merged into `main` with a merge commit and pushed, which released v0.4.20. The branch is kept
  on GitHub (not deleted) unless the owner asks.
- Smooth motion stays **off by default** (decided).

## Chunk D as built (0.4.20)

- **The words**: the Smooth motion help line ("drawn between game ticks, smooth at any refresh
  rate; up to 10 ms behind"), the Frame limit help line (Smooth motion only; VSync also caps it),
  the ini notes for `interpolation` and `max_fps`, the README's Video section (a fuller Smooth
  motion paragraph: what it does, the cost, what keeps the original's look) and its status line.
- **The docs**: "How it works (as built)" at the top of this file; the research marked as history;
  CLAUDE.md's smooth motion passages shortened to a summary and a pointer; the ROADMAP's status.
- No code beyond those texts.

Final checks (2026-10-28):

- `mbn_hero_tests` and the other tests pass.
- **Byte for byte**, Smooth motion off and on at `alpha=1`, against 0.4.15: all 48 shop-slide shots
  match a 0.4.15 run; 27 of the 28 level shots match exactly. The 28th (the dragon in 1-10 at
  14 s) matched in 6 of 8 runs; the 2 others differed from it in 34 pixels by 1 in 255, in the
  flame's glow over the water: the graphics card's rounding when blending see-through effects,
  with everything in the same place.
- **A trap found on the way**: a first run differed everywhere because a test window had been
  maximized (the ini kept `maximized = 1`), which scales the picture. With the window setting
  put back, everything matched. When a byte-for-byte check fails everywhere, check the window
  settings in the test folder's ini before anything else.

The owner then chose to merge (2026-10-28), with the release notes summed up in 0.4.20 (D5).

## How the game logic is shown untouched (the method, used in every chunk)

- Smooth motion off: screenshots byte for byte the same as 0.4.15; `mbn_hero_tests` (800,000
  ticks) unchanged.
- Smooth motion on with alpha held at 1 (`--auto alpha=1`): screenshots byte for byte the same.
- Alpha held at 0.5, with temporary logs of tick and drawn positions: every blended object
  exactly halfway between its last two ticks, with the right earlier pose.
- The owner's play tests at 144 Hz (and 60 Hz): smoothness is felt more than seen in
  screenshots.

When a byte-for-byte run fails everywhere, check first that the test folder's ini hasn't kept
`maximized = 1` (see "Chunk D as built").

## Decisions (all settled)

- **The default: off** (2026-10-27), following the project's rule that the original's feel is
  the default. It was on by default before 0.4.16 (from Hopmon Native, where it only repeated
  frames); config_version 21 turns it off once in older ini files, and the help line and README
  say how to turn it on.
- **Versions on the branch: each build took the next version** (2026-10-27; 0.4.16 to 0.4.20),
  so builds were easy to tell apart. The owner first chose no bump, then changed this the same
  day. Pushes to the branch published nothing; v0.4.20 was released at the merge.
- **Where it ended up:** stage 7, done as one piece of work in four chunks, not a stage of its
  own.
- The release notes and the merge: see "D5" (a summary in 0.4.20; merged directly).
