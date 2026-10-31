"""High-resolution shadow of SWIV's chip RAM: the engine change for RTG.

The game keeps drawing exactly as it always has (its own 68000 code, its own
blits into its own bitplanes), so the gameplay is untouched. Beside that,
every bit of chip RAM has a 4x4 block of sub-pixels here. Each blit the game
starts is done twice: once at the original size (the real result the game
reads back) and once here, on the blocks, with the same channels, shifts,
masks and minterm.

Where a blit reads graphics that came from the disk and have not been drawn
here yet, they are upscaled first, in colour: the game draws a bob or a map
tile as one blit per bitplane, so the blits of one such run are gathered,
the planes (and the cut-out mask) are put back together into the coloured
picture, that picture is upscaled once, and each plane's share goes back to
its own blit. Pictures the CPU writes straight to the screen (title, score
panel) are upscaled in colour from the screen itself. The game never sees a
difference.

Copyright (c) 2026 AmigaChrome Team
"""
import numpy as np

S = 4                       # sub-pixels per original pixel, each way
GROUP_MAX = 8               # most blits one bob or tile is drawn with


TEXTURE = 11                # above this many changes around a pixel it is texture


def texture(img):
    """Per pixel: how many colour changes there are in its 3x3 area. Dithered
    ground and noise score high; outlines of craft and lettering score low."""
    h, w = img.shape
    p = np.pad(img, 2, mode='edge')
    t = np.zeros((h, w), np.int16)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            q = p[2 + dy:2 + dy + h, 2 + dx:2 + dx + w]
            t += q != p[2 + dy:2 + dy + h, 3 + dx:3 + dx + w]
            t += q != p[3 + dy:3 + dy + h, 2 + dx:2 + dx + w]
    return t


def epx2(img, smooth=None):
    """Edge-aware 2x of a small-integer image, the same rule as nativertg,
    applied only where `smooth` allows (texture keeps its pixels)."""
    h, w = img.shape
    p = np.pad(img, 1, mode='edge')
    c = p[1:-1, 1:-1]
    n, s = p[:-2, 1:-1], p[2:, 1:-1]
    wv, e = p[1:-1, :-2], p[1:-1, 2:]
    turn = (n != s) & (wv != e)
    if smooth is not None:
        turn &= smooth
    a = np.where(turn & (wv == n), wv, c)
    b = np.where(turn & (n == e), e, c)
    d = np.where(turn & (wv == s), wv, c)
    g = np.where(turn & (s == e), e, c)
    out = np.empty((h * 2, w * 2), img.dtype)
    out[0::2, 0::2], out[0::2, 1::2], out[1::2, 0::2], out[1::2, 1::2] = a, b, d, g
    return out


def upscale(img):
    """4x: smooth outlines twice, leave dithered texture as the artist drew it."""
    smooth = texture(img) <= TEXTURE
    return epx2(epx2(img, smooth), np.repeat(np.repeat(smooth, 2, 0), 2, 1))


def blocks_of(big, wbytes):
    """(h*S, wbytes*8*S) sub-pixels -> (h, wbytes*8, S, S) blocks per bit."""
    h = big.shape[0] // S
    return big.reshape(h, S, wbytes * 8, S).transpose(0, 2, 1, 3)


def sgn(v):
    return (v - 0x10000 if v & 0x8000 else v) & ~1


class Blit:
    """One blit as the game set it up, with the source bytes it read."""
    def __init__(self, r, bltsize):
        self.con0, self.con1 = r.get(0x040, 0), r.get(0x042, 0)
        self.size = bltsize
        self.h = bltsize >> 6 or 1024
        self.w = bltsize & 63 or 64
        self.wb = self.w * 2
        self.desc = bool(self.con1 & 2)
        c0 = self.con0
        self.use = [c0 >> 11 & 1, c0 >> 10 & 1, c0 >> 9 & 1, c0 >> 8 & 1]
        self.ash, self.bsh, self.mint = c0 >> 12, self.con1 >> 12, c0 & 0xff
        self.ptr = [(r.get(o, 0) << 16 | r.get(o + 2, 0)) & 0x1ffffe for o in (0x050, 0x04c, 0x048, 0x054)]
        self.mod = [sgn(r.get(o, 0)) for o in (0x064, 0x062, 0x060, 0x066)]
        self.const = [r.get(0x074, 0), r.get(0x072, 0), r.get(0x070, 0)]
        self.fwm, self.lwm = r.get(0x044, 0xffff), r.get(0x046, 0xffff)
        # lowest address of each row, and the step between rows (in blit order)
        self.row0, self.step = [], []
        for ch in range(4):
            if self.desc:
                self.row0.append(self.ptr[ch] - 2 * (self.w - 1))
                self.step.append(-(self.wb + self.mod[ch]))
            else:
                self.row0.append(self.ptr[ch])
                self.step.append(self.wb + self.mod[ch])
        self.key = (self.con0, self.con1, bltsize, tuple(self.mod), self.fwm, self.lwm)
        self.rows = {}

    def addrs(self, ch):
        return [self.row0[ch] + self.step[ch] * y for y in range(self.h)]

    def snapshot(self, mem, ch, n):
        out = np.zeros((self.h, self.wb), np.uint8)
        for y, a in enumerate(self.addrs(ch)):
            if 0 <= a and a + self.wb <= n:
                out[y] = mem[a:a + self.wb]
        return out


