#!/usr/bin/env python3
"""Build AstroMenace's AmigaOS 3.x Workbench GlowIcon.

The artwork is intentionally original: a silver/red interceptor crossing an
orange planet, reduced to a clear 64x64 silhouette that remains recognisable
on a classic Workbench.  The selected state adds a cyan OpenGPU-style glow.

Usage:
    make_workbench_icon.py AMIGAICON_PY OUTPUT.info [PREVIEW.png]

AMIGAICON_PY is AmigaChrome Guest's tools/icons/amigaicon.py.  The generated
DiskObject is a WBTOOL with normal and selected GlowIcon images, a classic
four-colour fallback, default tool AstroMenace and a 1 MB stack.
"""
from __future__ import annotations

import importlib.util
from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

SIZE = 64
SCALE = 4


def load_icon_module(path: Path):
    spec = importlib.util.spec_from_file_location("amigachrome_amigaicon", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load Amiga icon writer from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def p(points):
    return [(int(x * SCALE), int(y * SCALE)) for x, y in points]


def build_art() -> Image.Image:
    w = h = SIZE * SCALE
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im, "RGBA")

    # A warm planet gives the tiny icon a strong AstroMenace identity without
    # borrowing upstream artwork.  Several rings keep it legible after the
    # GlowIcon palette reduction.
    d.ellipse((5 * SCALE, 5 * SCALE, 49 * SCALE, 49 * SCALE),
              fill=(72, 7, 15, 248), outline=(255, 97, 28, 255), width=2 * SCALE)
    d.ellipse((8 * SCALE, 8 * SCALE, 46 * SCALE, 46 * SCALE),
              fill=(150, 20, 24, 255), outline=(255, 170, 48, 210), width=SCALE)
    d.arc((11 * SCALE, 11 * SCALE, 43 * SCALE, 43 * SCALE),
          198, 342, fill=(255, 219, 121, 215), width=2 * SCALE)
    d.arc((9 * SCALE, 9 * SCALE, 47 * SCALE, 47 * SCALE),
          22, 151, fill=(75, 0, 9, 190), width=3 * SCALE)

    # Sparse stars around the silhouette.
    for x, y, r in ((52, 8, 1), (57, 19, 1), (4, 49, 1), (53, 51, 1),
                    (10, 59, 1), (35, 3, 1)):
        d.ellipse(((x-r) * SCALE, (y-r) * SCALE,
                   (x+r) * SCALE, (y+r) * SCALE), fill=(210, 235, 255, 235))

    # Engine plumes, underneath the hull.  A broad orange body and pale core
    # survive both the 8-bit GlowIcon and four-pen classic fallback.
    d.polygon(p([(10, 47), (1, 57), (15, 52), (22, 45)]),
              fill=(255, 69, 10, 225))
    d.polygon(p([(15, 51), (7, 63), (22, 54), (27, 46)]),
              fill=(255, 127, 16, 235))
    d.polygon(p([(12, 49), (5, 56), (17, 51), (22, 46)]),
              fill=(255, 235, 167, 255))
    d.polygon(p([(18, 52), (12, 60), (23, 53), (27, 47)]),
              fill=(255, 236, 168, 255))

    # Main interceptor: long, forward-pointing and asymmetrical enough to read
    # as a spacecraft rather than a generic triangular badge.
    hull = [(12, 42), (28, 29), (53, 12), (48, 33), (58, 37),
            (40, 41), (31, 54), (27, 45)]
    d.polygon(p(hull), fill=(207, 218, 225, 255),
              outline=(19, 28, 45, 255))
    d.line(p(hull + [hull[0]]), fill=(7, 13, 27, 255), width=2 * SCALE,
           joint="curve")

    # Wings and red armour panels.
    d.polygon(p([(27, 34), (11, 35), (22, 43), (34, 39)]),
              fill=(132, 145, 158, 255), outline=(10, 16, 30, 255))
    d.polygon(p([(35, 30), (36, 51), (45, 39), (49, 26)]),
              fill=(156, 170, 181, 255), outline=(10, 16, 30, 255))
    d.polygon(p([(22, 40), (31, 31), (39, 26), (34, 38), (27, 45)]),
              fill=(198, 29, 32, 255), outline=(55, 4, 9, 255))
    d.polygon(p([(40, 24), (51, 15), (47, 29), (39, 35)]),
              fill=(238, 70, 41, 255), outline=(58, 5, 7, 255))
    d.polygon(p([(32, 39), (40, 36), (37, 47), (31, 51)]),
              fill=(154, 17, 25, 255), outline=(48, 3, 8, 255))

    # Canopy, nose light and panel highlights.
    d.polygon(p([(33, 30), (42, 24), (40, 33), (34, 37), (29, 37)]),
              fill=(27, 105, 160, 255), outline=(5, 23, 41, 255))
    d.polygon(p([(35, 30), (40, 27), (39, 31), (34, 34)]),
              fill=(136, 223, 255, 245))
    d.ellipse((50 * SCALE, 11 * SCALE, 55 * SCALE, 16 * SCALE),
              fill=(255, 244, 185, 255))
    d.line(p([(18, 40), (28, 34), (42, 21)]),
           fill=(250, 252, 255, 205), width=SCALE)
    d.line(p([(29, 44), (38, 39), (47, 34)]),
           fill=(255, 143, 95, 210), width=SCALE)

    # Clean antialiasing at the final Workbench size.
    return im.resize((SIZE, SIZE), Image.Resampling.LANCZOS)


def selected_from(normal: Image.Image) -> Image.Image:
    alpha = normal.getchannel("A")
    glow = Image.new("RGBA", normal.size, (0, 0, 0, 0))
    cyan = Image.new("RGBA", normal.size, (42, 190, 255, 0))
    cyan.putalpha(alpha.filter(ImageFilter.GaussianBlur(3)))
    glow = Image.alpha_composite(glow, cyan)
    bright = ImageEnhance.Brightness(normal).enhance(1.16)
    bright = ImageEnhance.Color(bright).enhance(1.12)
    return Image.alpha_composite(glow, bright)


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print(__doc__.strip(), file=sys.stderr)
        return 2

    icon_writer_path = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    preview = Path(sys.argv[3]).resolve() if len(sys.argv) == 4 else None
    if not icon_writer_path.is_file():
        raise FileNotFoundError(icon_writer_path)

    amigaicon = load_icon_module(icon_writer_path)
    normal = build_art()
    selected = selected_from(normal)
    icon = amigaicon.Icon(
        type=amigaicon.WBTOOL,
        width=SIZE,
        height=SIZE,
        default_tool="AstroMenace",
        stack=1_000_000,
        normal=normal,
        selected=selected,
    )

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(amigaicon.glowicon(icon))
    check = amigaicon.read(output.read_bytes())
    if check.get("type") != amigaicon.WBTOOL or check.get("gadget") != (SIZE, SIZE):
        raise RuntimeError(f"generated icon did not validate: {check}")
    if check.get("default_tool") != "AstroMenace" or check.get("stack") != 1_000_000:
        raise RuntimeError(f"generated icon metadata did not validate: {check}")

    if preview:
        preview.parent.mkdir(parents=True, exist_ok=True)
        sheet = Image.new("RGBA", (SIZE * 2 + 12, SIZE), (0, 0, 0, 0))
        sheet.alpha_composite(normal, (0, 0))
        sheet.alpha_composite(selected, (SIZE + 12, 0))
        sheet.save(preview)

    print(f"ASTROMENACE_ICON={output}")
    print(f"ASTROMENACE_ICON_BYTES={output.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
