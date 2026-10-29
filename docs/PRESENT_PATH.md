# The present path (DXGI flip in a window and borderless)

How a finished picture reaches the screen, why the port is getting a second way of doing it,
and how it is being built, chunk by chunk. Built on the `dxgi-present` branch, from 0.4.22.
Stage 7, after smooth motion and the graphics review ([ROADMAP.md](ROADMAP.md)).

## Status: paused (the owner, 2026-10-29)

The owner put this branch on hold after P2 and went back to 0.4.21 on `main`. The released game
recommends that Windows players with NVIDIA cards set **NVIDIA Control Panel > Manage 3D settings
> Program settings > MageBrosNative.exe > Vulkan/OpenGL present method: Prefer layered on DXGI
swapchain**, which gives a window and borderless the same smoothness (the owner's test that started
this work). The branch is pushed to GitHub, kept apart from `main`; pushing it published nothing.

Where it stands:

- **P1 (0.4.22)**: done and built here; the owner began testing it with the NVIDIA setting back on
  Auto, and no result was reported before the pause.
- **P2**: built, measured and documented below ("P2 as built"), committed with the version still
  0.4.22 in `CMakeLists.txt`; its CHANGELOG notes are under `## [Unreleased]`. Never packaged.
- **P3 to P5**: not started.

To pick it up again:

1. `git checkout dxgi-present`, and bring `main`'s later changes in (merge `main` into it). Keep the
   branch's versions above whatever `main` has reached by then.
2. Ask the owner to test P1 and P2 together (`present_mode = dxgi`, the NVIDIA setting on Auto, a
   window and borderless, Smooth motion on, limits 60 and Unlimited), then run `make_release.bat`
   for the next version, turning `[Unreleased]` into its section.
3. Carry on with P3 (Alt+Tab with the exact borderless first: it is the one P2 change not yet tried
   in play), P4 and P5 as planned in the table below.
4. AMD: still untried. The path needs `WGL_NV_DX_interop2`, which AMD's drivers list; AMD has no
   setting like NVIDIA's, so for AMD players this path is the only cure.

## Why (the owner's tests, 2026-10-28 and 29)

With Smooth motion on and a frame limit below the monitor's refresh rate (60 on the owner's
144 Hz monitors), the game felt sluggish and choppy in a window and in borderless, and smooth in
exclusive fullscreen. The owner's monitors have G-Sync on for windowed and fullscreen mode.

- **The port's timing was not the cause.** Smooth motion's chunk D log ([SMOOTH_MOTION.md](SMOOTH_MOTION.md),
  "Frame pacing, measured first") showed frames at a 60 limit 15.8 to 17.5 ms apart, each
  showing the game within 0.04 ms of where it should be. The review of `d3dapp.cpp` (the tick
  clock, the blend, the limiter, VSync) found nothing wrong.
- **The cause is how OpenGL presents in a window.** `SDL_GL_SwapWindow` (SwapBuffers) in a window
  or borderless is, by default, copied into the desktop by the compositor (DWM). That adds a
  frame of delay and its own pacing, and G-Sync / FreeSync can't follow such a window, so the
  monitor stays at a fixed 144 Hz, where a 60 limit shows each picture for 3, 2, 2, 3, 2...
  refreshes. Exclusive fullscreen skips the compositor, so G-Sync engaged there.
- **The proof**: NVIDIA's "Vulkan/OpenGL present method: Prefer layered on DXGI swapchain" for
  `MageBrosNative.exe` made a window and borderless smooth. That setting presents OpenGL through
  a DXGI flip swapchain, as Direct3D 11 / 12 games present.
- **The owner's aim**: players with an NVIDIA or AMD card play smoothly with or without G-Sync /
  FreeSync, at any refresh rate and frame limit, without changing driver settings. AMD has no such
  setting at all.

## What it does (as built so far)

`src/gfx/dxgi_present.{h,cpp}` (Windows; elsewhere it is a stub that always says no):

- A Direct3D 11 device on the GPU OpenGL runs on (picked by vendor from `GL_VENDOR`: a system with
  integrated graphics has two adapters, and the default one may be the other).
- A flip-model swapchain (`DXGI_SWAP_EFFECT_FLIP_DISCARD`, 2 buffers, RGBA8) on the game's own
  window, with `DXGI_MWA_NO_ALT_ENTER` so Alt+Enter stays the game's. The window keeps its OpenGL
  pixel format and context; the swapchain was accepted on it (the risk the spike was for), so no
  helper window is needed. `ALLOW_TEARING` is set where the system has it, and the frame-latency
  waitable object holds the queue to one frame (P2).
