#!/usr/bin/env python3
"""make-icon.py [PREVIEW.png]: draws Arkanoid's icon as pixel art in the game's own colours (a wall with a brick
gone, the ball and the Vaus, in the playfield's pipes) and writes

  docs/icon.svg            the 32-grid picture, as the sister sites draw their icons (tiger.v2core.com shows it)
  resources/Arkanoid.icns  the application's icon: 128 and 32 px from the 32 grid, 48 and 16 px from a 16 grid
                           drawn for small sizes, each 24-bit colour (it32, ih32, il32, is32) with its 8-bit mask.
                           PNG elements in an icns are 10.5's, so Tiger needs these.

Run it from anywhere; it needs Pillow."""
import struct, sys
from pathlib import Path
from PIL import Image

HERE = Path(__file__).resolve().parent.parent

OUTLINE, PIPE_HI, PIPE_LO, FIELD = "383c48", "c8d0e0", "888c9c", "1c2c78"
# base, highlight, shade: brick_cols in src/game.c
SILVER = ("e0e0e0", "ffffff", "9898a8")
RED = ("e02828", "ff8878", "901010")
YELLOW = ("f0d828", "fff8a0", "a88800")
BLUE = ("2850e8", "80a8ff", "1828a0")
# four of the Vaus's eight rows (vb_cap, vb_body, vb_band in src/game.c)
CAP = ("ff9c9c", "f05858", "d03030", "801418")
BODY = ("f0f4ff", "c8d0e0", "a0a8b8", "646878")
BAND = ("b0f8ff", "70d8f0", "40b0d0", "186878")
BALL = {"W": "ffffff", "w": "e8e8f0", "v": "b0b0c0", "u": "707080"}


def tile(n, pipe):
    """An n-by-n grid of colours (None: clear): a rounded tile, its outline, `pipe` rings of pipe, the field."""
    cut = {0: 2, 1: 1} if n > 16 else {0: 1}

    def inside(x, y):
        if not (0 <= x < n and 0 <= y < n):
            return False
        edge = min(y, n - 1 - y)
        return not (edge in cut and min(x, n - 1 - x) < cut[edge])

    outside = [(x, y) for y in range(-1, n + 1) for x in range(-1, n + 1) if not inside(x, y)]
    g = [[None] * n for _ in range(n)]
    for y in range(n):
        for x in range(n):
            if inside(x, y):
                # rings by the distance to the nearest clear pixel, so the corners round like the edges
                d = min(max(abs(x - ox), abs(y - oy)) for ox, oy in outside) - 1
                g[y][x] = OUTLINE if d == 0 else (PIPE_HI if d == 1 else PIPE_LO) if d <= pipe else FIELD
    return g


def brick(g, x, y, w, h, cols):
    base, hi, lo = cols
    for dy in range(h):
        for dx in range(w):
            c = base
            if dx == w - 1 or dy == h - 1:
                c = lo
            elif dy == 0 or dx == 0:
                c = hi
            g[y + dy][x + dx] = c


def vaus(g, x, y, w, rows, cap, band):
    for dy in range(rows):
        for dx in range(w):
            ex = min(dx, w - 1 - dx)
            if ex == 0 and (dy == 0 or dy == rows - 1) and rows > 2:
                continue
            k = dy if rows == 4 else (0, 2)[dy]
            g[y + dy][x + dx] = (CAP if ex < cap else BAND if ex < cap + band else BODY)[k]


def ball(g, x, y, rows):
    for dy, line in enumerate(rows):
        for dx, ch in enumerate(line):
            if ch != ".":
                g[y + dy][x + dx] = BALL[ch]


def big():
    g = tile(32, 2)
    for r, cols in enumerate((SILVER, RED, YELLOW, BLUE)):
        for i in range(4):
            if (r, i) != (3, 2):
                brick(g, 4 + 6 * i, 5 + 3 * r, 6, 3, cols)
    ball(g, 19, 19, (".Ww.", "Wwwv", "wwvu", ".vu."))
    vaus(g, 9, 24, 14, 4, 3, 1)
    return g


def small():
    g = tile(16, 1)
    for r, cols in enumerate((RED, YELLOW, BLUE)):
        for i in range(4):
            if (r, i) != (2, 2):
                brick(g, 2 + 3 * i, 3 + 2 * r, 3, 2, cols)
    ball(g, 9, 9, ("Ww", "wv"))
    vaus(g, 5, 12, 7, 2, 2, 0)
    return g


def image(g, scale):
    n = len(g)
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    for y, row in enumerate(g):
        for x, c in enumerate(row):
            if c:
                im.putpixel((x, y), tuple(int(c[i:i + 2], 16) for i in (0, 2, 4)) + (255,))
    return im.resize((n * scale, n * scale), Image.NEAREST)


def svg(g):
    paths = {}
    for y, row in enumerate(g):
        x = 0
        while x < len(row):
            c = row[x]
            n = 1
            while x + n < len(row) and row[x + n] == c:
                n += 1
            if c:
                paths.setdefault(c, []).append(f"M{x} {y}h{n}v1h-{n}z")
            x += n
    out = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" shape-rendering="crispEdges" width="32" height="32">',
           "<!-- Arkanoid: the wall with a brick gone, the ball and the Vaus, in the playfield's pipes. 32-grid pixel art,"
           " as the sister sites draw; tools/make-icon.py writes it -->"]
    out += [f'<path fill="#{c}" d="{"".join(d)}"/>' for c, d in paths.items()]
    return "\n".join(out) + "\n</svg>\n"


def rle(channel):
    out = bytearray()
    for i in range(0, len(channel), 128):
        c = channel[i:i + 128]
        out.append(len(c) - 1)
        out += c
    return bytes(out)


def element(kind, data):
    return kind.encode() + struct.pack(">I", len(data) + 8) + data


def main():
    b, s = big(), small()
    sizes = [(image(s, 1), "is32", "s8mk"), (image(b, 1), "il32", "l8mk"),
             (image(s, 3), "ih32", "h8mk"), (image(b, 4), "it32", "t8mk")]
    out = b""
    for im, rgb_kind, mask_kind in sizes:
        r, g, bl, a = im.split()
        data = b"".join(rle(ch.tobytes()) for ch in (r, g, bl))
        if rgb_kind == "it32":
            data = b"\0\0\0\0" + data
        out += element(rgb_kind, data) + element(mask_kind, a.tobytes())
    (HERE / "resources").mkdir(exist_ok=True)
    (HERE / "resources" / "Arkanoid.icns").write_bytes(b"icns" + struct.pack(">I", len(out) + 8) + out)
    (HERE / "docs" / "icon.svg").write_text(svg(b))
    if len(sys.argv) > 1:
        sheet = Image.new("RGBA", (128 + 48 + 32 + 16 + 64 + 6 * 8, 144), (221, 221, 221, 255))
        x = 8
        for im in (image(b, 4), image(s, 3), image(b, 1), image(s, 1), image(b, 2)):
            sheet.alpha_composite(im, (x, 8))
            x += im.size[0] + 8
        sheet.save(sys.argv[1])


main()
