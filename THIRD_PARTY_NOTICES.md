# Third-party notices

| Component | Files | License |
|---|---|---|
| [ColDet 1.2](https://sourceforge.net/projects/coldet/) by Amir Geva | `third_party/coldet/` | GNU LGPL version 2 or later. See `third_party/coldet/COPYING` and below |
| [TinySoundFont](https://github.com/schellingb/TinySoundFont) (tsf, tml) by Bernhard Schelling | `third_party/tsf.h`, `third_party/tml.h` | MIT. See `third_party/TinySoundFont-LICENSE` |
| [dr_mp3, dr_flac](https://github.com/mackron/dr_libs) by David Reid | `third_party/dr_mp3.h`, `third_party/dr_flac.h` | Public domain (Unlicense) or MIT No Attribution, your choice. See `third_party/dr_libs-LICENSE` |
| [stb_image, stb_vorbis](https://github.com/nothings/stb) by Sean Barrett | `third_party/stb_image.h`, `third_party/stb_vorbis.c` | Public domain (Unlicense) or MIT, your choice. See `third_party/stb-LICENSE` |
| [stb_truetype](https://github.com/nothings/stb) by Sean Barrett (v1.26) | `third_party/stb_truetype.h` | Public domain (Unlicense) or MIT, your choice. See `third_party/stb-LICENSE` |
| [cgltf](https://github.com/jkuhlmann/cgltf) by Johannes Kuhlmann | `third_party/cgltf.h` | MIT. See `third_party/cgltf-LICENSE` |
| [font8x8](https://github.com/dhepper/font8x8) by Daniel Hepper, based on Marcel Sondaar's public-domain font | `third_party/font8x8_basic.h` | Public domain |
| [TimGM6mb](https://timbrechbill.com/saxguru/Timidity.php) by Tim Brechbill, with David Bolton | `third_party/soundfonts/TimGM6mb.sf2` | GNU GPL version 2. See `third_party/TimGM6mb-LICENSE` and below |
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

## TimGM6mb

The desktop packages include `TimGM6mb.sf2` (5,969,788 bytes, SHA-256
`c5378b62028c920cb11e4803327983fee2f2cdff5dc89c708e39da417e51c854`) as the built-in
General MIDI bank. It is the bank from Debian's `timgm6mb-soundfont` 1.3, originally shipped
with MuseScore 1.x. Copyright (C) 2004 Tim Brechbill; copyright (C) 2010 David Bolton. It is
distributed under the GNU General Public License, version 2; the full text is in
`licenses/TimGM6mb-LICENSE`. The bank is read as data at run time and is not linked into the
game; the game's own source remains under the MIT licence in `LICENSE`.

Source: <https://deb.debian.org/debian/pool/main/t/timgm6mb-soundfont/>; upstream:
<https://github.com/musescore/musescore-old/commit/90c33ef9d87b3f5ff92efd3b07d89eb455fb1fef>.

## Game content

The game Mage Bros. and all of its assets are (c) 2003-2009 Saito Games and are not part of this
repository. See `LICENSE`, part 2.