- A D3D11 texture the size of the window, shared with OpenGL through `WGL_NV_DX_interop2`
  (NVIDIA, AMD and Intel drivers have it): registered as a `GL_TEXTURE_2D` (the renderbuffer form
  gave an incomplete framebuffer on the owner's NVIDIA) and attached to a framebuffer.
- `Device::Present` draws its last pass (the scene, scaled into the window with the letterbox and
  Brightness) into that framebuffer instead of the window's own, upside down, since the shared
  texture's rows run top to bottom (`v` swapped, viewport y counted from the top). Everything
  before that pass, and the screenshots (read from `resolveFbo_`), are unchanged.
- `SwapFrame` (`d3dapp.cpp`) replaces `SDL_GL_SwapWindow`: unlock the texture, `CopyResource` to
  the back buffer, `Present` with the VSync in effect (P2: Off `Present(0, ALLOW_TEARING)`, On
  `Present(1)`, Adaptive `Present(1)` unless the frame comes more than 1.5 refreshes after the
  last, then `Present(0, ALLOW_TEARING)`).
- Borderless covers the monitor exactly on this path (P2; OpenGL's path keeps its extra row).
- The run loop waits on the waitable object once per presented frame before taking the frame's
  time, and, on a fixed refresh grid with VSync and a frame limit, starts each frame just after a
  refresh (P2, below).
- A window size change (dragging, maximizing, F11, borderless's extra row) is seen in
  `BeginFrame`, which calls `ResizeBuffers` and remakes the shared texture.
- `present_mode = dxgi` in the ini turns it on (default `legacy`). If any step fails, the game
  logs why ("Present: DXGI not used (...)") and keeps OpenGL's own SwapBuffers.

## The chunks

| Chunk | What it covers | Status |
|---|---|---|
| **P1: Spike** | The swapchain, the shared texture and the last pass drawn into it; VSync always on; `present_mode = dxgi` in the ini only (default legacy) | Done (0.4.22) |
| P2: VSync and latency | VSync Off / On / Adaptive on DXGI; the frame-latency waitable object (one frame queued); **frames aligned to the refresh grid** (planned as display-timed Smooth motion, changed while building: see "P2 as built"); borderless covering the monitor exactly (moved here from P3: G-Sync needs it); the `MBN_PRESENT_TRACE` frame log | Built (to be 0.4.23); waiting for the owner's P1 test before the release build |
| P3: The window's life | Resizing, maximizing, monitors with other scales, minimizing, Alt+Tab with the exact borderless, Alt+Enter / F11 to and from exclusive fullscreen (exclusive to stay on OpenGL's path, or not: to decide), render scale and antialiasing changes, screenshots | Next |
| P4: Fallback and the option | Every failure falls back at start-up and says why; a test hook to force it; Options > Video "Present: Modern / Legacy" with its help line; the Frame limit row offering the refresh rate's divisors, with a help line on limits and fixed-refresh screens; **the owner decides the default** | |
| P5: Checks and docs | Screenshots and screen captures the same on both paths; the frame-pacing log on the new path; a test matrix: NVIDIA (and AMD if possible), G-Sync / FreeSync on and off, 144 and 60 Hz, limits Unlimited / 141 / 100 / 72 / 60, window / borderless / exclusive; the owner's play test with the NVIDIA setting back on Auto; then the merge into `main` | |

## P1 as built (0.4.22)

- Checked on the owner's machine (RTX 3090 Ti, driver 616.92, 1280 x 960 window on the 144 Hz
  monitor): the log shows "Present: Direct3D 11 on NVIDIA GeForce RTX 3090 Ti" and "Present: DXGI
  flip swapchain 1280x960 (tearing supported)". Screen captures of the window (not the game's own
  screenshots, which are read before the last pass) show 1-1 the right way up, HUD included.
- Borderless at 3840 x 2161 (the extra row) with Aspect Widescreen: the swapchain resized from the
  window's 1280 x 960 and the picture filled the monitor, the right way up.
