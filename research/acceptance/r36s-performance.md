# R36S performance: benchmark, baseline and the 60 fps gate

**Status: the benchmark and the renderer work are built; the 60 fps target is NOT met.**
The console holds 30 fps with the benchmark level and with level 1-1/3-10, although the processor
work per frame fell by more than half. The remaining limit is on the graphics/present side, not
in the game ticks or in the draw submission measured below. This file records the evidence so the
next iteration starts from numbers rather than guesses.

## Device and build

- R36S: aarch64, kernel 4.4.189, 640x480 60.02 Hz panel, Mali-G31, OpenGL ES 3.2
  (`v1.r13p0`), 895 MB RAM, battery full.
- Build: `tools/r36s/build.sh`, Debian 13 arm64 container, static SDL3 3.4.18.
- Benchmark: the generated level in `src/core/benchmark_level.cpp`, started with
  `--benchmark` or `MageBrosBench.sh`; 8 s measured after a 2 s warm-up unless noted.
- Screen mode: Window at 640x480 (the KMSDRM panel ignores a smaller `--size`), VSync on,
  Smooth motion on, all enhancements off unless a row says otherwise.
- EmulationStation was stopped for each run and started again afterwards; the game ran on
  KMSDRM. /sys GPU governor `simple_ondemand` at 400 MHz; forcing `performance` (520 MHz)
  changed nothing, so the clock is not the explanation.

## The scene

The generated level is 40 rows, 287 + 64 level quads, 63 enemies of 17 kinds, 8 coins, 6 torches,
6 fences, 10 platforms, 13 props, 19 candle rows and 2 water rows. It is valid and
`mbn_benchmark_tests` builds its geometry without `MageBros.exe`.

With the final build the benchmark reports about **226 draws (186 3D)**; level 3-10 reports about
**150 draws (106 3D)**. The benchmark is about 1.5x 3-10 in draws, in the intended 1.3 to 1.5
range (a little above on the 3D count).

## What changed in the renderer

1. **State cache** in `Device`: framebuffer, viewport, program, depth/blend/cull, matrices,
   material, ambient, light, texture stage, texture binding/filtering and the extra replay
   uniforms are sent only when they change. Invalidation covers every path outside the replay.
2. **Static geometry buffers**: models keep one `MeshBuffer`; the level's floor/underground/goal
   keep theirs. A re-upload happens only when the model or level changes. Uploaded bytes fell
   from about 1.16 MB to about 24 KB a frame on the benchmark.
3. **Opaque model materials merged** into one draw per model with a per-vertex material slot
   (`uMatMode`, material arrays); alpha models and white flashes keep the old per-material path.
   Draws fell from 366 to 226 a frame on the benchmark; the level-viewer screenshot is byte
   identical to the 0.4.47 build.
4. **Direct window rendering on GLES**: when the window covers the picture, the render scale is
   1.0, Bloom is off and Brightness is 100, the game draws into the window's own multisampled
   buffer. The display resolves the samples in the swap instead of `glBlitFramebuffer` into an
   off-screen texture. This removed a measured 30 fps lock in the empty/upload-free probe; with
   full scene content the present still costs about 25 ms a frame.

## Measurements

### Before the renderer work (same benchmark, old build)

| run | fps | CPU ms | render ms | swap ms | draws | uploaded |
|---|---:|---:|---:|---:|---:|---:|
| benchmark, MSAA 4x, aniso 8x | 11.8 | 41.1 | 32.5 | 43.4 | 366 | 1.16 MB |
| 1-1, MSAA 4x, aniso 8x | 19.9 | 15.3 | 12.7 | 35.0 | 173 | 0.40 MB |
| 3-10, MSAA 4x, aniso 8x | 15.0 | 23.4 | 20.4 | 43.1 | 327 | 0.60 MB |

The earlier A/B (MSAA 0, aniso 1, render scale 0.75) changed these by only a few frames; the
processor/renderer submission was the limit then.

### After the renderer work

| run | settings | fps | CPU ms | tick ms | render ms | swap ms | draws (3D) |
|---|---|---:|---:|---:|---:|---:|---:|
| benchmark | MSAA 4x, aniso 8x, scale 1 | 20.0 | 14.8 | 2.1 | 12.3 | 35.1 | 226 (186) |
| benchmark | MSAA 2x, aniso 1, scale 1 | 19.3 | 15.5 | 2.2 | 11.2 | 36.0 | 225 (187) |
| benchmark | MSAA 0, aniso 1, scale 1 | 20.0 | 14.4 | 2.1 | 11.9 | 35.5 | 226 (188) |
| 1-1 | MSAA 0, aniso 1, scale 1 | 30.0 | 7.2 | 0.6 | 6.2 | 26.0 | 114 (74) |
| 1-1 | MSAA 2x, aniso 1, scale 1 | 20.0 | 9.2 | 0.8 | 6.3 | 40.9 | 116 (76) |
| 3-10 | MSAA 0, aniso 1, scale 1 | 29.5 | 8.8 | 0.7 | 7.7 | 25.1 | 150 (106) |

