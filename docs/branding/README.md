# Branding

Original artwork made for Mage Bros. Native. It contains no artwork from the game: the brick,
tiles, torches and hats are drawn from scratch.

| File | What it is |
|---|---|
| `../banner.png` | README banner, 2560×640 (shown at 1280×320) |
| `banner.svg` | Banner source: a torchlit brick wall, a tiled floor and the two mages' hats (blue for player 1, pink for player 2). The text uses the [Inter](https://rsms.me/inter/) font (SIL Open Font License). |
| `icon.svg` | Program icon, used for sizes 40 px and up: the blue hat on the tiled floor in front of the brick wall |
| `icon_small.svg` | Simplified icon for 16–32 px (no bricks, glow or star; straight sides and a thick band) |
| `../../res/magebros_native.ico` | Windows icon (16, 20, 24, 32, 40, 48, 64, 128 and 256 px), built into `MageBrosNative.exe` with its version information by `res/magebros_native.rc` |

To change the artwork, edit the SVGs, then run `python tools/render_branding.py` from the
repository folder (Windows, with Microsoft Edge and Pillow): it writes `docs/banner.png` and
`res/magebros_native.ico`. The banner's text is drawn with Inter loaded from Google Fonts, so
it needs an internet connection; check the result before committing it.