class Shadow:
    def __init__(self, chip_bytes):
        self.n = chip_bytes
        # tiles[bit] is the 4x4 block for chip bit `bit` (addr * 8 + 0..7, MSB first)
        self.tiles = np.zeros((chip_bytes * 8, S, S), np.bool_)
        self.mirror = np.zeros(chip_bytes, np.uint8)   # chip RAM as the tiles last saw it
        self.pending = []
        self.blits = self.upscaled = self.groups = 0

    # ---- taking blits from the game ----
    def blit(self, mem, r, bltsize):
        """Before the real blit: note it and the bytes it is about to read."""
        b = Blit(r, bltsize)
        if b.con1 & 1 or not b.use[3]:
            return
        for ch in range(3):
            if b.use[ch]:
                b.rows[ch] = b.snapshot(mem, ch, self.n)
        self.pending.append(b)

    def after_blit(self, mem, r_before, bltsize):
        """After the real blit: keep what it wrote, to mark it as drawn here."""
        if self.pending and self.pending[-1].size == bltsize and 3 not in self.pending[-1].rows:
            self.pending[-1].rows[3] = self.pending[-1].snapshot(mem, 3, self.n)

    # ---- bringing sub-pixels in step with fresh bytes ----
    def put_rows(self, b, ch, big):
        """Write upscaled sub-pixels for the rows of channel `ch` whose bytes
        are new here (rows already drawn at high resolution are kept)."""
        blk = blocks_of(big, b.wb)
        for y, a in enumerate(b.addrs(ch)):
            if not (0 <= a and a + b.wb <= self.n):
                continue
            row = b.rows[ch][y]
            old = self.mirror[a:a + b.wb]
            diff = row != old
            if not diff.any():
                continue
            for i in np.nonzero(diff)[0]:
                self.tiles[(a + i) * 8:(a + i) * 8 + 8] = blk[y, i * 8:i * 8 + 8]
            self.mirror[a:a + b.wb] = row

    def fresh(self, b, ch):
        for y, a in enumerate(b.addrs(ch)):
            if 0 <= a and a + b.wb <= self.n and not np.array_equal(b.rows[ch][y], self.mirror[a:a + b.wb]):
                return True
        return False

    def plain(self, b, ch):
        if self.fresh(b, ch):
            img = np.unpackbits(b.rows[ch], axis=1)
            self.put_rows(b, ch, np.repeat(np.repeat(img, S, 0), S, 1).astype(np.bool_))

    def single(self, b, ch):
        """A source read on its own (a shadow or cut-out mask): upscale it
        as a two-colour picture."""
        if self.fresh(b, ch):
            self.put_rows(b, ch, upscale(np.unpackbits(b.rows[ch], axis=1)).astype(np.bool_))
            self.upscaled += 1

    def refresh_group(self, g):
        """Upscale the sources of one run of blits in colour."""
        first = g[0]
        varying = [ch for ch in (0, 1) if first.use[ch] and len(g) > 1
                   and any(m.ptr[ch] != first.ptr[ch] for m in g)]
        steady = [ch for ch in (0, 1) if first.use[ch] and ch not in varying]
        for ch in varying:
            if not any(self.fresh(m, ch) for m in g):
                continue
            img = np.zeros((first.h, first.wb * 8), np.uint8)
            for k, m in enumerate(g):
                img |= np.unpackbits(m.rows[ch], axis=1) << k
            extra = [c for c in steady if len(varying) == 1]
            for c in extra:                 # the cut-out mask goes with the colours
                img |= np.unpackbits(first.rows[c], axis=1) << len(g)
            big = upscale(img)
            self.upscaled += 1
            for k, m in enumerate(g):
                self.put_rows(m, ch, (big >> k & 1).astype(np.bool_))
            for c in extra:
                self.put_rows(first, c, (big >> len(g) & 1).astype(np.bool_))
        for ch in steady:
            self.single(first, ch)

    def flush(self, keep=0):
        """Do the noted blits at high resolution, a run at a time (keeping
        the last `keep`, which may still grow into a longer run)."""
        i, p = 0, self.pending[:len(self.pending) - keep]
        while i < len(p):
            j = i + 1
            if j < len(p) and p[j].key == p[i].key:
                delta = [p[j].ptr[c] - p[i].ptr[c] for c in range(4)]
                while (j < len(p) and j - i < GROUP_MAX and p[j].key == p[i].key and
                       all(p[j].ptr[c] - p[j - 1].ptr[c] == delta[c] for c in range(4))):
                    j += 1
            g = p[i:j]
            self.groups += 1
            self.refresh_group(g)
            for m in g:
                if m.use[2] and m.ptr[2] != m.ptr[3]:
                    self.single(m, 2)       # C is graphics, not the background being drawn on
                elif m.use[2]:
                    self.plain(m, 2)
                self.run(m)
                if 3 in m.rows:
                    for y, a in enumerate(m.addrs(3)):
                        if 0 <= a and a + m.wb <= self.n:
                            self.mirror[a:a + m.wb] = m.rows[3][y]
            i = j
        self.pending = self.pending[len(p):]

    # ---- the blit, at four times the resolution ----
    def run(self, b):
        nb = b.w * 16
        maskbits = np.ones(nb, np.bool_)
        first, last = (b.w - 1, 0) if b.desc else (0, b.w - 1)
        bits16 = lambda v: np.unpackbits(np.array([v >> 8, v & 0xff], np.uint8)).astype(np.bool_)
        maskbits[first * 16:first * 16 + 16] &= bits16(b.fwm)
        maskbits[last * 16:last * 16 + 16] &= bits16(b.lwm)
        maskbits = maskbits[:, None, None]

        def shifted(t, sh):
            if not sh:
                return t
            out = np.zeros_like(t)
            if b.desc:
                out[:-sh] = t[sh:]
            else:
                out[sh:] = t[:-sh]
            return out

        consts = [np.broadcast_to(np.tile(bits16(b.const[ch]), b.w)[:, None, None], (nb, S, S)) for ch in range(3)]
        for y in range(b.h):
            chans = []
            for ch in range(3):
                if b.use[ch]:
                    s = (b.row0[ch] + b.step[ch] * y) * 8
                    t = self.tiles[s:s + nb] if 0 <= s and s + nb <= len(self.tiles) else np.zeros((nb, S, S), np.bool_)
                else:
                    t = consts[ch]
                if ch == 0:
                    t = shifted(t & maskbits, b.ash)
                elif ch == 1:
                    t = shifted(t, b.bsh)
                chans.append(t)
            a, bb, c = chans
            d = np.zeros((nb, S, S), np.bool_)
            for bit in range(8):
                if b.mint >> bit & 1:
                    d |= (a if bit & 4 else ~a) & (bb if bit & 2 else ~bb) & (c if bit & 1 else ~c)
            s = (b.row0[3] + b.step[3] * y) * 8
            if 0 <= s and s + nb <= len(self.tiles):
                self.tiles[s:s + nb] = d
        self.blits += 1

    # ---- the picture ----
    def compose(self, mem, lines):
        """lines: per display line (regs, plane pointers) from the copper.
        Returns a (lines*S, 320*S) colour index image."""
        self.flush()
        H = len(lines)
        native = np.zeros((H, 320), np.uint8)
        fresh = []
        for y, (regs, ptrs) in enumerate(lines):
            for p in range(regs.get(0x100, 0) >> 12 & 7):
                a = ptrs[p]
                if 0 <= a and a + 40 <= self.n:
                    row = np.asarray(mem[a:a + 40])
                    native[y] |= np.unpackbits(row) << p
                    diff = row != self.mirror[a:a + 40]
                    if diff.any():
                        fresh.append((y, p, a, row, diff))
        if fresh:
            # what the CPU drew straight onto the screen: upscale it in colour
            big = upscale(native)
            for y, p, a, row, diff in fresh:
                blk = blocks_of((big[y * S:(y + 1) * S] >> p & 1).astype(np.bool_), 40)[0]
                for i in np.nonzero(diff)[0]:
                    self.tiles[(a + i) * 8:(a + i) * 8 + 8] = blk[i * 8:i * 8 + 8]
                self.mirror[a:a + 40] = row
        out = np.zeros((H * S, 320 * S), np.uint8)
        for y, (regs, ptrs) in enumerate(lines):
            for p in range(regs.get(0x100, 0) >> 12 & 7):
                a = ptrs[p]
                if 0 <= a and a + 40 <= self.n:
                    t = self.tiles[a * 8:a * 8 + 320]
                    out[y * S:(y + 1) * S] |= t.transpose(1, 0, 2).reshape(S, 320 * S).astype(np.uint8) << p
        return out


def nearest(mem, lines):
    """The same frame without the engine change: each pixel as a 4x4 block."""
    H = len(lines)
    out = np.zeros((H, 320), np.uint8)
    for y, (regs, ptrs) in enumerate(lines):
        for p in range(regs.get(0x100, 0) >> 12 & 7):
            a = ptrs[p]
            if 0 <= a and a + 40 <= len(mem):
                out[y] |= np.unpackbits(np.asarray(mem[a:a + 40])) << p
    return np.repeat(np.repeat(out, S, 0), S, 1)
