# The Statistics Overlay (O1 built as 0.4.34, O2 as 0.4.35; O3 optional)

A panel of statistics and information drawn over the game, turned on and off by the player: how
the game is running (frame rate, frame times, the game's ticks, smooth motion's blend) and on what
(the display settings in effect, the graphics card and driver, the build). Planned with the owner
on 2026-11-10, from the "in-game debug overlay" set aside on 2026-10-27. Off by default.

Why: each frame timing fix (0.4.27 to 0.4.32) was measured with temporary logging, removed before
its release. The overlay keeps those measurements in the game, so the owner can check any machine
and a player's screenshot (F12) shows both how the game runs and on what: the hardware the owner
doesn't have (AMD and Intel graphics, 60 Hz monitors, 32:9 screens). It is also the "measuring
build" noted for the window / borderless smoothness (`CLAUDE.md`, "Windowed / borderless
smoothness").

## Rules

- **It only reads.** It shows values the frame loop already works out, plus a short history of
  recent frames. It never changes the game logic, the tick order or when a frame starts, so the
  game's speed and behaviour are the same with it on or off.
- **Off, nothing changes**: screenshots byte for byte as the build before it, on the logo, the
  title, a level and the ending.
- **On, the cost is small**: a few clock reads a frame, and one small panel drawn in the menu's
  font (`UIDraw`: Classic 8x8, or from 0.4.36 the player's own) and plain rectangles. Measured with it on and off; reported in each chunk.
- Drawn after the game's picture and before the options menu (the menu covers it), on every
  screen, so it shows in F12 screenshots.

## Where it goes

- `src/framework/d3dapp.*`: a `FrameStats` record filled in `Render3DEnvironment` and `SwapFrame`
  (each frame's start, the time spent before the swap, the swap's wait, `WaitForGpu`'s wait with
  Low latency, the ticks run, the blend alpha, how late the latest tick ran) and a ring buffer of
  the last 512 frames. Everything is already at hand there: `m_fFPS`, `lastTickNs_`,
  `g_tickAlpha`, `vsyncInEffect_`, `vsyncOverride_`, `swapEvery_`, `SwapPaced`, `refreshHz_`.
- `src/ui/stats_overlay.*` (new): draws the panel from `FrameStats`, the config and the device.
- `src/gfx/device.*`: the graphics card, vendor and OpenGL version (`glGetString`, already
  logged at start), the antialiasing samples and texture filtering in effect (`samples_`,
  `maxAniso_`), the picture's size and the render target's (`Layout`).
- `src/core/config.*`: `[video] stats_overlay = 0` (0 off, 1 compact, 2 full).
- The options menu: a Statistics Overlay row (Options > Video, Off / Compact / Full).
- The key: **F9** cycles it anywhere (Off, Compact, Full, Off), as F11 toggles fullscreen. F9 is free; the Controls page
  would refuse it for game actions, as it already does F10 to F12.

## Chunks

Each chunk stops for the owner's test, and each is a version of its own.

### O1, the panel (text)

A panel at the left edge, below the HUD's level info (clear of the HUD, the FPS counter at the
bottom-right and the CHEATS mark at the bottom-left). **Semi-transparent** (the owner): a light
dark wash behind the text (about 45% opaque) so the game shows through and stays playable, with
the text itself opaque and given a 1-pixel dark outline so it reads over bright scenery. Two
sizes:

- **Compact**: Performance and Game timing (and, from O2, the graphs): about half the height,
  for leaving on while playing.
- **Full**: every group, Display and System too: for a screenshot to send with a report.

**Its shape follows the screen** (the owner): the panel is laid out each frame from the view's
width (`Device::ViewXRange`, as the other widescreen work), never from a fixed aspect ratio, and
always starts at the view's left edge, below the HUD's level info (which moves with HUD position
Edges):

- **4:3 and Stretch** (and anything up to about 16:10): one column, about 240 wide in the
  original's 640 x 480. Compact is about half the screen's height; Full runs down the left side.
