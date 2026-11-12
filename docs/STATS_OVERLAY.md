# The Statistics Overlay (O1 built as 0.4.34, O2 as 0.4.35; the graphics rows from 0.4.39; O3, the debug report, planned)

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
| Enhancements | (Full; added by the graphics enhancements, 0.4.46, on `graphics-enhancements`) each enhancement as set, dimmed where it doesn't run: see "The graphics enhancements' rows" below |

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

### The graphics enhancements' rows (0.4.39 to 0.4.46, on `graphics-enhancements`)

Added to the PERFORMANCE group of the **Full** page by the graphics enhancements
(`docs/GRAPHICS_ENHANCEMENTS.md`), each from the graphics card's own timer queries
(`Device::GpuTimer`: `GL_TIME_ELAPSED`, read frames later without waiting) or the renderer's
counts, summed over the last second (`CD3DApplication::SummarizeStats`):

| Row | What | Shown |
|---|---|---|
| GPU 3D | the card's time for a level's 3D a frame (the recorded draws; the card's timeline, waiting for the processor included, so an upper bound) | always in Full; "- (no level)" outside one, "- (off)" with `MBN_NO_RECORD` |
| 3D draws | the level's 3D draws in the last frame that had any | with GPU 3D |
| GPU AO | ambient occlusion's three passes a frame | Ambient occlusion on |
| GPU shadows | the dynamic shadows' map a frame (the lookups are in GPU 3D) | Dynamic shadows on |
| GPU bloom | the bloom's resolve, blur and adding a frame | Bloom on |
| Lights | the dynamic lights in the last frame that had any (16 at most) | Dynamic lights on |

They cost nothing measurable (a few queries a frame). Compact is unchanged.

**The enhancements in effect** (G7, 0.4.46, 2026-11-12): a group of its own,
ENHANCEMENTS, on the Full page, with a row for each: Ambient occl. (On / Off), Shadows (the
quality and the strength, "Medium, 80%"), Lights (On and the strength), Ambient light (its
percentage, or "- (lights off)") and Bloom (Off / Low / High), all read from the config as it is.
An option that is off is dimmed. Where nothing runs them, every row is dimmed under a line
saying why: "Not running (levels only)" on any screen but a level (no level's 3D in the last
second), "Not running (recorder off)" with `MBN_NO_RECORD`. In two columns (from about 16:9) the
group goes under SYSTEM; in one column (4:3 to 16:10), where the column has no room left, it
heads the graphs' side panel (`StatsOverlay::Render`'s `extra` groups). Checked by screenshots at
4:3 (a level and the logo), 4:3 in a 16:9 window and 16:9 widescreen; with the overlay off
nothing is drawn, as before. The heights move: Full in two columns now ends at about 444 at HUD
size 100% (was 396; at larger HUD sizes it moves up over the HUD's level info, as it was built
to), and the side panel at 4:3 grows from about 106 high to about 180.

### O3, the debug report (planned, not built; for later, on the owner's word)

The purpose (the owner, 2026-11-10): **players submit a report file for debugging**, so that a
problem on hardware the owner doesn't have (AMD or Intel graphics, a 60 Hz or 32:9 screen, a
weak computer) can be read exactly instead of described. A screenshot of the overlay shows one
moment and its graphs the last few seconds; the report covers a stretch of play, with everything
needed to read it in one file. One chunk, a version of its own.

**What the player does.** Press **Shift+F9** (or Options > Video > **Record debug report**) right
after a stutter or slowdown, and keep playing. For 30 seconds a small "RECORDING 23s" shows in a
corner (drawn like the CHEATS mark, whether the overlay is on or not); then "Report saved:
reports\MageBrosNative-report-<date>-<time>.txt" for a few seconds. Pressing again while it
records stops it early and saves what it has.

**What the file holds** (one plain text file, readable by anyone, that can be attached to a
GitHub issue):

1. **Header**: the game's version and the date; Windows' version (`SDL_GetPlatform`, and the build
   number), the processor's core count and the memory (`SDL_GetNumLogicalCPUCores`,
   `SDL_GetSystemRAM`); the graphics card, vendor and OpenGL version; every monitor (name,
   resolution, refresh rate, scale) and which one the game is on.
2. **The settings in effect**: the overlay's Full Display and System groups (display mode, window
   and picture sizes, render scale, VSync chosen and in use and the driver verdict, the swap
   interval, the frame limit and who paces it, Low latency, Smooth motion, antialiasing, texture
   filtering), the menu font (Classic or a player's), and the ini's video and graphics values as
   written.
3. **A summary of the recording**: frames a second; frame time average, shortest, longest and the
   99th and 99.9th percentiles ("1% and 0.1% lows"); how many frames were stutters (half again as
   long as expected, as the overlay's red bars); ticks a second (100 is right); tick lateness with
   Smooth motion off; the screen it was recorded on (a level, the title, ...).
4. **Every frame**, as a CSV block a spreadsheet opens: time since the recording began, frame time,
   processor time, the swap's wait, the card's wait (Low latency), ticks run, the blend, tick
   lateness. It starts with the frames **before** the key was pressed (the 512 frames the overlay
   already keeps, so the stutter just seen is in the file), then the 30 seconds after.
5. **The end of the log** (`magebros_native.log`, its last 100 lines or so: the monitors, VSync,
   the menu font, warnings).

**Privacy.** Nothing in the report may identify the player. Folder paths in the log can contain
the Windows user name (`C:\Users\<name>\...`, for the save folder): every path is written with the
user folder replaced by `%USERPROFILE%` (and the game's folder by `<game>`), and the report's
header says the file holds no personal data. No network: the game never sends anything; the
player chooses to attach the file.

**Where it goes in the code.** `src/framework/d3dapp.*`: while recording, `StatsFrameStart` and
`SwapFrame` also append each frame's record to a buffer (about 40 bytes a frame: 30 seconds at
2,000 frames a second is 2.4 MB, kept in memory); at the end the file is written in one go
(`SDL_IOFromFile`), off the frame loop if it takes noticeable time (a few milliseconds at most).
The key beside F9 in `Run`; the menu row in `options_menu.cpp`; the corner message drawn beside
`DrawStatsOverlay`. The `reports` folder is next to the program (beside the ini and the log),
made on first use.

**Rules, as the overlay's**: recording only reads the frame loop; it never changes the game, the
ticks or when a frame starts. Not recording, screenshots stay byte for byte as before.

**Checks when built**: the figures in a report against the overlay's at the same moment; a
recording at 100, 144 and an unlimited frame rate (the file's size, and no hitch in play when it
is written); the report opened in a spreadsheet; a search of the file for the user name (none);
not recording, screenshots as the build before.

**For the README when built** (a short player section): "If the game stutters or runs slowly,
press Shift+F9 right after it happens and keep playing for 30 seconds. The report is saved in
the `reports` folder next to the game; attach it to your report on GitHub Issues." The owner may
want a GitHub issue template that asks for it.

**To decide when building**: the length (30 seconds proposed); Shift+F9 or another key; whether to
add a GitHub issue template.

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
