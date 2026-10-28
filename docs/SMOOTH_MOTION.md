# Smooth motion (interpolation): research and plan

Research for drawing the game smoothly at any refresh rate, done on 2026-10-26 after 0.4.15.
Chunk A was planned file by file on 2026-10-27 ("Chunk A in detail", below) and built the same
day in 0.4.16 ("Chunk A as built"), and the owner's first play test found nothing off. Chunk B
is next. The work happens on the `interpolation-test` branch, apart
from `main`, until the owner has tested it and says to merge.

## Where things stand (0.4.15)

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

## What exists already (from Hopmon Native)

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

## How objects would be blended

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
| B: everything in a level (planned in detail below: B1 models, B2 sprites) | `JObject3DX` blending for enemies, bosses (the dragon's body, the nest's birds), platforms and riders, objects, bullets; sprites (particles, shots, the flame) blended from their owners' positions; the drawn-twice check | All motion in a level smooth |
| C: other screens | The shop's slide, the ending's credits and film strips, moving parts of the title and front-end screens | Menus and the ending match |
| D: polish | Frame pacing with the frame limit (the G-Sync / FreeSync "refresh minus 3" choice becomes worth having), help text, docs, the owner's play test at 60 and 144 Hz | Fine-tuning |

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
| **B2: sprites** | Everything drawn as a sprite from its owner's position: particles (smoke, flashes, explosions, rings, sparkles, the warp photons, the flames' flickers), the mages' shots, the dragon's flame; the water's drifting textures | 0.4.18 |

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


- Smooth motion off: screenshots byte for byte the same as 0.4.15; `mbn_hero_tests` (800,000
  ticks) unchanged.
- Smooth motion on with alpha held at 1 (a test hook for `--auto`): screenshots byte for byte
  the same as today.
- Alpha held at 0.5: screenshots showing poses exactly halfway between ticks.
- The owner's play test at 60 Hz and 144 Hz: smoothness is felt more than seen in screenshots.

## Open decisions

- **The default (decided 2026-10-27): off.** The owner chose off, following the project's rule
  that the original's feel is the default. Smooth motion is on by default today (from Hopmon
  Native), where it only repeats frames. Once it blends, it adds 0 to 10 ms of display delay,
  so it becomes something players turn on. The branch changes the default in `src/core/config`.
  A `config_version` bump resets the setting once in older ini files, as 0.4.13 did for HUD
  position. The help line and README then say how to turn it on.
- **Versions on the branch (decided 2026-10-27): each build takes the next version** (chunk A is
  0.4.16), so it is easy to tell which build is which. The owner first chose no bump, then
  changed this the same day. A push to this branch publishes nothing: its versions are released
  when it is merged into `main`. While the branch is open, work on `main` must not take the same
  numbers.
- **Where it ends up:** perhaps a stage of its own ("Stage 8: smooth motion") rather than a
  stage 7 chunk.