- **Wider views** (from about 16:9, when the view has room for a second column beside the HUD's
  left part): two columns side by side, Performance and Game timing on the left, Display and
  System on the right (Full), so Full is about as short as Compact. The graphs (O2) then span both
  columns, so they show more frames.
- **Ultrawide** (21:9, 32:9): the same two columns, the graphs wider still, up to a limit (about
  500 wide), so the panel never spreads across the middle of the screen.

The font stays the menu's 8x8 at one size throughout; only the arrangement changes.

| Group | Lines |
|---|---|
| Performance | Frames a second; frame time over the last second: average, shortest, longest (ms); processor time per frame (before the swap); the swap's wait; the graphics card wait (with Low latency) |
| Game timing | Ticks in the last second (100 is right); ticks per frame (how many frames ran 0, 1, 2 or more); the latest tick's lateness (ms); smooth motion's blend (alpha) |
| Display in effect | Window, Borderless or Exclusive; the window's size and the picture's (and the render scale's target); the monitor's name and refresh rate; VSync chosen and in effect, and whether the driver seems to force it; the swap interval; the frame limit and who paces it (the driver or the limiter); Low latency; Smooth motion |
| System | The graphics card and vendor; the OpenGL version; antialiasing and texture filtering in effect; the game's version |

The F9 key, the menu row and the ini key come with it. Checks: off, screenshots byte for byte as
the build before; on, a screenshot on each screen, the figures compared with the log at a few
settings (VSync on and off, a frame limit, Smooth motion on and off).

### O1 as built (0.4.34)

- **The figures** (`src/framework/d3dapp.*`): `StatsFrameStart` on every frame drawn (its start,
  the gap since the last, the ticks it ran; with Smooth motion off, how long after its tick it
  started), `SwapFrame` (the work before the swap, the swap, `WaitForGpu` with Low latency),
  summed up once a second with the frame rate (`SummarizeStats` from `UpdateStats`) into
  `statsSec_`, which the panel shows. The 512-frame history is left for O2, which needs it.
  "CPU" is the frame's work from its start to the swap (the ticks, drawing, the overlay itself);
  "Swap wait" is the time inside `SDL_GL_SwapWindow` (with VSync, mostly waiting for the screen).
- **The panel** (`src/ui/stats_overlay.*`): groups of label and value rows, laid out from the
  view's width (`UIDraw::Left` / `Right`): one column below `kTwoColumnView` (800 in the 480-high
  space: 4:3 is 640, 16:10 768, 16:9 853), two above. Compact is Performance with Game timing
  beside or below it; Full is those two in the left column and Display with System in the right.
  Its top is `4 + 58 x the HUD size` in a level (below the TORCH row), else 62; it moves up rather
  than run off the bottom (Full in one column at HUD size 200% fits exactly).
- **Drawing it costs about nothing**: `UIDraw::BeginBatch` / `EndBatch` gather the rectangles
  and letters and draw them in two draws (`Device::DrawUIQuads`). Drawn one by one (each letter a
  draw, three times over for its outline and shadow), the panel cost 0.8 ms a frame in Compact and
  2.5 ms in Full (unlimited frames: about 2060 a second without it, 582 and 292 with it). Batched:
  Full at 2045, Compact as without it. The options menu, which still draws one by one, is byte
  for byte as 0.4.33.
- **F9** is handled beside F11 / F12 in `CD3DApplication::Run` (before the options menu, so it
  works with the menu open, under which the panel is hidden), and saves the config.
- **Checked**: with it off, the logo, title, 1-1 (still and moving) and the ending byte for byte
  as 0.4.33; on, screenshots at 4:3 (Compact, Full, HUD size 200%, the title, under the menu),
  16:9 (Compact and Full, two columns) and 32:9 (Full); F9 twice leaves `stats_overlay = 2` in
  the ini. Figures seen: 100 ticks a second everywhere; Smooth motion off, frames 9.3 to 10.7 ms
  apart and starting 0.02 ms after their tick on average (at most about 1 ms); Smooth motion on
  at 144 Hz with VSync, 31% of frames with no tick and 69% with one, as expected.

