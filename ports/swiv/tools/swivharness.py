#!/usr/bin/env python3
"""A small Amiga harness for studying SWIV's own kernel and game code.

Runs the 68000 code from the player's own SWIV disk image (never shipped)
under Unicorn, with just enough of the chipset to get through: a floppy
drive that answers DMA reads with standard AmigaDOS MFM sectors built from
the ADF, the vertical blank interrupt, the beam counter and CIA-A/B ports.
Every custom-chip write is logged, so the game's display set-up (copper
list, bitplane pointers, BPLCON0-2, colours, sprites) can be read off.

  swivharness.py SWIV.adf OUTDIR [--frames N]

Copyright (c) 2026 AmigaChrome Team
"""
import argparse, os, struct, sys
from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_BLOCK, UcError
from unicorn.m68k_const import *

TRACK = 5632
KERNEL_AT = 0xd9400          # 22 sectors: the game's own boot kernel
KBUF = 0x40000               # where the original boot puts it
CHIP = 0x80000
SLOW = 0xc00000              # 512K "slow" (trapdoor) RAM, as an A500 with A501
SLOWSIZE = 0x80000

NAMES = {0x080: 'COP1LCH', 0x082: 'COP1LCL', 0x084: 'COP2LCH', 0x086: 'COP2LCL', 0x088: 'COPJMP1',
         0x08e: 'DIWSTRT', 0x090: 'DIWSTOP', 0x092: 'DDFSTRT', 0x094: 'DDFSTOP', 0x096: 'DMACON',
         0x09a: 'INTENA', 0x09c: 'INTREQ', 0x09e: 'ADKCON', 0x100: 'BPLCON0', 0x102: 'BPLCON1',
         0x104: 'BPLCON2', 0x108: 'BPL1MOD', 0x10a: 'BPL2MOD', 0x020: 'DSKPTH', 0x022: 'DSKPTL',
         0x024: 'DSKLEN', 0x07e: 'DSKSYNC'}
for i in range(6):
    NAMES[0xe0 + 4 * i] = 'BPL%dPTH' % (i + 1); NAMES[0xe2 + 4 * i] = 'BPL%dPTL' % (i + 1)
for i in range(32):
    NAMES[0x180 + 2 * i] = 'COLOR%02d' % i


def mfm_track(adf, track):
    """One track as the drive's DMA would deliver it after the sync word."""
    def odd_even(longs):
        return [(v >> 1) & 0x55555555 for v in longs] + [v & 0x55555555 for v in longs]
    words = []
    for s in range(11):
        data = adf[track * TRACK + s * 512: track * TRACK + (s + 1) * 512]
        info = 0xff000000 | track << 16 | s << 8 | (11 - s)
        hdr = odd_even([info]) + odd_even([0, 0, 0, 0])
        hsum = 0
        for v in hdr: hsum ^= v
        dl = list(struct.unpack('>128L', data))
        body = odd_even(dl)
        dsum = 0
        for v in body: dsum ^= v
        longs = hdr + odd_even([hsum & 0x55555555]) + odd_even([dsum & 0x55555555]) + body
        sector = [0xAAAA, 0xAAAA, 0x4489, 0x4489]
        prev = 1  # last bit of 0x4489
        for v in longs:            # data bits sit on even positions
            w = 0
            for b in range(31, -1, -1):
                if b & 1:              # clock bit: set between two zero data bits
                    bit = 1 if (prev == 0 and not v >> (b - 1) & 1) else 0
                else:
                    bit = prev = v >> b & 1
                w = w << 1 | bit
            sector += [w >> 16, w & 0xffff]
        words += sector
    words += [0xAAAA] * 300
    return words[2:]   # the DMA starts just after the first sync word


