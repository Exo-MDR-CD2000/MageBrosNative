# R36S: build, probe and first device session

Date: 2026-11-12. Change: `r36s-gles` (`openspec/changes/r36s-gles`).

The console is aarch64 Debian 13 with no SDL3 and no desktop OpenGL, so this records what was
observed on the device itself. Gameplay acceptance by the owner is listed at the end as not yet
done.

## The device

| Item | Value |
|---|---|
| System | dArkOS RE, Debian 13.3 (trixie) |
| glibc | 2.41-12+deb13u1 |
| SoC / GPU | RK3326 / Mali-G31, kernel 4.4.189 |
| Memory | 895 MB |
| Display | 640x480 @ 60.02 Hz (`DSI-1`) |
| Cards | `/roms` internal 4.8 GB; `/roms2` second card 233 GB, 146 GB free, Ports folder here |
| Graphics | `libEGL.so`, `libGLESv2.so`, `libgbm.so` -> `libMali.so`; `libmali-bifrost-g31-rxp0-gbm` |
| SDL | SDL2 2.32.10 installed, **SDL3 absent** |
| Audio | ALSA (no SoundFont installed) |
| Gamepad | GO-Super Gamepad, GUID `1900bb3e4b4800000011000000010000` |

## Build

```sh
tools/r36s/build.sh
tools/r36s/install.sh ark@192.168.0.104
```

`build.sh` runs a Debian 13 `linux/arm64` container (glibc 2.41, the console's) with the pinned
SDL3 3.4.18 built statically and `MBN_GLES=ON`, KMSDRM and ALSA only. Tests run in the same
container: `formats` and `collision` pass on aarch64.

Executable: `build-r36s/MageBrosNative`, SHA-256
`613fb3b0c48039050049a66f34c634694fcb87c21f5e9e24bff3f79343a10543`.
`readelf -d` needs only `libstdc++`, `libm`, `libgcc_s` and `libc`; ALSA, DRM and GBM are loaded
by static SDL at run time. `ldd` on the console resolves all four.

## Probe

`tools/r36s/probe.cpp` (`-DMBN_BUILD_R36S_PROBE=ON`) ran on the console before the renderer was
touched:

```
sdl_version: 3.4.18
video_driver: kmsdrm
audio_driver: alsa
display: 640x480 @ 60.02 Hz
gamepads: 1
  [0] GO-Super Gamepad
      guid: 1900bb3e4b4800000011000000010000
      mapping: ...a:b0,b:b1,x:b2,y:b3,leftshoulder:b4,rightshoulder:b5,dpup:b8,...
gl_version: OpenGL ES 3.2 v1.r13p0-01rel0.8747d14aec16ec21ac2b2da252c4ec5f
gl_renderer: Mali-G31
gl_vendor: ARM
glsl_version: OpenGL GLSL ES 3.20
msaa_samples: 4
provoking_vertex: absent
probe_ok
```

The two open questions in the design are answered: SDL3's KMSDRM path works on this console, and
a 4x multisampled ES 3.0 context is available.

### The one environment hint that matters

SDL does not find the Mali EGL library by itself. Measured by running the probe in each
configuration:

| Configuration | Result |
|---|---|
| no hints | KMSDRM selected, window creation fails: "Can't load EGL/GL library" |
| `SDL_VIDEO_DRIVER=kmsdrm` | same failure |
| `SDL_OPENGL_LIBRARY=libGLESv2.so.2` | same failure |
| `SDL_EGL_LIBRARY=libMali.so` | **works** |
| `SDL_EGL_LIBRARY` + `SDL_OPENGL_LIBRARY` | works (the second adds nothing) |

The launcher therefore sets `SDL_EGL_LIBRARY` only, plus `SDL_VIDEO_DRIVER=kmsdrm` for
determinism.

## The game on the console

```sh
cd /roms2/ports/magebros
SDL_EGL_LIBRARY=/usr/lib/aarch64-linux-gnu/libMali.so \
  ./MageBrosNative --game MageBros.exe --auto "0:level=1,1;5000:shot=r36s-final.bmp;5500:quit"
```

Log:

```
Monitor 1: "DSI-1", 640x480 at 60.02 Hz, scale 100%, at (0, 0), primary
OpenGL OpenGL ES 3.2 v1.r13p0-01rel0.8747d14aec16ec21ac2b2da252c4ec5f | Mali-G31
OpenGL vendor: ARM
Renderer backend: gles
Provoking vertex: absent (smooth shading only)
Render target 640x480, MSAA 4x
```

Exit code 0, no GL errors, and level 1-1 draws the walls, floor tiles, water, bridge, columns and
HUD exactly as the macOS frame does. Screenshot `r36s-final.bmp` (640x480, BMP), SHA-256
`016388116481a40f9900f5316dc83b381963bfe8deef76397b8175019fa12f52`.

## Defects found on the way

Both were invisible on macOS and Windows:

- **A crash on exit.** `JModelList` and `JSurfaceList` keep GL buffers and textures in
  function-local statics; their destructors ran after `SDL_Quit`, i.e. without a context, and
  the Mali driver segfaulted after `main` returned. The caches are now cleared while the context
  is alive, and the shutdown stages are logged.
- **`GL_INVALID_VALUE` on every pre-lit draw.** ES has no `ARB_vertex_array_bgra`, where desktop
  GL passes `GL_BGRA` as the *size* argument. The ES path passed `GL_RGBA` in the same place;
  the correct call is `glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, ...)` and the
  vertex shader swizzles. Before the fix the level's walls and floor were black (zero vertex
  colour times the texture).

## Launcher and packaging

`/roms2/ports/MageBros.sh` plus `/roms2/ports/magebros/` (binary, `gamecontrollerdb.txt`, the
player's `MageBros.exe`, `magebros.log`). Nothing on the system is replaced. `tools/r36s/install.sh`
writes both over SSH.

## Defects found in the first play session

The owner played level 1-1 on the console and reported three things.

**The music was silent.** The console has no system MIDI synthesizer, so the port needs a
General MIDI SoundFont, and the original's `Music*.mid` files next to `MageBros.exe`. Both were
missing from the installed folder. With `soundfont.sf2` (a 3.2 MB GM bank already on the device,
from GZDoom's data) and the six `Music*.mid` files copied in, the log reports
`MIDI: using built-in synthesizer with .../soundfont.sf2` and
`music: SoundFont synth`. This is a packaging gap, not a renderer defect; it is now documented in
the README and the launcher's folder.

**There was no way out of a level.** SDL's mapping for an unlisted pad is a guess: it invents
`a:b0..righttrigger:b7` and `dpup:b8..dpright:b11`. On this device the d-pad is 8/11/10/9 and
Select/Start are **12 and 13**, so the guessed d-pad silently swallowed them and no START event
ever reached the game. The in-game menu (F10 / Start) is where "Level select" and "Quit game"
live, so on a console with no keyboard there was no exit at all. Measured with
`mbn_r36s_probe --buttons` while each button was pressed: Start 13, Select 12, d-pad up 8,
down 11, left 10, right 9, L1 4, A 1. The shipped mapping now names them and SDL reports it back
(with `crc:3ebb`).

**The game stutters.** Deferred by the owner's decision; the frame time was not measured in this
session.

## Not verified

- Starting from the Ports menu. EmulationStation scans `/roms2/ports` at start; the entry was
  added while it was running, so the owner needs to restart the frontend and pick it once.
- Gamepad movement, jump and fire during play; sound audibility; exiting back to the menu and
  the console recovering. The launcher and the ES session are the owner's next step.
- Frame time and memory under a real session.