The CPU/render side improved substantially: benchmark CPU 41 -> 15 ms, uploads 1.16 MB -> 24 KB,
draws 366 -> 226; 1-1 render 12.7 -> 6.2 ms. The frame rate did not improve beyond 30 fps because
the `swap` wait stayed at about 25 ms and the frame interval locked to two 60 Hz refreshes.

### What the frame is waiting on

Diagnostic builds (temporary environment switches, not in the source now) skipped parts of the
replay on 1-1:

| skipped | fps | CPU ms | swap ms |
|---|---:|---:|---:|
| nothing | 30.0 | 7.2 | 26.0 |
| all 3D draws | 60.0 | 3.6 | 13.0 |
| all models (`D3DFVF_VERTEX`) | 30.0 | 6.7 | 26.6 |
| all level/particle draws (`D3DFVF_LVERTEX`) | 60.0 | 4.1 | 12.4 |
| level meshes only | 30.0 | 7.2 | 26.1 |
| flat rectangles/particles only | 30.0 | 4.9 | 28.3 |
| level meshes **and** rectangles/particles | 60.0 | 4.1 | 12.5 |
| one goal pad remains (level meshes and rects skipped) | 60.0 | 4.1 | 12.5 |

**The limit is fill/overdraw in the `D3DFVF_LVERTEX` path (the level's pre-lit geometry and the
game's flat particles/rectangles), not the models, the ticks, the draw count or uploaded bytes.**
The path is not a fixed driver stall: its cost scales with the render target.

| 1-1, MSAA 0 | full 640x480 | 0.75 (480x360) | 0.5 (320x240) |
|---|---:|---:|---:|
| everything | 30.0 | 30.0 | 54.2 |
| level meshes only | 37.9 | 59.7 | 60.0 |
| flat rects/particles only | 30.0 | 55.0 | 59.7 |
| no 3D (HUD/present only) | 60.0 | 60.0 | 60.0 |

At full resolution the level alone costs about 10 ms of present wait and the flat
rects/particles alone about 14 ms; together they miss the 16.7 ms deadline.

### Why the result is 30 or 60, never 45

The KMSDRM panel flips at 60 Hz and SDL's swap waits for a page flip; there is no
show-immediately path. A frame that is not ready before the next refresh waits a whole
16.7 ms, so the only stable presentation rates are 60, 30, 20... (VSync on/off made no
difference: `vsync = 0` and `vsync = 1` measured identically on the console).

At 0.5, the frame-time samples split cleanly: **90.6 % of frames under 20 ms (60 fps),
9.4 % at about 33 ms (30 fps)**, average 54.9. Full resolution: **100 % of frames at
33 ms**, average 30.0. Stable 60 therefore means removing the 9.4 % misses at 0.5 (CPU
spikes / dynamic uploads), or halving fill at 640x480; it is not a matter of raising an
average. At 0.75 the two
still just miss it together; at 0.5 each alone reaches 60 and together they average 54.2 with
p95 still at 33 ms (some frames still miss). Enabling backface culling for the level, skipping
textures, vertex colours or attribute 2, or disabling depth test/write changed nothing. MSAA
0/2/4 all lock to 30 at full resolution, so MSAA is not the differentiator; the fragment/fill
work is. The 60 fps target at 640x480 is therefore a **fill/overdraw problem**, not a state or
submission problem.

## Target verdict

The specification's R36S target (average >= 59 fps, 1% low >= 55 fps, no sustained 60/30 at the
current defaults MSAA 4x, anisotropy 8x, render scale 1.0) is **not met**:

- the benchmark holds 20 fps with MSAA 4x and 20 fps with MSAA 0;
- level 1-1 holds 30 fps with MSAA 0 and 20 fps with MSAA 2x/4x;
- level 3-10 holds 29.5 fps with MSAA 0.

The failure is recorded here as required by the `render-performance` spec (a failed target is
named, not hidden). No default was silently lowered; the console's configuration was left at
its pre-change performance values after the runs (MSAA 0, anisotropy 1, Smooth motion on).

## Next step (needs the owner's decision)

1. **Accept a first-milestone performance profile**: MSAA 0, anisotropy 1 and render scale 0.5
   is the closest measured point (54.2 fps average, p95 33 ms; level-only and rects-only each
   reach 60). Reducing CPU work further (batch the 2D/dynamic quads, shave the remaining 7 ms)
   could close the last few frames. This keeps the 640x480 picture but renders it at 320x240 and
   upscales it, so it is a quality decision for the owner, not a silent default.
2. **Reduce the fill/overdraw at full resolution**: render the flat particles/shadows into a
   lower-resolution layer and composite, cut the level's overdraw (the floor, walls and
   underground all cover pixels) or find a cheaper level shader. These change the picture or
   add a compositing pass and belong with the next quality iteration.
3. **Reduce the benchmark's density**; 1-1 alone already locks to 30 at full resolution, so this
   is not a fix by itself, only a way to make the stress scene match a chosen target.

The benchmark, the report and the renderer changes stay useful for all three.
