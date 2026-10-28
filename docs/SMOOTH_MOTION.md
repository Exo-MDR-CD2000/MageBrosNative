# Smooth motion (interpolation): research and plan

Research for drawing the game smoothly at any refresh rate, done on 2026-10-26 after 0.4.15.
Chunk A was planned file by file on 2026-10-27 ("Chunk A in detail", below). No code has been
written for it yet. The work happens on the `interpolation-test` branch, apart
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
| A: timing and camera | Snapshots before every tick, alpha from the high-resolution clock, snaps at resets; the camera, both mages and their shadows blended | Scrolling and the player's mage smooth at 144 Hz, most of what is seen |
| B: everything in a level | `JObject3DX` / `JObject3DRect` blending for enemies, bosses (the dragon's spline body, the nest's birds), platforms and riders, objects, shots, bullets, particles; the drawn-twice check | All motion in a level smooth |
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

## How the game logic is shown untouched

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
- **Versions on the branch.** `main` may release new versions while this branch is open, so
  builds here should not take the next patch number. Not decided yet: build with
  `make_release.bat none` and give the work one version when it is merged, or merge it as a
  minor version (0.5.0).
- **Where it ends up:** perhaps a stage of its own ("Stage 8: smooth motion") rather than a
  stage 7 chunk.
