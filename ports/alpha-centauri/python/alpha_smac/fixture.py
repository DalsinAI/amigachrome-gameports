# SPDX-License-Identifier: GPL-3.0-or-later
"""Write a synthetic stand-in for an Alpha Centauri install (for tests and a
look at the frontend without original files). Nothing here is Firaxis data.

    python -m alpha_smac.fixture OUTDIR

It runs ../wasm/make_fixture.py (synthetic rules of the native play test,
computed openinga/xopeninga/texture/Units/console PCX patterns, a free
system font under the original font names) and adds what the map window
reads besides: computed faction base art (new1.pcx .. new7.pcx, magenta
key, a border colour at 161,749), palette.pcx, and short computed tones as
the effect, menu-music and music files the audio layer looks for.
"""
import math
import struct
import subprocess
import sys
import wave
from pathlib import Path

from ._lib import NATIVE_DIR

WASM_DIR = NATIVE_DIR.parent / 'wasm'
FACTION_RGB = [(170, 170, 170), (220, 200, 40), (60, 170, 60), (220, 80, 60), (90, 120, 230), (210, 120, 40), (170, 90, 200), (60, 190, 190)]


def _make_fixture():
    sys.path.insert(0, str(WASM_DIR))
    import make_fixture
    return make_fixture


def faction_art(index):
    """1024x768 base sprites: 4 sizes x 6 rows in 101x76 cells on magenta."""
    mf = _make_fixture()
    pal = [255, 0, 255] * 256
    r, g, b = FACTION_RGB[index % 8]
    for k in range(1, 64):
        pal[3 * k:3 * k + 3] = [min(255, r * k // 40), min(255, g * k // 40), min(255, b * k // 40)]
    pal[300:303] = [40, 40, 40]

    def px(x, y):
        if 150 <= x <= 172 and 740 <= y <= 758:
            return 40  # border colour sample (161,749)
        cx, cy = x % 101, y % 76
        col, row = x // 101, y // 76
        if col > 3 or row > 5:
            return 0
        size = 14 + 6 * col
        if abs(cx - 50) + abs(cy - 46) * 2 < size * 2 and cy > 20:
            return 30 + (cx + cy) % 20
        if row % 3 and abs(cx - 50) < size + 6 and 58 <= cy <= 62:
            return 100  # perimeter defence bar
        return 0
    return mf.pcx(1024, 768, px, pal)


def tone(path, freq, seconds=0.25, rate=22050):
    path.parent.mkdir(parents=True, exist_ok=True)
    n = int(rate * seconds)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b''.join(struct.pack('<h', int(6000 * math.sin(2 * math.pi * freq * i / rate) * (1 - i / n))) for i in range(n)))


def make(out):
    out = Path(out)
    subprocess.run([sys.executable, str(WASM_DIR / 'make_fixture.py'), str(out)], check=True, stdout=subprocess.DEVNULL)
    mf = _make_fixture()
    for i in range(1, 8):
        (out / f'new{i}.pcx').write_bytes(faction_art(i))
    (out / 'palette.pcx').write_bytes(mf.pcx(16, 16, lambda x, y: (x + 16 * y) % 256, mf.palette(13)))
    for name, f in (('ok.wav', 880), ('menu up.wav', 660), ('menu down.wav', 440), ('menu out.wav', 550), ('mmenu.wav', 990),
                    ('cpu turn complete.wav', 330), ("CPU please don't go.wav", 220)):
        tone(out / 'fx' / name, f)
    tone(out / 'opening menu.wav', 262, 1.0)
    tone(out / 'music' / 'aset1.wav', 196, 2.0)
    (out / 'alpha-audio.cfg').write_text('map.unit_move=ok.wav\nmap.base_built=mmenu.wav\nmap.turn_start=menu out.wav\nmusic_volume=50\n')
    return out


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('usage: python -m alpha_smac.fixture OUTDIR')
    print(make(sys.argv[1]))