### O2, the graphs

Below the text, three strips of the last 2 to 3 seconds, one column a frame:

- **Frame time**, with lines at the refresh interval and at the frame limit's target: an even
  line is smooth, spikes are stutters.
- **Smooth motion's blend**: an even sawtooth means each picture is placed evenly; jumps show
  uneven pictures (with Smooth motion off it is flat at 1).
- **Ticks per frame**: 1 a frame at 100 frames a second; 0 and 1 in turn above it; 1 and 2 below.

### O2 as built (0.4.35)

- **No separate panel was needed** (the owner asked to check first). The graphs take about 90 in
  the 480-high space. Compact with them ends at 316 at 4:3 (374 at HUD size 200%), and Full in
  two columns at 396 (454): both fit below the HUD and above the bottom (474). Only Full in one
  column (4:3 to about 16:10) has no room under it (it would end at 524), so there the graphs get
  a small panel of their own beside it, from below the HUD's top-right panel (`sideTop`, about 84).
  F9 keeps its three states.
- **The history** (`src/framework/d3dapp.h`): `statsHist_`, the last 512 frames drawn (the gap
  since the one before, the blend it was drawn at, its ticks), filled by `StatsFrameStart` and
  `StatsFrameAlpha` whether the overlay shows or not, so the graphs are full when it is turned on.
- **The graphs** (`StatsOverlay::Graphs`): one column a frame, as many frames as the graph is
  wide, the newest at the right: 232 in one column (2.3 s at 100 frames a second, 1.6 s at 144),
  488 across two. The frame time is scaled so that the expected time is halfway up (a gold line):
  a tick (10 ms) with Smooth motion off; with it on, the frame limit, else with VSync the refresh
  times the swap interval, else none (drawing freely, scaled by the average). A frame half again
  as long as expected is red. The blend strip is flat and dim with Smooth motion off. Ticks a
  frame: dark none, green one, gold two or more.
- **Cost**: still drawn in the same two batched draws. Unlimited frames at 1280 x 960: about 2,045
  to 2,070 a second off, 2,005 to 2,040 Compact, 1,860 to 1,990 Full (about 0.01 and 0.03 ms a
  frame).
- **Checked**: off, the logo, title, 1-1 (still and moving) and the ending byte for byte as
  0.4.34; on, 4:3 Compact and Full (the side panel), 16:9 Compact and Full, 16:9 at limits 60 and
  72 (refresh over 2) and 32:9 Full. Seen: Smooth motion off, flat 10 ms and one tick a frame;
  144 Hz with VSync, 6.95 ms and ticks 0 and 1 in turn; a limit of 60, 16.66 to 16.68 ms and 1 or
  2 ticks a frame; the limit of 72 paced by the driver at 13.90 ms (two refreshes).

### O3, recording (optional, the owner to decide)

A Record row (and perhaps Shift+F9) writes the same figures for each frame, for 30 seconds, to a
CSV file beside the game, headed by the System and Display lines: a file a player can send in
with a report. Only if the owner wants it once O1 and O2 have been tried.

## Decided with the owner (2026-11-10)

- The key is **F9**, the name **"Statistics Overlay"**.
- It is **persistent**: F9 or the menu row saves the state (`stats_overlay` 0, 1 or 2), so it is
  as it was at the next start.
- It **shows in F12 screenshots**.
- F9 **cycles three states**: Off, Compact (Performance, Game timing, graphs) and Full (every
  group), after the owner saw the first mockup (the full panel covered most of the left side at
  4:3).
- The panel is **semi-transparent**, so the game can be seen and played with it on.
- The look of the second mockup (about 45% opaque, outlined text) is the one to build (the owner).
- Its **shape follows the screen**: one column at 4:3, two columns side by side from about 16:9,
  wider graphs on ultrawide screens, up to a limit.
