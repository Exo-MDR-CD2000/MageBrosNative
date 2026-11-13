# Linux desktop: build and acceptance

Date: 2026-11-13. Change: `linux-release`.

Status: **complete.** The first Debian 13 live session found a crash when switching the Music
synth setting; the cause was fixed, and the owner re-ran the fixed artifact successfully. The
first Linux release publication remains a release follow-up.

## Artifact

`MageBrosNative-0.4.47-linux-x86_64.tar.gz`, built by `tools/linux/build.sh` in a Debian 13
x86_64 container with a static SDL3 3.4.18. The first live session used
`cf45033a4f98e00c448b3c4aa5e5cb574194920c2b07781ac901a65121175120`; the fixed artifact is
`0b82c31dcb1d9e42fa63d1bf8bd298f3a6980141740f9dcfe81b2db019ed73af`.

## Machine

From the live session (`debian_check.txt` and the game log):

| Item | Value |
|---|---|
| System | Debian 13.7 live |
| glibc | 2.41-12+deb13u4 |
| Graphics | AMD Radeon Vega 8 Graphics (radeonsi, raven, ACO, Mesa 25.0.7) |
| Display | 1920x1080 at 60.03 Hz |
| Session type | not recorded |
| Original | `MageBros.exe`, SHA-256 `ac191080af002b8ad0615e432faa92a2a47ce76a9aaa667bacf628836c098c49` |

## Build and dependency checks

- `dpkg -l | grep -i sdl3` produced no output: SDL3 is not installed on the machine.
- `ldd ./MageBrosNative` lists only `libm`, `libc` and the loader.
- The game opened a desktop OpenGL 4.6 core context through Mesa and reached level 1-1.
- The packaged TimGM6mb bank and a user `soundfont.sf2` both loaded.

## The session

The normal log (`magebros_native.log`) shows level 1-1, display changes (borderless, window,
MSAA 4x and 8x, AO, shadows, bloom), level 1-2, and `Shutdown: done`. No game or renderer defect
was reported.

`debian_check.txt` records the expected Debian 13.7 / glibc 2.41 / no-SDL3 / minimal `ldd`
result.

## Defect: switching the synthesizer

The crash log (`magebros_native_linux_crash.log`) ends after this sequence:

```text
MIDI: using SoundFont .../soundfont.sf2
MIDI: using the built-in TimGM6mb bank with .../TimGM6mb.sf2
MIDI: failed to load SoundFont  @A�XU./soundfont.sf2
MIDI: the selected SoundFont is unavailable; music stays silent
```

Root cause: the new `BasePath()` helper freed the pointer returned by `SDL_GetBasePath()`.
SDL3 caches that string and returns the same pointer on later calls, so the next synthesizer
switch read freed memory; the garbage path then caused the crash. The fix is to return the
cached string without freeing it.

A second, cosmetic issue in the same path: when the game was started with `./MageBros.exe`,
`Assets::GameDir()` was `./`, which added the program folder a second time and produced duplicate
`.sf2` menu entries (including `TimGM6mb.sf2` as a user file). `SoundFontDirs()` now resolves that
relative spelling against the current directory and drops it when it is the program folder.

## Verification after the fix

- A Debian 13 x86_64 container with Xvfb and Mesa llvmpipe, using the real game files, cycled
  the Music synth setting repeatedly: user SoundFont → built-in → user SoundFont, with no
  corrupted path, no failed load and `Shutdown: done`.
- macOS actual-game checks: user `soundfont.sf2` wins in Automatic; without it the DLS synth is
  used; `builtin` uses TimGM6mb; the menu shows the new order.
- `ctest` on macOS: 6/6. `ctest` in the Linux container: 3/3.
- **Owner re-test of the fixed artifact (`0b82c31...`) on Debian 13:** the game plays normally,
  repeated Music synth switching works, and the crash no longer occurs. The owner reports that
  Linux is in order.

## Windows

The Windows log (`magebros_native_windows.log`) shows the user SoundFont, the Windows system
synthesizer and the built-in TimGM6mb bank being selected in turn, with normal shutdown. The
owner reports that Windows behaves correctly and that the new builds contain both
`TimGM6mb.sf2` and `licenses\TimGM6mb-LICENSE`.

## Release follow-up

- The release workflow includes the Linux tarball; the first published Linux asset will be
  checked on the next release. The owner explicitly deferred that publication check and asked
  that it not block this change.
