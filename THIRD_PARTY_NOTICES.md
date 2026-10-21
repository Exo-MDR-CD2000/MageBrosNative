# Third-party notices

| Component | Files | License |
|---|---|---|
| [ColDet 1.2](https://sourceforge.net/projects/coldet/) by Amir Geva | `third_party/coldet/` | GNU LGPL version 2 or later. See `third_party/coldet/COPYING` and below |
| [TinySoundFont](https://github.com/schellingb/TinySoundFont) (tsf, tml) by Bernhard Schelling | `third_party/tsf.h`, `third_party/tml.h` | MIT. See `third_party/TinySoundFont-LICENSE` |
| [dr_mp3, dr_flac](https://github.com/mackron/dr_libs) by David Reid | `third_party/dr_mp3.h`, `third_party/dr_flac.h` | Public domain (Unlicense) or MIT No Attribution, your choice. See `third_party/dr_libs-LICENSE` |
| [stb_image, stb_vorbis](https://github.com/nothings/stb) by Sean Barrett | `third_party/stb_image.h`, `third_party/stb_vorbis.c` | Public domain (Unlicense) or MIT, your choice. See `third_party/stb-LICENSE` |
| [cgltf](https://github.com/jkuhlmann/cgltf) by Johannes Kuhlmann | `third_party/cgltf.h` | MIT. See `third_party/cgltf-LICENSE` |
| [font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper, based on Marcel Sondaar's public-domain font | `third_party/font8x8_basic.h` | Public domain |
| [SDL3](https://libsdl.org) | Not included; CMake downloads it at build time | zlib license |

## ColDet

The original Mage Bros. was built with ColDet 1.2 (C++ 3D Collision Detection Library,
Copyright (C) 2000 Amir Geva). It is included here unmodified, from the `coldet12.zip` release
(`third_party/coldet/SHA256SUMS` lists every file), and compiled as a separate static library. Two
build settings let the 2000-era source build on current compilers without editing it: `tritri.c`
is compiled as C, and `third_party/coldet/compat/coldet_compat.h` (ours) is force-included.

This project's source is public, so you can rebuild the program with a modified ColDet, as the
LGPL requires: replace the files in `third_party/coldet/src` and build. Binary releases include the
LGPL text as `licenses/coldet-LGPL.txt`.

The game Mage Bros. and all of its assets are (c) 2003-2009 Saito Games and are not part of this
repository. See `LICENSE`, part 2.
