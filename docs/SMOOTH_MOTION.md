# Smooth motion (interpolation): research and plan

Research for drawing the game smoothly at any refresh rate, done on 2026-10-26 after 0.4.15. No
code has been written for it yet. The work happens on the `interpolation-test` branch, apart
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

## How the game logic is shown untouched

- Smooth motion off: screenshots byte for byte the same as 0.4.15; `mbn_hero_tests` (800,000
  ticks) unchanged.
- Smooth motion on with alpha held at 1 (a test hook for `--auto`): screenshots byte for byte
  the same as today.
- Alpha held at 0.5: screenshots showing poses exactly halfway between ticks.
- The owner's play test at 60 Hz and 144 Hz: smoothness is felt more than seen in screenshots.

## Open decisions

- **The default.** Smooth motion is on by default today (from Hopmon Native), where it only
  repeats frames. Once it blends, it adds 0 to 10 ms of display delay. On suits most players
  with high-refresh screens; the project's rule that the original's feel is the default argues
  for off. To be decided by the owner before the branch is merged.
- **Where it ends up:** perhaps a stage of its own ("Stage 8: smooth motion") rather than a
  stage 7 chunk.