class Harness:
    def __init__(self, adf, out):
        self.adf, self.out = adf, out
        self.mu = mu = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
        mu.ctl_set_cpu_model(UC_CPU_M68K_M68000)
        mu.mem_map(0, CHIP)
        mu.mem_map(SLOW, SLOWSIZE)
        mu.mem_map(0xf80000, 0x80000)                  # ROM space reads as zero
        mu.mmio_map(0xdff000, 0x1000, self.cread, None, self.cwrite, None)
        mu.mmio_map(0xbfd000, 0x2000, self.ciaread, None, self.ciawrite, None)
        self.regs = {}
        self.intena = self.intreq = 0
        self.dmacon = 0
        self.dsklen_prev = 0
        self.cyl, self.side, self.prb = 0, 0, 0xff
        self.cycles = 0
        self.frame = 0
        self.log = open(os.path.join(out, 'custom.log'), 'w')
        self.pc = 0
        self.blits = 0
        self.copstate = {}

    # ---- custom chips ----
    def cread(self, uc, off, size, ud):
        if size == 4:
            return self.cread(uc, off, 2, ud) << 16 | self.cread(uc, off + 2, 2, ud)
        if size == 1:
            w = self.cread(uc, off & ~1, 2, ud)
            return w & 0xff if off & 1 else w >> 8
        if off == 0x002: return self.dmacon
        if off == 0x004: return (self.vpos() >> 8) & 1
        if off == 0x006: return (self.vpos() & 0xff) << 8 | (self.cycles // 4) & 0xff
        if off == 0x01c: return self.intena
        if off == 0x01e: return self.intreq
        if off == 0x00a or off == 0x00c: return 0          # mouse/joystick counters
        if off == 0x016: return 0xff00                    # POTGOR: buttons up
        if off == 0x010: return 0x1100                    # ADKCONR
        if off == 0x01a: return 0x8000                    # DSKBYTR: byte ready
        return 0

    def vpos(self):
        return (self.cycles // 227) % 313

    def cwrite(self, uc, off, size, value, ud):
        if size == 4:
            self.cwrite(uc, off, 2, value >> 16, ud)
            self.cwrite(uc, off + 2, 2, value & 0xffff, ud)
            return
        if size == 1:                       # byte writes land on the word
            value = value << 8 if not off & 1 else value
            off &= ~1
        self.regs[off] = value
        name = NAMES.get(off, '%03x' % off)
        if off not in (0x09c, 0x020, 0x022) and not 0x040 <= off <= 0x074:
            self.log.write('f%05d pc=%06x %s=%04x\n' % (self.frame, self.mu.reg_read(UC_M68K_REG_PC), name, value))
        if off == 0x09a:
            self.intena = (self.intena | value & 0x7fff) if value & 0x8000 else (self.intena & ~value)
        elif off == 0x09c:
            self.intreq = (self.intreq | value & 0x7fff) if value & 0x8000 else (self.intreq & ~value)
        elif off == 0x096:
            self.dmacon = (self.dmacon | value & 0x7fff) if value & 0x8000 else (self.dmacon & ~value)
        elif off == 0x058:
            self.blit(value)
        elif off == 0x024:
            if value & 0x8000 and self.dsklen_prev & 0x8000 and not value & 0x4000:
                self.disk_dma(value & 0x3fff)
            self.dsklen_prev = value

    def rd16(self, a):
        a &= 0x1ffffe
        return struct.unpack('>H', self.mu.mem_read(a, 2))[0] if a < CHIP else 0

    def blit(self, bltsize):
        """Area-mode blitter: four channels, shifts, masks, modulos,
        descending mode and minterms. Line mode and fill are logged."""
        r = self.regs
        con0, con1 = r.get(0x040, 0), r.get(0x042, 0)
        h = bltsize >> 6 or 1024
        w = bltsize & 63 or 64
        if con1 & 1:
            self.log.write('f%05d blit LINE mode (not drawn) con0=%04x con1=%04x\n' % (self.frame, con0, con1))
            self.intreq |= 0x0040
            return
        pa = (r.get(0x050, 0) << 16 | r.get(0x052, 0)) & 0x1ffffe
        pb = (r.get(0x04c, 0) << 16 | r.get(0x04e, 0)) & 0x1ffffe
        pcc = (r.get(0x048, 0) << 16 | r.get(0x04a, 0)) & 0x1ffffe
        pd = (r.get(0x054, 0) << 16 | r.get(0x056, 0)) & 0x1ffffe
        ma, mb, mc, md = (struct.unpack('>h', struct.pack('>H', r.get(o, 0)))[0] & ~1 for o in (0x064, 0x062, 0x060, 0x066))
        use_a, use_b, use_c, use_d = con0 >> 11 & 1, con0 >> 10 & 1, con0 >> 9 & 1, con0 >> 8 & 1
        ash, bsh = con0 >> 12, con1 >> 12
        mint = con0 & 0xff
        desc = con1 & 2
        step = -2 if desc else 2
        fwm, lwm = r.get(0x044, 0xffff), r.get(0x046, 0xffff)
        adat, bdat, cdat = r.get(0x074, 0), r.get(0x072, 0), r.get(0x070, 0)
        aold = bold = 0
        fill = con1 & 0x18
        out = []
        for y in range(h):
            fc = 0
            for x in range(w):
                if use_a: adat = self.rd16(pa); pa += step
                if use_b: bdat = self.rd16(pb); pb += step
                if use_c: cdat = self.rd16(pcc); pcc += step
                am = adat
                if x == 0: am &= fwm
                if x == w - 1: am &= lwm
                if desc:
                    a = ((am << 16 | aold) >> (16 - ash)) & 0xffff if ash else am
                    b = ((bdat << 16 | bold) >> (16 - bsh)) & 0xffff if bsh else bdat
                else:
                    a = ((aold << 16 | am) >> ash) & 0xffff
                    b = ((bold << 16 | bdat) >> bsh) & 0xffff
                aold, bold = am, bdat
                d = 0
                for bit in range(8):
                    if mint >> bit & 1:
                        ta = a if bit & 4 else ~a
                        tb = b if bit & 2 else ~b
                        tc = cdat if bit & 1 else ~cdat
                        d |= ta & tb & tc
                d &= 0xffff
                if use_d:
                    if pd < CHIP:
                        out.append((pd, d))
                    pd += step
            if use_a: pa += -ma if desc else ma
            if use_b: pb += -mb if desc else mb
            if use_c: pcc += -mc if desc else mc
            if use_d: pd += -md if desc else md
        for a, v in out:
            self.mu.mem_write(a, struct.pack('>H', v))
        if fill:
            self.log.write('f%05d blit FILL requested (not done)\n' % self.frame)
        for o, v in ((0x050, pa), (0x04c, pb), (0x048, pcc), (0x054, pd)):
            self.regs[o], self.regs[o + 2] = v >> 16 & 0xffff, v & 0xffff
        self.blits += 1
        self.intreq |= 0x0040

    def copper(self):
        """Runs the copper list once at the top of the frame: MOVEs land in
        the registers (so the display state can be read), WAITs are ignored
        apart from being recorded, and INTREQ writes raise the interrupt."""
        if not (self.dmacon & 0x0280) == 0x0280:
            return
        pc = (self.regs.get(0x080, 0) << 16 | self.regs.get(0x082, 0)) & 0x1ffffe
        state = {}
        for _ in range(2000):
            if pc >= CHIP: break
            a, b = struct.unpack('>HH', self.mu.mem_read(pc, 4)); pc += 4
            if a & 1:
                if a == 0xffff and b == 0xfffe: break
                state.setdefault('waits', []).append(a >> 8)
                continue
            reg = a & 0x1fe
            if reg == 0x09c:
                self.intreq = (self.intreq | b & 0x7fff) if b & 0x8000 else (self.intreq & ~b)
            elif reg == 0x088:
                pc = (self.regs.get(0x080, 0) << 16 | self.regs.get(0x082, 0)) & 0x1ffffe
                continue
            if reg not in state:          # the first value of a frame's line 0 setup
                state[reg] = b
        self.copstate = state

    def dump_frame(self, path):
        """The display as the copper last set it up, for nativertg_frame."""
        st = self.copstate
        con0 = st.get(0x100, self.regs.get(0x100, 0))
        depth = con0 >> 12 & 7
        ddf = (st.get(0x094, 0xd0) - st.get(0x092, 0x38)) // 8 + 1
        width = ddf * 16
        stride = width // 8 + (st.get(0x108, 0) if st.get(0x108, 0) < 0x8000 else st.get(0x108, 0) - 0x10000)
        height = 256
        cols = [st.get(0x180 + 2 * i, 0) for i in range(32)]
        out = [b'NRTG', struct.pack('>6H', width, height, depth, stride, con0 >> 10 & 1, st.get(0x104, 0) >> 6 & 1),
               struct.pack('>32H', *cols)]
        for i in range(depth):
            p = (st.get(0xe0 + 4 * i, 0) << 16 | st.get(0xe2 + 4 * i, 0)) & 0x1ffffe
            out.append(bytes(self.mu.mem_read(p, stride * height)))
        open(path, 'wb').write(b''.join(out))

    def disk_dma(self, nwords):
        ptr = (self.regs.get(0x020, 0) << 16 | self.regs.get(0x022, 0)) & 0xfffffe
        track = self.cyl * 2 + (0 if self.prb & 4 else 1)
        words = mfm_track(self.adf, track)
        words = (words * 2)[:nwords]
        self.mu.mem_write(ptr, struct.pack('>%dH' % len(words), *words))
        self.log.write('f%05d disk read track %d (cyl %d head %d) %d words -> %06x\n'
                       % (self.frame, track, self.cyl, track & 1, nwords, ptr))
        self.intreq |= 0x0002

    # ---- CIAs ----
    def ciaread(self, uc, off, size, ud):
        addr = 0xbfd000 + off
        if addr == 0xbfe001:
            v = 0xc0 | 0x08                 # fire buttons up, write-protected
            sel = not self.prb & 0x08          # DF0 selected (active low)
            if sel:
                if self.cyl == 0: pass
                else: v |= 0x10
                v |= 0x04                   # disk present
            else:
                v |= 0x30 | 0x04
            return v
        if addr == 0xbfd100: return self.prb
        if addr == 0xbfec01: return 0xff
        if addr in (0xbfed01, 0xbfdd00): return 0
        return 0xff

    def ciawrite(self, uc, off, size, value, ud):
        addr = 0xbfd000 + off
        if addr == 0xbfd100:
            self.log.write('f%05d pc=%06x CIAB-PRB=%02x cyl %d\n' % (self.frame, self.mu.reg_read(UC_M68K_REG_PC), value, self.cyl))
            if (self.prb & 1) and not (value & 1):     # STEP pulse, falling edge
                if value & 2: self.cyl = max(0, self.cyl - 1)
                else: self.cyl = min(79, self.cyl + 1)
            self.prb = value

    # ---- CPU ----
    def exception(self, vector):
        mu = self.mu
        sr = mu.reg_read(UC_M68K_REG_SR)
        pc = mu.reg_read(UC_M68K_REG_PC)
        if not sr & 0x2000:
            self.usp = mu.reg_read(UC_M68K_REG_A7)
            mu.reg_write(UC_M68K_REG_A7, self.ssp)
        sp = mu.reg_read(UC_M68K_REG_A7) - 6
        mu.mem_write(sp, struct.pack('>HL', sr, pc))
        mu.reg_write(UC_M68K_REG_A7, sp)
        mu.reg_write(UC_M68K_REG_SR, sr | 0x2000)
        mu.reg_write(UC_M68K_REG_PC, struct.unpack('>L', mu.mem_read(vector, 4))[0])

    def interrupt(self, level, vector):
        mu = self.mu
        sr = mu.reg_read(UC_M68K_REG_SR)
        if (sr >> 8) & 7 >= level: return False
        pc = mu.reg_read(UC_M68K_REG_PC)
        if not sr & 0x2000:
            usp = mu.reg_read(UC_M68K_REG_A7)
            self.usp = usp
            ssp = getattr(self, 'ssp', 0x7fff0)
            mu.reg_write(UC_M68K_REG_A7, ssp)
        sp = mu.reg_read(UC_M68K_REG_A7) - 6
        mu.mem_write(sp, struct.pack('>HL', sr, pc))
        mu.reg_write(UC_M68K_REG_A7, sp)
        mu.reg_write(UC_M68K_REG_SR, (sr & 0xf8ff) | 0x2000 | level << 8)
        target = struct.unpack('>L', mu.mem_read(vector, 4))[0]
        mu.reg_write(UC_M68K_REG_PC, target)
        return True

    def run(self, start, frames):
        mu = self.mu
        mu.reg_write(UC_M68K_REG_PC, start)
        per_frame = 7000          # instructions per PAL frame, roughly 7 MHz / 50 / 20
        # Stop only at the start of a translated block: a stop inside a block
        # (Unicorn's own count, or emu_stop from a per-instruction hook) can
        # lose the lazily kept condition codes or replay instructions, which
        # showed up as a corrupt unpack. Blocks are short, so the frame
        # length stays close enough; instructions are counted as bytes / 3.
        self.left = 0
        def tick(uc, addr, size, ud):
            self.left -= max(1, size // 3)
            if self.left <= 0:
                uc.emu_stop()
        mu.hook_add(UC_HOOK_BLOCK, tick)
        used = 0
        while self.frame < frames:
            pc = mu.reg_read(UC_M68K_REG_PC)
            self.left = per_frame - used
            try:
                mu.emu_start(pc, 0xffffffff)
                used = per_frame - max(0, self.left)
                if self.left > 0:           # STOP or an end address: idle out the frame
                    used = per_frame
            except UcError as e:
                used = per_frame - max(0, self.left) + 20
                pc = mu.reg_read(UC_M68K_REG_PC)
                op = struct.unpack('>H', mu.mem_read(pc, 2))[0]
                if op & 0xfff0 == 0x4e40:          # TRAP #n
                    mu.reg_write(UC_M68K_REG_PC, pc + 2)
                    self.exception(0x80 + 4 * (op & 15))
                elif op & 0xf000 in (0xf000, 0xa000):    # line F / line A
                    vec = 0x2c if op & 0xf000 == 0xf000 else 0x28
                    if not struct.unpack('>L', mu.mem_read(vec, 4))[0]:
                        print('line %s %04x at %06x with no handler' % ('F' if vec == 0x2c else 'A', op, pc)); break
                    self.exception(vec)
                elif op == 0x4e73:                  # RTE (68000 frame)
                    sp = mu.reg_read(UC_M68K_REG_A7)
                    sr, npc = struct.unpack('>HL', mu.mem_read(sp, 6))
                    sp += 6
                    if not sr & 0x2000:
                        self.ssp = sp
                        sp = getattr(self, 'usp', sp)
                    mu.reg_write(UC_M68K_REG_A7, sp)
                    mu.reg_write(UC_M68K_REG_SR, sr)
                    mu.reg_write(UC_M68K_REG_PC, npc)
                else:
                    print('stopped at %06x op %04x: %s' % (pc, op, e)); break
            if used < per_frame:
                continue
            used = 0
            self.cycles += per_frame * 8
            self.frame += 1
            self.intreq |= 0x0020           # vertical blank
            self.copper()
            if self.intena & 0x4000:
                if self.intena & self.intreq & 0x0780:
                    self.interrupt(4, 0x70)
                elif self.intena & self.intreq & 0x0070:
                    self.interrupt(3, 0x6c)
                elif self.intena & self.intreq & 0x0007:
                    self.interrupt(1, 0x64)
        self.log.flush()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('adf'); ap.add_argument('out'); ap.add_argument('--frames', type=int, default=500)
    ap.add_argument('--dump', action='store_true', help='write the last frame as frame.nrtg for nativertg_frame')
    a = ap.parse_args()
    adf = open(a.adf, 'rb').read()
    os.makedirs(a.out, exist_ok=True)
    h = Harness(adf, a.out)
    mu = h.mu
    boot = adf[:1024]
    k = adf[KERNEL_AT:KERNEL_AT + 22 * 512]
    mu.mem_write(KBUF, k)
    # the memory list the original boot builds with exec's TypeOfMem
    tab = struct.pack('>HLL', 3, 0x100, CHIP - 0x100) + struct.pack('>HLL', 5, SLOW, SLOWSIZE) + struct.pack('>H', 0x8001)
    mu.mem_write(KBUF + 0x1da, tab)
    # the routine the disk's boot block leaves at $90 (the kernel jumps to $be)
    mu.mem_write(0x90, boot[0x42:0x42 + 0x70])
    mu.reg_write(UC_M68K_REG_SR, 0x2700)
    mu.reg_write(UC_M68K_REG_A7, KBUF + 0x70)
    h.ssp = KBUF + 0x70
    # kernel entry after the trap to supervisor: find memory, copy itself, jump
    h.run(KBUF + 0x8e, a.frames)
    open(os.path.join(a.out, 'chip.bin'), 'wb').write(bytes(mu.mem_read(0, CHIP)))
    open(os.path.join(a.out, 'slow.bin'), 'wb').write(bytes(mu.mem_read(SLOW, SLOWSIZE)))
    if a.dump:
        h.dump_frame(os.path.join(a.out, 'frame.nrtg'))
    print('frames', h.frame, 'pc %06x' % mu.reg_read(UC_M68K_REG_PC), 'cyl', h.cyl)


if __name__ == '__main__':
    main()
