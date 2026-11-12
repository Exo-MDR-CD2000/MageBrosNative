# Graphics enhancements (G1 to G4 built; the rest planned)

**Status: planned 2026-11-10, on the `graphics-enhancements` branch (made from `main` at
0.4.38). G1, G2 and G3 are built (2026-11-11, each with an "as built" section below) and were
built together as 0.4.39, the first test build (the owner's choice); AO's Low was removed after
the owner's test, as 0.4.40; the later chunks take the next numbers from 0.4.41. G4 (dynamic shadows, G4a and G4b together) is built too (2026-11-11,
"G4 as built"), as 0.4.41; the later chunks take the next numbers from 0.4.42. Nothing else is.** Each chunk below is a version of its own and stops for the
owner's test; the owner says when to start each one. Written from reading the code at 0.4.38;
nothing in it has been measured yet, and every cost below is an estimate until the chunk that
measures it.

## In short

- **Out:** Per-pixel lighting and Soft shadows. Both are off by default and add little: the
  original lights its models with one directional light, no specular, on low-poly meshes, so
  lighting per pixel has almost nothing to work with (and it loses the original's coin-brightness
  quirk, below); Soft shadows only blurs the edge of the blob texture.
- **In, all opt-in, off by default, levels only at first:**
  1. **Ambient occlusion**: soft darkening where floors meet walls, under ledges, around and
     under objects. The tile-built levels gain the most depth from it for the least risk.
  2. **Dynamic shadows**: real shadows from the game's own light (the original's direction,
     (-1, -1, 0.5)): mages, enemies, bosses, pillars, trees, platforms and the level's own walls
     cast onto the floor, over cliff edges and onto each other.
  3. **Dynamic lights**: lit torches, wall candles, the mages' shots, flames, fireballs and
     explosions light the floor, walls and models around them, flickering as their flames do.
  4. **Bloom**: a glow around the game's own glowing things (fire, shots, warp rings, sparkles),
     and only those.
- **The game is untouched.** No game logic changes, nothing in `src/game` asks for anything new,
  and with every new option off every picture is byte for byte as 0.4.38. All the work is in the
  port's renderer (`src/gfx/device.cpp`) and a few files beside it.

## The rules this work keeps

These are the project's rules (`CLAUDE.md`) applied to this work:

1. **Off by default, and byte for byte as before when off.** Every new setting defaults to Off;
   with all of them off, the screenshot suite must match the build before the chunk exactly.
   Removing Per-pixel lighting and Soft shadows changes nothing at default either (both default
   to off).