- F11 to borderless and back to the window: no errors, the picture right in the window afterwards.
- Smooth motion on, VSync on, no limit: 144 frames a second on both paths (the FPS counter).
- `present_mode = legacy` runs exactly as 0.4.21 (the last pass draws to framebuffer 0 as before).
- Not yet: VSync Off or Adaptive on DXGI (Present always waits for the refresh in P1), the queue
  depth (DXGI's default of up to 3 frames), the frame limiter's interplay, exclusive fullscreen on
  this path (it runs, but its choice is P3's), AMD and Intel.

## P2 as built (to be 0.4.23)

**Measuring.** `MBN_PRESENT_TRACE=<file>` (an environment variable, a test hook) writes a line per
present: its number and `QueryPerformanceCounter` time, and the last present the frame statistics
say reached the screen, with its time and refresh count. From that, for each frame shown: the time
from present to screen, and how far its shown interval strays from its present interval, which is
how far Smooth motion's picture (placed for when it was drawn) is from where it is seen. The
statistics only name the latest picture shown, so at 60 on 144 Hz about 60% of frames get a time;
the spread over those is the measure. All in 1-1 with the mage running, Smooth motion on, on the
owner's machine (G-Sync on for windowed and fullscreen mode).

**What the measurements showed, and what was built from them:**

1. **A window is always composed on a fixed grid**, through DXGI too: the statistics show every
   picture on a 6.951 ms grid (143.86 Hz) and about 19.7 ms from present to screen; G-Sync does
   not follow a window here. With a limit of 60, a picture drawn anywhere between two refreshes
   shows at the next one, so the time from present to screen varied by 2.1 ms (sd) and pictures
   strayed about 3.3 ms (sd) from where they belonged: the choppiness the owner felt.
2. **Display-timed Smooth motion, as planned, can't work**: Smooth motion only keeps the last two
   ticks, so a picture can only show the game between them, at most 10 ms back. Placing each
   picture for when it reaches the screen needs a constant delay longer than a refresh plus a
   tick, which would need a third tick kept by every object. **Built instead: frames start just
   after a refresh.** `NsUntilFrameStart` gives the wait to 0.2 ms past the next refresh of the
   measured grid, unless the frame is already within 1 ms of one; the limiter's schedule (due
   times) is unchanged, so the average rate is still the limit. Then every frame takes the same
   time from being drawn to being shown, and Smooth motion's pictures are right as they are.
3. **The grid is judged from the statistics, not from the monitor's rate**: in a window on the
   60 Hz receiver the pictures came on the primary monitor's 144 Hz grid (Windows 10 composes on
   the primary's refreshes). `UpdateGrid` keeps a second of refreshes and calls it a grid when
   every one lies within 0.3 ms of a lattice of the measured period. With G-Sync following the
   frames they don't (the refresh count steps unevenly). Not judged with VSync off. Verdict
   changes are logged (the first 20).
4. **Borderless needs to cover the monitor exactly for G-Sync**: with OpenGL's extra row it was
   composed on the grid like a window; exactly covering, Windows flips the swapchain straight to
   the screen and G-Sync followed every frame. On this path OpenGL never presents, so the reason
   for the row (drivers turning an OpenGL window covering the monitor into exclusive fullscreen)
   doesn't apply. Alt+Tab with it is for the owner to try (P3).

| Case (limit, VSync on) | Grid seen | Present to screen | Pictures stray (sd) |
|---|---|---|---|
| Window, 144 Hz monitor, 60, not aligned | yes | 19.0 ms, sd 2.15 | 3.27 ms |
| Window, 144 Hz monitor, 60, aligned | yes | 19.7 ms, sd 0.22 | 0.31 ms |
| Window, 60 Hz receiver, 40, aligned | yes (144 Hz, the primary's) | 19.7 ms, sd 0.22 | 0.32 ms |
| Borderless with the extra row, 60 | yes | 19.7 ms, sd 0.48 | 1.06 ms |
| Borderless exact, 60 | no: G-Sync, every picture 16.67 ms after the last | sd 0.45 | 0.76 ms |
| Borderless exact, unlimited | yes (G-Sync at its ceiling, 143.7) | 6.0 ms | 0.39 ms |

Also checked: VSync Off uncapped (795 frames a second in a window, 668 borderless), Adaptive and
On at 144 in a window, Smooth motion off at 100 a second as the original, the options menu drawn
right in borderless, all 4 tests. The present-to-screen times are as the statistics report them
(borderless's negative figure, -6.7 ms, is how Windows stamps a flipped picture; only the spread
counts). Not measured: the legacy path (it has no statistics), AMD, a monitor without G-Sync in
borderless (the grid alignment applies there as in a window).

**For the owner to test 0.4.22**: put the NVIDIA "Vulkan/OpenGL present method" back to **Auto**
for `MageBrosNative.exe`, set `present_mode = dxgi` in `magebros_native.ini`, and play in a window
and in borderless with Smooth motion on at Frame limit 60 and Unlimited: is it as smooth as with
the NVIDIA setting, and does G-Sync engage (the NVIDIA G-Sync indicator, or the monitor's own
refresh readout)? `present_mode = legacy` gives the old path to compare.
