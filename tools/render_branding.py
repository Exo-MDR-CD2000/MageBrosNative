"""Renders docs/banner.png and res/magebros_native.ico from the SVGs in docs/branding.

Draws each SVG at its exact pixel size with Microsoft Edge (headless, transparent background),
then packs the icon sizes into one .ico with Pillow: icon.svg for 40 px and up, icon_small.svg
for 16 to 32 px. The banner's text uses Inter, loaded from Google Fonts while it renders.
Needs Windows, Microsoft Edge, Python 3 and Pillow (pip install pillow).

Run from the repository folder:  python tools/render_branding.py
(the work files go to build/branding-render)
"""
import os
import subprocess
import sys
import time

from PIL import Image

root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
work = os.path.join(root, "build", "branding-render")
branding = os.path.join(root, "docs", "branding")
os.makedirs(work, exist_ok=True)
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"


def render(svg_path, width, height, png_path):
    svg = open(svg_path, encoding="utf-8").read()
    # the SVG drawn at the target size; Inter from Google Fonts for the banner's text
    page = os.path.join(work, "page_%dx%d.html" % (width, height))
    with open(page, "w", encoding="utf-8") as f:
        f.write('<!doctype html><html><head><meta charset="utf-8">'
                '<link href="https://fonts.googleapis.com/css2?family=Inter:wght@500;800;900&display=block" rel="stylesheet">'
                '<style>html,body{margin:0;padding:0;background:transparent;overflow:hidden}'
                'svg{display:block;width:%dpx;height:%dpx}</style></head><body>%s</body></html>'
                % (width, height, svg))
    profile = os.path.join(work, "edge-profile")
    if os.path.exists(png_path):
        os.remove(png_path)
    # (Edge's launcher returns at once; the headless browser writes the screenshot a little later)
    subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
                    "--user-data-dir=" + profile, "--force-device-scale-factor=1",
                    "--default-background-color=00000000", "--window-size=%d,%d" % (width, height),
                    "--virtual-time-budget=10000", "--screenshot=" + png_path, "file:///" + page.replace("\\", "/")],
                   check=True, capture_output=True, timeout=120)
    last, stable = -1, 0
    for _ in range(600):
        time.sleep(0.1)
        size = os.path.getsize(png_path) if os.path.exists(png_path) else -1
        stable = stable + 1 if size > 0 and size == last else 0
        last = size
        if stable >= 5:
            break
    else:
        raise RuntimeError("no screenshot: " + png_path)
    img = Image.open(png_path).convert("RGBA")
    if img.size != (width, height):
        img = img.crop((0, 0, width, height))
    img.save(png_path)
    return img


banner = render(os.path.join(branding, "banner.svg"), 2560, 640, os.path.join(root, "docs", "banner.png"))
print("banner", banner.size)

sizes_large = [256, 128, 64, 48, 40]
sizes_small = [32, 24, 20, 16]
images = []
for s in sizes_large:
    images.append(render(os.path.join(branding, "icon.svg"), s, s, os.path.join(work, "icon_%d.png" % s)))
for s in sizes_small:
    images.append(render(os.path.join(branding, "icon_small.svg"), s, s, os.path.join(work, "icon_%d.png" % s)))
for im in images:
    print("icon", im.size, "corner alpha", im.getpixel((0, 0))[3], "centre alpha", im.getpixel((im.size[0] // 2, im.size[1] // 2))[3])

ico = os.path.join(root, "res", "magebros_native.ico")
images[0].save(ico, format="ICO", sizes=[im.size for im in images], append_images=images[1:])
check = Image.open(ico)
print("ico", os.path.getsize(ico), "bytes, sizes", sorted(check.info.get("sizes", [])))