2. **No game logic changes.** `src/game` keeps its tick order, constants and draw calls. The
   renderer works out what to light and shadow from what the game already draws (see "How it
   fits the renderer").
3. **The draw order stays.** See-through things blend over what is already drawn, so the order
   is part of the picture. Extra passes (depth, shadows) are added *before* the game's own draws;
   the game's own draws still run in the game's order.
4. **The frame rate never changes the game.** Everything here is drawing only. With Smooth motion
   on, it uses the same blended positions the game is drawn at (`--auto alpha=1` must still give
   the same pictures as Smooth motion off, with each new option on or off).
5. **Any window shape and size.** Nothing assumes an aspect ratio; shadows and the ambient
   occlusion follow the view as laid out each frame (widescreen, Field of view, Render scale).
6. **One chunk at a time**, each a version (the branch takes the next numbers: 0.4.39 on; `main`
   stays off them until the branch merges), each built, measured, screenshot-checked and handed
   to the owner before the next. Ask the owner before raising the version and building
   (`make_release.bat patch`), and keep a build per chunk in `build\fix-builds\` named by what it
   holds, as on `smooth-motion-rework`.

## What the original's picture is made of (what we have to work with)

Read for this plan; the details are in `docs/REVERSE_ENGINEERING.md` ("Light and lens", "Map
geometry", "Particles and shadows").

| Part | How it is drawn | What it means here |
|---|---|---|
| **The light** | One white directional light, direction (-1, -1, 0.5), full strength, set once (`XGameScene::SetDeviceState`); ambient `0x4c4c4c` (0.3). Specular off | A strong, slanted light and a dim ambient: shadows and added light will show clearly. The direction slants down, towards -x and along +z, so shadows fall to one side and forward, about 1.1 units sideways per unit of height |
| **The level** | A few large merged meshes, one per texture (`XLevelView::DrawMesh`: floor, underground, goal pads), `D3DLVERTEX`: **pre-lit** (the shading is baked into vertex colours; lighting off), textured, point-sampled. Built by the port's own `MapGeometry` from the original's map data | No normals in the vertices. For lighting and shadows the normal can be worked out per pixel from the surface itself (screen-space derivatives); the faces are flat, so this is exact |
| **Models** | `CD3DFileObject::Render`: lit per vertex (`D3DFVF_VERTEX`), **untextured** (material colours only; textures only in mods), low-poly. Solid materials first, see-through materials (`hasAlpha`) in a second, blended pass. Normals are not renormalized (D3D7's default), so scaled models (coins at 0.35 and 0.6) light brighter | They live or die by their light and dark sides: shadows and AO add a lot. The vertex lighting must stay exactly as it is when the new options are off |
| **The blob shadow** | Texture `0xa3`, a flat rectangle multiplied onto the floor (`ALPHA_SHADOW`: `ZERO, SRCCOLOR`), straight below each mage, coin and some objects | It shows where a jumping mage will land: a gameplay aid. It stays as it is, with Dynamic shadows on too (the owner, 2026-11-10) |
| **Particles and shots** | Flat rectangles (`JObject3DRect`) with `ALPHA_PARTICLE` (`SRCALPHA, DESTALPHA`, which with the destination alpha cleared to 1 is **additive**) and no depth writing: torch flames (`CreateTorchFire2`, every 12 ticks per lit torch), the mages' shots (`fireSprite*`), flashes, explosions, warp rings and photons, coin sparkles, the water's glints (`water2_`) | The game's own "light". Additive rectangles are what Bloom glows, and the ones that are flames or shots become Dynamic lights |
| **Water** | Two rectangles: `water1_` see-through (`SRCALPHA, INVSRCALPHA`), `water2_` additive | Not a caster, not lit; it keeps its look |
| **2D** | The HUD, the shop, fades, menus: `Draw2DQuad` / `DrawUIQuad`, after the level's 3D | Untouched; the new effects end before the 2D begins (`SetGameplayView(false)`) |

## How it fits the renderer: record the level's 3D, then draw it

Today each `DrawIndexedPrimitive` is sent to OpenGL as it comes. Shadows need a picture of the
scene from the light *before* the scene is drawn, and ambient occlusion needs the scene's depth
before the colours. Two ways were considered:

- **Run the game's `Render` twice** (once for depth, once for real). Rejected: drawing code in
  `src/game` is not guaranteed to be free of side effects (counters, `textureOffset` in the
  ending, sprite vertices written in place), and it would put the extra pass inside the game's
  code.
- **Use last frame's shadows.** Rejected: shadows would trail a frame behind fast movers, and the
  first frame after a cut would be wrong.
- **Chosen: record, then draw.** While `SetGameplayView(true)` is on (a level's 3D, every frame,
  already marked by `XGameScene::Render`), `Device` **records** each draw instead of sending it:
  a snapshot of the emulated state it needs (`rs_`, `tss_`, the three transforms, the material,
  the texture, the vertex format) and a copy of its vertices and indices into one frame-sized
  buffer. At `SetGameplayView(false)` it draws the recording: first the extra passes the
  settings ask for, then the recorded draws **in their original order**, exactly as they would
  have been sent. `src/game` notices nothing.

What the recording makes possible, beyond these effects:

- **One upload a frame for the extra passes** (depth, shadows): all the frame's vertices in one
  `glBufferData`, drawn with a base vertex. **Not for the game's own draws**: G2 found that
  drawing them from one shared upload instead of one each changes two pixels of 1-6 by one shade
  on NVIDIA, every time ("G2 as built"), so the main pass keeps one upload a draw, as before, and
  stays byte for byte. The extra passes only make depth, where that hair's difference can't show.
- **Classifying draws** from the state alone, with nothing asked of the game:

  | Kind | Recognised by | Casts a shadow | Receives shadow and AO | Lit by Dynamic lights | Glows (Bloom) |
  |---|---|---|---|---|---|
  | Level geometry | `D3DFVF_LVERTEX`, lighting off, blending off | Yes | Yes | Yes | No |
  | Solid model part | `D3DFVF_VERTEX`, lighting on, blending off | Yes | Yes | Yes | No |
  | See-through model part | `D3DFVF_VERTEX`, blending `SRCALPHA, INVSRCALPHA` | No (decision; could cast a lighter shadow later) | No | Yes | No |
  | Blob shadow | blending `ZERO, SRCCOLOR` | No | No | No | No |
  | Additive rectangle | blending `SRCALPHA, DESTALPHA` (or `ONE, ONE`) | No | No | No | **Yes** |
  | See-through rectangle (water) | `LVERTEX`, blending `SRCALPHA, INVSRCALPHA` | No | No | No | No |

- **The white flash** of a hit mage (`CD3DFileObject::Render(..., white)`) is a material: it
  replays as recorded.

Outside a level's 3D (the title, level select, the ending, the shop's panel, the HUD), nothing is
recorded and nothing changes. Whether the ending's corridor and the title's mages should get the
effects later is a decision for the owner (below).

**The trap** (the same one as `RENDERER_OPTIMIZATION.md`'s R1): anything that reads back state
mid-level (`GetRenderState`, `GetTransform`, `GetTexture`, which the game uses to save and
restore around its rectangles) must still get the value last *set*, not last drawn. The setters
keep storing into `rs_` etc. as now; only the sending changes. Covered by the byte-for-byte runs.

## The new frame, in order (with every option on)

Inside a level's 3D, at `SetGameplayView(false)`:

1. **Upload** the recorded vertices and indices once, for the extra passes (2 and 4).
2. **Depth pre-pass** (only for AO): the solid draws (level and solid model parts), depth only,
   from the camera, into a single-sample depth texture at the render target's size (or half, for
   AO Low).
3. **Ambient occlusion**: from that depth, a texture of how open each pixel is (0 to 1), blurred
   by depth so it doesn't bleed across edges.
4. **Shadow map**: the casters (level and solid model parts), depth only, from the light, into a
   depth texture.
5. **Lights**: the frame's light list made from the recorded additive rectangles (see chunk G5),
   sent as one small uniform array.
6. **The recorded draws, in their original order**, into the scene target (MSAA as set). The one
   shader gets three new inputs, each used only by the draws that receive it: the shadow map, the
   AO texture (looked up at the pixel's screen position), the light list. Additive draws also
   write their colour into a second colour target, the **glow target**, for Bloom.
7. **Bloom**: the glow target shrunk and blurred in a few steps, then added onto the scene.
8. Back to the game: the shop's panel, the HUD and the fade are drawn as now.

With an option off, its steps are skipped entirely; with all off, steps 2 to 7 are skipped, and
step 6 sends exactly what is sent today.

## How each draw is shaded (the new shader paths)

Today a lit model's colour per vertex is `emissive + ambient x matAmbient + light x matDiffuse x
max(N . L, 0)`, all clamped together. To shadow only the direct light, the vertex shader passes
the two parts separately (**ambient part** and **direct part**, two new varyings) and the pixel
shader adds them: `clamp(ambient part + direct part x shadow)`. With `shadow = 1` this is the same
sum as today, but the clamp moves after an interpolation: to keep byte-for-byte pictures with
Dynamic shadows off, the old path stays as it is, and the split path is used only with Dynamic
shadows on. (The vertex normals stay un-renormalized in both paths: the coins keep their glow.)

| Draw | With the new options |
|---|---|
| **Level** (pre-lit) | `baked colour x texture`, then x `mix(1, shadowDarkness, inShadow)` where the face turns towards the light (where it faces away, its baked colour is already the dark side), then x the AO term, then `+ texture x sum of lights (colour x falloff x max(N . toLight, 0))` with N from the surface (`normalize(cross(dFdx(pos), dFdy(pos)))`) |
| **Solid model part** | `clamp(ambient part x AO + direct part x shadow) + material diffuse x sum of lights (colour x falloff x max(N . toLight, 0))` |
| **See-through model part** | as today, `+` the lights' term (so a lit torch also tints the genie's steam, the ghosts) |
| **Everything else** | as today |

`shadowDarkness` is not a light value the original has (its level is pre-lit), so it comes from
the Shadow strength setting (default to be tuned by eye: about 0.55, close to what the blob
shadow's centre does to the floor).

## The chunks

| Chunk | Version | What | Risk |
|---|---|---|---|
| G1 | 0.4.39 | Per-pixel lighting and Soft shadows removed (**built**, 2026-11-11) | Low |
| G2 | 0.4.40 | The recorder and the GPU timers: no visible change (**built**, 2026-11-11) | Medium (every level picture goes through it) |
| G3 | 0.4.41 | Ambient occlusion (**built**, 2026-11-11) | Low to medium |
| G4a | 0.4.42 | Dynamic shadows: the shadow map, models and level casting and receiving (**built** with G4b, 2026-11-11) | Medium |
| G4b | 0.4.43 | Dynamic shadows: soft edges, steadiness, quality levels (**built** with G4a) | Low |
| G5a | 0.4.44 | Dynamic lights: finding the lights | Low |
| G5b | 0.4.45 | Dynamic lights: lighting the level and the models | Medium |
| G6 | 0.4.46 | Bloom | Low |
| G7 | 0.4.47 | The page, presets, the overlay, final checks, docs; merge | Low |

The versions are a guide: a chunk that splits takes the next number, as before.

### G1. Remove Per-pixel lighting and Soft shadows (0.4.39)

What goes:

- `src/core/config.h`: `perPixelLighting`, `enhancedShadows`.
- `src/core/config.cpp`: the two keys are no longer written; when read from an older ini they are
  ignored silently (as `force_soft_synth` is: listed as removed, so they aren't reported as
  unknown); the ini notes for them removed. No `config_version` bump is needed (defaults
  unchanged; the next `Save` drops the lines).
- `src/ui/options_menu.cpp`: the two rows of `GraphicsPage`, and the revert check at line 153
  (`old.enhancedShadows`).
- `src/gfx/device.cpp`: `uPerPixel` and its branch in the pixel shader, `u_.perPixel` and its
  three `glUniform1i` calls (`ApplyState`, `Draw2DQuad`). `vNormal` stays (G5 uses it).
- `src/framework/d3dtextr.cpp`: `SoftenShadow` and its special case in `Restore`; the blob
  texture is made like every other.
- Docs: README (Options > Graphics), CLAUDE.md, ROADMAP (a G1 row), `RENDERER_OPTIMIZATION.md`
  (its check list names both), CHANGELOG (in plain words: "removed: they made little difference;
  better shadows and lighting are coming").

Checks: at defaults, the screenshot suite byte for byte as 0.4.38; an ini with both set to 1 is
read without a warning and rewritten without them; the Graphics page and its help lines.

### G1 as built (2026-11-11)

As planned, in six files (79 lines out, 4 in). Also removed from the pixel shader: the material,
ambient and light uniforms only the per-pixel branch read (the vertex shader still declares and
uses them, so their locations and `ApplyState`'s calls are unchanged). `vNormal` is still passed
for G5. Old ini keys: `per_pixel_lighting` and `enhanced_shadows` are read and dropped without a
log line, and the next `Config::Save` leaves them out.

Checked, with the build still numbered 0.4.38 (the version is raised only on the owner's word):

- The four tests (`ctest`) pass.
- **Byte for byte**: nine 1280 x 960 shots (the title; 1-1 standing, and moving and firing; 1-6,
  2-4, 3-5, 3-10, 4-10, 1-10), each run of the old and new build identical: old against old,
  new against new, old against new. The first run of the old build in its fresh folder differed
  from every other run in moving things (particles, platforms; up to 212 in a byte), as timing
  noise does; **compare from a folder's second run on**, not its first.
- An ini with both keys set to 1: 1-1 identical to the defaults, no "ignoring unknown setting"
  line in the log; with its `game_version` made older, the rewrite at start drops both keys.
- The Graphics page: Antialiasing, Render scale, Texture filtering, Smooth textures, Smooth 2D
  images, Brightness, Back.

### G2. The recorder, and measuring (0.4.40)

No new setting, no visible change: this chunk only moves where the level's 3D is sent to OpenGL,
so it can be checked on its own before any effect depends on it.

- `Device::SetGameplayView(true)` starts recording, `false` replays (a new `Device::Replay`). A
  temporary switch (`MBN_NO_RECORD=1`, test builds only) sends draws directly as today, to compare.
- `RecordedDraw`: the primitive type, vertex format, the first vertex and index in the frame
  buffer and their counts, the world / view / projection matrices, the material, the light state,
  the texture, the render states and texture stage states `ApplyState` reads, and the draw's kind
  (the table above). About 400 bytes a draw; a few hundred draws a frame.
- The frame buffer: one growing `std::vector` each for vertices (as bytes; both formats are 32
  bytes) and indices, kept between frames, cleared each frame. Indices are stored with their
  vertex offset added (32-bit indices on the card), or drawn with `glDrawElementsBaseVertex`.
- Replay sends each draw's state through today's `ApplyState`, so the picture is the same by
  construction; only the vertex source changes (the one upload instead of one each).
- A draw outside the gameplay view (the 2D, the shop's panel, the menus) is sent as now; a
  `Clear` inside it (none expected) ends the recording early and is logged.
- **GPU timers**: `GL_TIME_ELAPSED` queries around the replay (and, from G3 on, around each new
  pass), read a frame later so they never wait, shown in the Statistics Overlay's Full page as
  "3D x.xx ms, shadows x.xx, AO x.xx, bloom x.xx" (only the passes in use). This is what lets the
  owner and players see what each option costs.
- **Measure** before and after on the owner's machine (an RTX 5080 since 2026-11-11; the earlier figures in `RENDERER_OPTIMIZATION.md` were taken on an RTX 3090 Ti) with VSync off: 1-1, 3-10
  (the nest, the busiest), 4-10, at 1280 x 960 and 4K borderless. The frame rate must not fall;
  it may rise (one upload). And once on the integrated AMD graphics (Windows Settings > Display >
  Graphics, "Power saving"; the log's `GL_RENDERER` confirms which drew), as the low-end
  reference for every chunk after.

Checks: the full byte-for-byte suite (below) with the recorder on against 0.4.39; and against
the switch off in the same build.

### G2 as built (2026-11-11)

- **Recording** (`src/gfx/device.{h,cpp}`): `SetGameplayView(true)` starts it, `false` sends it
  (`FlushRecording`). `DrawIndexedPrimitive` and `DrawPrimitive` record while it is on: a
  `DrawState` (the transforms, material, light, texture and the nine render states and eight
  texture stage states `ApplyState` reads; `CurrentState()` copies them) and the draw's vertices
  and indices, appended to two vectors kept between frames. `ApplyState` is now
  `ApplyDrawState(DrawState, fvf)`, which reads only the state it is given; the live path passes
  `CurrentState()`, so a draw sent at once and a recorded one go through the same code.
- **What sends the recording early**, so nothing is drawn out of order or with a changed texture:
  any 2D (`ApplyState(0, true)`: the HUD, blits, fills, the menus' text), `Clear`, `Present`,
  `SaveScreenshot`, `OnSettingsChanged` (render targets made again), `BeginFrame`, and a texture
  being updated or deleted (`Texture::Update`, `~Texture`; a texture made lazily mid-level splits
  the recording in two, which changes nothing). A plain static flag (`g_recordPending`) guards the
  texture calls, so textures freed at exit, after the device, never touch it. **Any new code
  that talks to OpenGL during a level's 3D must call `FlushRecording()` first.**
- **The replay sends each draw as before, one upload each.** The plan's single upload for the
  frame was built first (one buffer, `glDrawElementsBaseVertex`) and checked: every shot matched
  except 1-6, where two pixels inside a slime's smooth-shaded body were one shade of green apart,
  in every run. Attribute offsets into the shared buffer gave the same; uploading each recorded
  draw on its own matched exactly, and so did either half on its own (levels' meshes or models)
  only when the other half also had its own uploads. No draw reads past its own vertices (checked
  with a temporary log). So the NVIDIA driver doesn't treat a draw from a large shared buffer
  quite like one from its own small upload, to the last bit. The game's draws keep their own
  uploads; the extra passes of G3 and G4 (depth only) may still share one.
- **`MBN_NO_RECORD=1`** (an environment variable, read at start; logged) sends each draw at once
  as before 0.4.39. Kept in the release build: it is how later chunks compare.
- **GPU timer**: eight `GL_TIME_ELAPSED` queries in turn around each replay, read at the next
  frames' `BeginFrame` without waiting (`PollGpuTimers`); `TakeGpu3D` hands the sum and the frames
  to the overlay once a second. The Statistics Overlay's Full page shows **GPU 3D** (the card's
  time for a level's 3D a frame; "- (no level)" outside one, "- (off)" with `MBN_NO_RECORD`) and
  **3D draws** (the last frame's). The time is the card's timeline between the first and last
  draw, waiting for the processor included, so it is an upper bound of the drawing itself.
- **Checked** (RTX 5080):
  - the four tests pass;
  - byte for byte, 14 shots (the title; 1-1 still and moving and firing; 1-6, 2-4, 3-5, 3-10,
    4-10, 1-10; Antialiasing changed to 8x and back through the menu mid-level; Smooth motion held
    at `alpha=0.5` in 1-1 and 1-6; 1-1 and 3-10 at 1600 x 900 Widescreen): G2 against G1, and G2
    against itself with `MBN_NO_RECORD=1`, all identical from a folder's second run (the first run
    of the 16:9 pair was a moment apart, as the first-run noise of G1);
  - frame rates, VSync off, Smooth motion on, 1280 x 960 window: 1-1 2034 (G1) and 2074 (G2), GPU
    3D 0.22 ms, 100 draws; 3-10 (the nest) 1097 (G1), 1104 (G2, not recording) and 1128 (G2),
    GPU 3D 0.72 ms, 267 draws. No loss; the differences are within a run's spread.
- **Not done here**: 4K borderless and the integrated AMD graphics (choosing it needs a Windows
  graphics setting changed, the owner's to make: Settings > System > Display > Graphics, add
  `MageBrosNative.exe`, "Power saving"). The Statistics Overlay's GPU 3D row is what to read there.

### G3. Ambient occlusion (0.4.41)

Setting: **Ambient occlusion: Off / Low / High** (`[video] ambient_occlusion = 0..2`). (As built: Off / On, Low removed after the owner's test.)

- **Depth pre-pass** (step 2): solid draws only, depth only, a single-sample `GL_DEPTH_COMPONENT24`
  texture at the render target's size (High) or half (Low). The colour writes off, the simplest
  shader.
- **The occlusion**: a screen-space pass over that depth. Proposed: a horizon-based method
  (GTAO-style, 2 directions x 4 steps at Low, 4 x 6 at High) with a radius in world units, not
  pixels (about 30 units: three-quarters of a map cell), so it looks the same at any resolution,
  Render scale and Field of view. Normals from the depth (the faces are flat). Noise from a 4 x 4
  pattern, then a 4 x 4 blur that stops at depth edges, so no speckle and no halo around the
  mages.
- **Applied** in step 6 to the receivers only (level and solid model parts), looked up at the
  pixel's screen position: on models to the ambient part (the direct light is not occluded, which
  is how it reads as soft contact shading); on the level to the whole baked colour, with a
  strength factor (the baked colour is mostly light, not ambient), default to tune by eye.
  Particles, shots, water and the blob shadows are never darkened, which a pass over the finished
  picture couldn't avoid.
- **Where it shows**: the bottoms of walls and cliffs, under the ledges of raised floor, between
  pillars and fence posts, under platforms, around the barrels and pots, the mages' feet.
- Cost, expected: a few tenths of a millisecond at 1080p on the owner's card; most of it on the
  integrated chip at High. Measured in the chunk.

Checks: Off byte for byte; screenshots of 1-1, 1-6, 2-4, 3-5 with Low and High, at 4:3, 16:9 and
21:9, Render scale 50% and 200%, Field of view 150%; no halo at the mage's edge against the far
floor; no shimmering while the camera moves (a short `--auto` run at 144 frames a second with
Smooth motion, screenshots a few frames apart); Smooth motion `alpha=1` against off.

### G3 as built (2026-11-11)

**Options > Graphics > Ambient occlusion: Off / Low / High** in 0.4.39, **Off / On** since (`[video] ambient_occlusion`, 0 by
default), in `src/core/config.*` and `src/ui/options_menu.cpp`; everything else in
`src/gfx/device.{h,cpp}` (`ComputeAO`, run by `FlushRecording` before the replay).

- **What takes it** (`DrawKind`): recorded draws that are solid (no blending, depth test and
  writing on): pre-lit ones (the level, its goal pads; kind 1) and lit models (kind 2). See-through
  draws (blob shadows, particles, shots, water, see-through model parts) neither hide anything from
  it nor take it. Outside a level's 3D (the title, the ending, the shop's panel, the HUD, menus)
  nothing changes.
- **1. Depth pass**: the solid draws into a texture of view-space depth (`GL_R32F`, 0 where nothing
  is drawn) with a depth buffer of its own, at the **picture's** size (`min(dest, fbo)`: Render
  scale's extra pixels add nothing to a soft shading), halved for Low. It draws from **one upload
  of the frame's vertices** (`glDrawElementsBaseVertex`), as the plan allowed: depth only, never
  seen directly. Each draw's own culling and lens (`DrawProj`, the projection with the widescreen
  and Field of view changes, now shared with `ApplyDrawState`).
- **2. Occlusion** (`kAOFS`): the surface's position from the depth and the lens, its facing from
  the nearer neighbour on each axis (so edges don't bend it), then 8 (Low) or 16 (High) samples in
  the half-sphere above it, out to **30 world units** (three-quarters of a map cell; a floor step
  is 25), turned by a 4 x 4 pattern. A sample counts as hidden where the depth there is in front of
  it, weighed by how far above the surface the hiding point stands (its angle), by its real 3D
  distance (fading out from 15 to 30 units) and **not at all when it is much nearer the camera
  than the surface** (fading out from 7.5 to 15 units nearer). The last two are what removed the
  dark outlines first seen round the post in 1-1 and the barrels in 1-6 (a thing standing in front
  of a wall shaded a ring on the wall round itself). Then `pow(clamp(1 - 3 x hidden share), 1.5)`.
- **3. Blur** (`kBlurFS`): the 4 x 4 block round each pixel (the pattern's size, so it evens out
  completely), each weighed by how close its depth is (tolerance 1 + 1% of the depth).
- **Applied** in the main shader, after the texture and before the constant colour: `result.rgb *=
  mix(1, ao, strength)`, the AO looked up at the pixel's place on screen (texture unit 1). The
  strength is set per draw by the replay: **0.9 for the level, 0.7 for models** (by eye: the
  level's baked colours are light, the models keep most of their lit sides), and 0 for everything
  else; `ApplyDrawState` and the 2D path set it to 0, so the live path and every other screen draw
  as before. The plan's "ambient part only" for models was dropped: the vertex shader would have
  had to pass the light in two parts, for a difference too small to see on these models.
- **Textures made mid-level no longer send the recording** (`Texture::Create` uploads through a new
  `Texture::Upload`; only `Update` of an existing texture and its deletion send it): a texture
  just made can't be in the recording, and splitting the recording would also split the AO.
- **Overlay**: Full shows **GPU AO** (the card's time for the three passes a frame) with AO on.
- **Checked** (RTX 5080):
  - the four tests pass;
  - **AO off, byte for byte**: the 14-shot suite against G2, identical (the usual run-to-run noise
    showed in both builds alike, in different shots each run, never in a pair run back to back);
  - **AO High and Low by eye**, against Off: 1-1, 1-6, 2-4, 3-5, 3-10, 4-2. Darker where walls
    meet (the room corners), the walls' and cliffs' feet, round the barrels, pots, pillars' and
    fences' feet, under the needle balls and the hammer, the lower parts of models; no speckle, no
    outlines after the fixes above. Low is a little lighter and softer than High;
  - 1-6 and 3-10 at 1600 x 900 Widescreen with Render scale 200% (render target 3200 x 1800, AO at
    1600 x 900) and Field of view 150%: correct, no seams;
  - Smooth motion on with `alpha=1` and AO High: byte for byte the same as Smooth motion off with
    AO High (1-1);
  - cost, VSync off, Smooth motion on (frames a second; the overlay's GPU AO):

    | Scene | Off | Low | High |
    |---|---|---|---|
    | 1-1, 1280 x 960 | 1915 | 1740 (0.20 ms) | 1644 (0.26 ms) |
    | 1-1, 1920 x 1440 | 1904 | 1736 (0.18 ms) | 1065 (0.51 ms) |
    | 3-10, 1280 x 960 | 1057 | 969 (0.32 ms) | 965 (0.41 ms) |
    | 3-10, 1920 x 1440 | 1042 | 974 (0.31 ms) | 887 (0.53 ms) |

    About half a millisecond at most (a 144 Hz frame is 6.9 ms). Low's cost is mostly the depth
    pass's draws, High's grows with the pixels.
- **Not done here**: the shimmer check with the camera moving (the pattern is fixed to the screen
  and fully blurred, so none is expected; the owner's play will show it), 21:9, Render scale 50%,
  4K, and the integrated AMD graphics.

**After the owner's test of 0.4.39 (2026-11-11): Low removed (0.4.40).** The owner found High nice and Low
"really bad", so the row is now **Off / On**, On being the former High (full picture size, 16
samples). `ambient_occlusion` is read as on for any value above 0 (0.4.39's 1 and 2 both become
on) and written as 0 or 1; `Config::ambientOcclusion` is a `bool`; `ComputeAO` lost its level.
Checked: the tests pass; an ini with the old `2` gives the same pictures as `1` (six levels, two
runs each). Not checked pixel for pixel against 0.4.39's High: Defender had removed every copy of
the 0.4.39 exe on this machine but the owner's own (see below), and the code path is the former
High's line for line. The cost is the High column above.

**A puzzle for later (found 2026-11-11):** in those runs, 1-6's slime came out with two pixels
one shade of green apart between the `1` and `2` runs, the same every time, and nowhere else. It
is the same slime and the same kind of difference that G2 put down to the driver treating a shared
buffer differently ("G2 as built"). Here nothing in the drawing differed, only one digit of the
ini, so the cause is more likely something in how the slime is drawn that depends on what is in
memory (a value never set, say), and G2's explanation may be wrong. Harmless (one shade, two
pixels, no game logic involved), but worth a look before the next byte-for-byte chunk.

**Windows Defender** removed the 0.4.39 test build on the owner's PC as
`Behavior:Win32/DefenseEvasion.A!ml`, a machine-learning verdict on a new unsigned exe; judged a
false positive (the source adds no system calls, and the exe imports the same 553 Windows
functions as 0.4.38). The owner restored it. Code signing is the owner's decision, not built.

### G4a. Dynamic shadows: the shadow map (0.4.42)

Setting: **Dynamic shadows: Off / Low / Medium / High** (`[video] dynamic_shadows = 0..3`), and
**Shadow strength** (50% to 100%, `[video] shadow_strength`, shown only with shadows on).

- **The light's view**: an orthographic view along the light the game set (`light_.direction`,
  read from `SetLight`, never hard-coded), fitted each frame around what the camera sees: the
  view's eight corners from the near plane out to the draw range (`visibleRange`, which already
  grows with widescreen and the Field of view), turned into the light's space, the box around
  them, then extended towards the light by the tallest thing that can cast into view (the level's
  walls, the bosses: about 300 units to start). The levels are a corridor about 200 wide and
  long along z, so the box is narrow and the map's pixels go far: 2048 x 2048 (Medium) gives
  under half a unit a pixel over a 16:9 view.
- **Casters**: the level and the solid model parts (step 4), depth only, front faces culled away
  where the mesh is closed (models) and none culled for the level (the original draws it without
  culling). The see-through parts, the rectangles and the blob shadows cast nothing.
- **Receivers** (step 6): the level and the solid model parts. One lookup with the card's own
  depth comparison (`GL_COMPARE_REF_TO_TEXTURE`, a 2 x 2 filtered result for free) in this chunk.
- **Acne and gaps**: a small offset along the surface's normal and a slope-scaled depth offset
  (`glPolygonOffset`) in the shadow pass, tuned on the worst cases: the floor near the camera,
  the mage's own back (low-poly models shadowing themselves), the thin fence planks.
- **The blob shadow stays, unchanged** (the owner, 2026-11-10: a gameplay aid, see "Decisions"). Where both fall on the same spot
  (a mage standing still: the blob below, the real shadow beside it, towards -x), both show.

Checks: Off byte for byte; all four worlds' typical levels and the four boss levels with shadows
on; the dragon's body over the water; a mage on a moving platform (1-6's elevator `A`); hero on
hero; the camera shake (2-2's hammer); widescreen and the Field of view at 150% (no shadow cut off
at the edges of the view); Render scale; Smooth motion (shadows move with the blended models, no
trailing).

### G4b. Dynamic shadows: soft edges and steadiness (0.4.43)

- **Steady while the camera moves**: the light's view snapped to whole shadow-map pixels in the
  light's space and its size rounded up to steps, so the shadows' edges don't crawl as the camera
  follows the mage (the classic shimmer).
- **Soft edges**: percentage-closer filtering over a small disc (Low 4 lookups, Medium 8, High 16
  with a rotated pattern), the softness growing a little with the distance from the caster
  (a contact-hardening look: sharp at a mage's feet, softer from the top of a pillar). This is
  what Soft shadows tried to do for the blob.
- **Quality levels**: Low 1024 and 4 lookups, Medium 2048 and 8, High 4096 and 16.
- **A fade** at the far end of the draw range, so shadows don't stop on a line.

Checks: as G4a, plus a camera pan recorded frame by frame (no crawling edges); the cost at each
level on the owner's card and the integrated chip.

### G4 as built (2026-11-11)

G4a and G4b were built together (one test build for the owner). **Options > Graphics > Dynamic
shadows: Off / Low / Medium / High** (`[video] dynamic_shadows` 0..3, 0 by default) and **Shadow
strength** (50% to 100% in steps of 10, `shadow_strength`, 80 by default). Code in
`src/gfx/device.{h,cpp}` (`ComputeShadows`, run by `FlushRecording` after the AO and before the
replay), the shaders, `src/core/config.*`, `src/ui/options_menu.cpp`; the overlay's Full page
shows **GPU shadows** (the map's pass).

- **Which part of the level** (each frame): rays through a 9 x 9 grid of the screen, from 10 to
  1000 along the view, clipped to the slab every level lies in (x -60 to 260, y -260 to 300; the
  camera on the +x side), give the z range the camera sees; it is widened by 280 towards -z (the
  light comes from +x, above and -z, so things there cast into view) and 40 the other way. This
  follows any window shape, the Field of view and the camera's shake, and no game code is asked.
- **The light's view**: orthographic, along the light the game set (`light.direction`, never
  hard-coded), round that box. Its size rounded up to steps of 64 and squared, its centre snapped
  to whole map pixels (so edges don't crawl as the camera follows the mage). Map: 1024 (Low), 2048
  (Medium), 4096 (High) square, `GL_DEPTH_COMPONENT24` with the card's own comparison.
- **Casters**: the solid draws (`DrawKind`: the level, solid model parts), both sides of every face,
  pushed back by `glPolygonOffset(1.5, 2)`, from the shared upload (now `UploadPrep`, shared with
  AO's depth pass, once a flush).
- **Receivers** (the main shader, `uShadowKind` set per draw by the replay):
  - **lit models** (2): the vertex shader passes the light in two parts (ambient and emissive;
    the direct light, with D3D7's un-normalized normal kept, so the coins keep their glow); in
    shadow the direct part is taken away by the strength: `clamp(ambient + direct x (1 -
    shadow x strength))`. Only with shadows on: off, the old `vColor` path is drawn, byte for byte;
  - **pre-lit surfaces** (1: the level, unlit models): darkened by up to 0.55 x strength where in
    shadow, only where they face the light (their normal from the screen-space derivatives of the
    position, turned towards the camera); faces turned away are already the baked dark side;
  - nothing see-through takes them (particles, shots, water, blob shadows).
- **Reading the map** (`LitBy`): the point moved 1.5 map pixels along its normal (against
  self-shadowing on low-poly models), then a grid of reads, each already blended over 2 x 2 map
  pixels by the card: 2 x 2 (Low), 3 x 3 (Medium), 4 x 4 (High), one map pixel apart; a constant
  0.0004 of depth off; fading out over the map's last 3% so nothing stops on a line. The
  **contact-hardening** softness of the plan (sharper at a mage's feet, softer from a pillar's
  top) was **not built**: the grid already gives soft, even edges.
- **The blob shadows stay** as they are (the owner, 2026-11-10).
- **What shows**: the end walls throw long diagonal shadows across the floor and the back wall;
  the mages, enemies, spikes, pillars, barrels, pots, fences and hammers cast shadows on the floor
  and the back wall; the shop's roof shades the shopkeeper; models are darker on the sides away
  from the light and where other things shade them.
- **Checked** (RTX 5080):
  - the four tests pass;
  - **shadows off, byte for byte**: the 14-shot suite against 0.4.40, identical in two runs (and
    each build identical to itself between them);
  - High against Off by eye in 1-1, 1-6, 2-4, 3-5, 3-10, 4-2, and close-ups (the hammer, the mage):
    clean edges, no self-shadowing marks on floors or models;
  - Low, Medium and High side by side (3-5's hammer): nearly the same at 1280 x 960, Low a little
    softer;
  - 1-6 and 3-10 at 1600 x 900 Widescreen, Render scale 200%, Field of view 150%, with AO: correct;
  - Smooth motion `alpha=1` against Smooth motion off, shadows High and AO on: byte for byte;
  - cost at 1920 x 1440, VSync off (the overlay's GPU shadows: the map's pass; the lookups are in
    GPU 3D): 1-1 Low 0.03 ms, Medium 0.09, High 0.16; 3-10 Low 0.22, Medium 0.28, High 0.28. The
    frame rates moved more between runs than the shadows cost.
- **Not checked here**: the edges' steadiness with the camera moving (by construction they shouldn't
  crawl; the owner's play will show), the mage on a moving platform, the bosses, two players, 4K,
  the integrated AMD graphics. For the owner to judge: the strength's default (80%) and whether all
  three quality levels earn their place (they look much alike at this size; AO's Low was removed
  for looking poor).

### G5a. Dynamic lights: finding them (0.4.44)

No picture change yet (lights listed, not used), so the list can be checked in the log first.

- **From what the game already draws**: each recorded additive rectangle whose texture is a
  light source becomes a light at the rectangle's centre (its world matrix's position: already
  the blended one with Smooth motion), with the colour and reach from a table, times the
  rectangle's own vertex colour and alpha (so a fading flame dims its light and the torch's
  particle-by-particle flicker carries over).
- **The table** (`src/gfx/light_sources.cpp`, the port's own, by texture resource ID): the torch
  and candle flames, each player's shot colours, the hero fire flash, the enemy explosion, the
  bullet explosion, the dragon's flame, the wizard's fire and its wand flash, the warp ring and
  photons. Not lights: the water's glints, coin sparkles, smoke, landing dust, steam (reviewed
  one by one with screenshots). `Texture` gets the resource ID it was made from (set in
  `D3DTextr`'s `Restore`), so `Device` can look it up.
- **Merging and the limit**: particles come in clusters (a torch keeps a few flames alive), so
  lights within about 15 units of each other merge into one (colours added, position weighted).
  At most 16 lights a frame (the brightest and nearest to the view's centre win), sent as one
  uniform array: plenty for a level, which shows a handful of torches at once.
- **Steadiness**: a light's strength eases towards its new value over a few frames (on the
  drawing clock, not the ticks), so merges and particles appearing don't pop. This changes no
  game timing.
- Logged once a second in a test build: the lights, their colours, positions and reach, in 1-1
  (torches), 1-10 (the dragon's flame), 4-10 (the wizard).

### G5b. Dynamic lights: lighting the level and the models (0.4.45)

Setting: **Dynamic lights: Off / On** (`[video] dynamic_lights`), and **Light strength** (50% to
150%, `[video] light_strength`).

- The shader terms in "How each draw is shaded": a smooth falloff to zero at each light's reach
  (no hard edge), the surface's facing taken into account, added after the shadow and AO terms
  (so a torch lights a shadowed corner: the lights cast no shadows of their own; that would be
  far more work for little in levels this open).
- **Lit torches glow, unlit ones don't**: torch flames exist only once a torch is lit
  (`fireOn`), so lighting a torch now visibly lights the area around it. That is the effect the
  game's whole goal is about.
- **The look in bright levels**: World 1 is daylight; a torch's light on a bright floor adds
  little. The owner decides by eye (see "Decisions") whether that is enough, or whether Dynamic
  lights should also dim the ambient a little (a deliberate change of the original's mood, so
  only as a separate, clearly named choice).

Checks: Off byte for byte; screenshots of a level before and after lighting its torches; a shot
flying along a wall at night-ish 3-x levels; the nest's 40 birds with the mage firing (the most
lights at once); the wizard's fireballs; two players' shot colours; Smooth motion (lights move
with the blended sprites).

### G6. Bloom (0.4.46)

Setting: **Bloom: Off / Low / High** (`[video] bloom = 0..2`).

- **Only the game's own glow**: in step 6 the additive rectangles also write into the glow target
  (a second colour attachment: `glDrawBuffers` with two targets for those draws, one for the
  rest). Bright floors, white walls and the sky don't glow, which a brightness cut-off would make
  them do in World 1.
- **Blur**: the glow target resolved, shrunk in 4 (Low) to 6 (High) halving steps and grown back
  with a small filter at each step (the "dual filter" method: cheap and smooth at any
  resolution), then added onto the scene (step 7), before the HUD.
- Strength tuned by eye, Low subtle, High clearly glowing; the torches, the shots, the warp,
  the dragon's breath and the wizard's fire are the test cases.

Checks: Off byte for byte; no glow on anything but additive rectangles; the shop opening over a
glowing level (the dim and the panel over it, not under); MSAA on and off; Render scale 50% and
200%.

### G7. The page, presets, the overlay and the merge (0.4.47)

- **The Graphics page**, finished (below), with help lines in plain words saying what each costs.
- **A Preset row** at the top (decision): Original (everything at the original's picture,
  Antialiasing and Texture filtering off too), Default (today's), Enhanced (AO High, shadows
  Medium, lights On, bloom Low), Custom (shown when anything differs).
- **The Statistics Overlay** (Full): the enhancements in effect, and the GPU time of each pass
  (from G2).
- **README** (a line in Options > Graphics), **CHANGELOG**, **CLAUDE.md**, **ROADMAP**, this file's
  "as built" sections; `RENDERER_OPTIMIZATION.md` updated for what the recorder already did.
- The full check list below on the owner's machine and the integrated chip; the owner's play test;
  then the merge into `main` (a pull request, as `smooth-motion-rework`), released as its last
  version.

## The Graphics page, after

| Row | Values | Default | ini (`[video]`) |
|---|---|---|---|
| Preset (G7, decision) | Original / Default / Enhanced / Custom | Default | `graphics_preset` |
| Antialiasing | Off, 2x, 4x, 8x, 16x | 4x | `msaa` |
| Render scale | 50% to 200% | 100% | `render_scale` |
| Texture filtering | Off, 2x to 16x | 8x | `anisotropy` |
| Smooth textures | Off / On | Off | `smooth_textures` |
| Smooth 2D images | Off / On | Off | `smooth_sprites` |
| **Ambient occlusion** | Off / On (Low removed after the owner's test) | Off | `ambient_occlusion` |
| **Dynamic shadows** | Off / Low / Medium / High | Off | `dynamic_shadows` |
| **Shadow strength** | 50% to 100% | 80% | `shadow_strength` |
| **Dynamic lights** | Off / On | Off | `dynamic_lights` |
| **Light strength** | 50% to 150% | 100% | `light_strength` |
| **Bloom** | Off / Low / High | Off | `bloom` |
| Brightness | 50% to 150% | 100% | `brightness` |

Removed: Per-pixel lighting (`per_pixel_lighting`), Soft shadows (`enhanced_shadows`).

The new keys need no `config_version` bump (each defaults to off when missing). Changing one
takes effect at once, like the others (the targets made again through `OnSettingsChanged` where
sizes change).

## Cost, what we expect (to be replaced by measurements)

On the owner's RTX 5080 everything together should cost well under a millisecond at 1080p and
perhaps 1 to 2 ms at 4K with High settings: nowhere near the 6.9 ms of a 144 Hz frame. The
question is weak hardware:

| Pass | Grows with | Expected weight |
|---|---|---|
| Depth pre-pass | draws (a few hundred) | small; the extra passes draw from one upload a frame |
| AO | pixels (resolution; not Render scale) | 0.2 to 0.5 ms on the RTX 5080 (G3); the largest on integrated graphics |
| Shadow map | draws, and the map's size | small at Low / Medium |
| Shadow lookups | pixels x lookups | moderate at High |
| Lights | pixels x lights (16 at most) | small |
| Bloom | pixels, once | small |

Antialiasing multiplies the cost of step 6 only (the extra passes are single-sample). With every
new option off, the cost is what the recorder costs, expected to be less than today.

## How each chunk is checked

The method of `SMOOTH_MOTION.md` ("How the game logic is shown untouched") and
`RENDERER_OPTIMIZATION.md` ("How to check each chunk"), with:

1. **Byte for byte with the new options off**, against the build before the chunk: smooth
   motion's suite (the 14 level shots and the shop sliding, 24 shots), the logo, title, save
   slots, level select, the ending, a level at 16:9 and 21:9, the Tab viewers. Check the test
   folder's ini hasn't kept `maximized = 1` first (the trap noted in `SMOOTH_MOTION.md`).
2. **Every existing Graphics option with the new ones on**: Antialiasing off and 8x, Texture
   filtering off and 8x, Smooth textures, Render scale 50% and 200%, Brightness, Field of view
   100% and 150%, Aspect 4:3, Stretch and Widescreen.
3. **The special paths**: a mage hit (the white flash), the see-through pass of models, the
   additive particles, the water, the shop's panel over the level, the options menu opened over a
   level and closed again, a window resize and Alt+Enter mid-level (targets made again), a level
   loaded after another (the recorder's buffers), the camera shake.
4. **Smooth motion**: `alpha=1` gives the same pictures as Smooth motion off, with each option on;
   `alpha=0.5` shadows and lights sit with their models.
5. **OpenGL's error reports** (a debug context in a test build), and **more than one driver**:
   NVIDIA, and the AMD integrated chip at least.
6. **The frame rate and the GPU timers**, before and after, on both.
7. The four tests (`ctest`), and the owner's play test.

Screenshots are game images: viewed, then deleted, never committed.

## Decisions for the owner

To settle before or during the chunks (the ones marked **before** block their chunk):

1. **The blob shadow with Dynamic shadows on: decided (the owner, 2026-11-10): keep it**, as the
   original draws it. It shows where a jumping mage lands, which the slanted real shadow doesn't.
2. **Shadow strength's default**: by eye, from screenshots (during G4a). AO's strength was set by
   eye in G3 (0.9 on the level, 0.7 on models; "G3 as built"), for the owner to judge in play.
3. **Dimming the ambient for Dynamic lights** (**during G5b**): none (proposed to start), or a
   separate "Moodier lighting" choice.
4. **Other screens**: levels only (proposed), or later also the title's mages and the ending's
   corridor.
5. **The Preset row** (**before G7**): wanted or not, and what Enhanced holds.
6. **Default settings**: all new options off by default (the project's rule; proposed). Any
   exception, as with Antialiasing and Texture filtering in 0.4.21, is the owner's call after
   playing with them.

## Later ideas (not in this plan)

Noted so they aren't lost; each would be its own small plan:

- **Antialiased cut-outs** (alpha to coverage with MSAA) for any colour-keyed 3D surfaces, if any
  show jagged edges (to check: the original's models are untextured, so likely only mods).
- **Depth haze**: a light fog towards the back of the draw range, for depth and to soften where
  the level ends at wide Fields of view.
- **Colour grading** (warmth, contrast, saturation) beside Brightness.
- **Sharpening** for Render scale below 100%.
- **The water**: reflections of the level and the mages in the pits' water.
- **The 2D at the window's own resolution** (the HUD and menus are drawn into the render target
  today, so Render scale and Brightness affect them): found in the graphics review of 2026-11-10.

## Files

| File | Change |
|---|---|
| `src/gfx/device.{h,cpp}` | The recorder and `FlushRecording` (G2); GPU timers (G2); the depth pre-pass, AO, shadow map, lights and bloom passes and their targets (G3 to G6); the shader's new paths; `uPerPixel` removed (G1) |
| `src/gfx/post_fx.{h,cpp}` (new, G3) | The AO, blur and bloom shaders and passes, to keep `device.cpp` readable |
| `src/gfx/shadow_map.{h,cpp}` (new, G4a) | The light's view fitting and snapping, the shadow map target |
| `src/gfx/light_sources.{h,cpp}` (new, G5a) | The table of glowing textures; finding, merging and easing the frame's lights |
| `src/gfx/gl.h` | The OpenGL functions newly used (depth textures and comparison, `glDrawBuffers`, timer queries, `glDrawElementsBaseVertex`) |
| `src/framework/d3dtextr.cpp` | `SoftenShadow` removed (G1); textures carry their resource ID (G5a) |
| `src/core/config.{h,cpp}` | Two settings out (G1), the new ones in (G3 to G7) |
| `src/ui/options_menu.cpp` | The Graphics page's rows |
| `src/ui/stats_overlay.cpp` | The enhancements in effect and their GPU times (G2, G7) |

Nothing in `src/game` changes.
