# Alpha: an independent FS-UAE Linux lab

The real FS-UAE 3.2.35 emulator runs in the Linux work environment without a
Daletop connection. SDL's offscreen driver, EGL surfaceless and Mesa llvmpipe
provide an actual GL framebuffer; the capture helper reads that framebuffer
before forwarding to the real SDL_GL_SwapWindow. It does not substitute drawing,
CPU execution, ROM, or application behavior. Captures are actual running output.

## Reproduce the emulator bootstrap

Download the x86-64 portable archive linked by https://fs-uae.net/download/linux/
and extract it with `tar --no-same-owner`. The tested binary SHA256 is
`b6d7199d181a061fe3bbcbca244d861d8924073d8124dc26d082e537f040fe6a`.
Use a Linux host with Mesa/EGL, a C compiler and Python/Pillow:

```sh
FS_UAE_BINARY=/path/to/FS-UAE/Linux/x86-64/fs-uae \
  bash scripts/run_alpha_fsuae_lab.sh /absolute/path/to/new-proof-folder
```

The script uses a fresh HOME, bundled AROS replacement ROM, A1200, 68040/FPU,
2 MiB Chip and 8 MiB Z2 Fast. It takes three real frames and verifies the observed
CPU/FPU configuration. Its scope is an emulator boot/display check. No application
or game UI qualification is implied. The matching hosted workflow is present,
but run 40371958331 did not start: GitHub reported an account billing lock.
The successful runs documented here were performed locally instead.

## Full AROS desktop and local-player controller

The official AROS build listing at
https://aros.sourceforge.io/cgi-bin/files?lang=en&type=nightly2 supplied
`AROS-20261023-amiga-m68k-boot-iso.zip`. Extract its ISO with Rock Ridge names
into a fresh folder. Its `boot/amiga/aros-rom.bin` and `aros-ext.bin` are the ROM
pair for that same system. Mount the extracted folder as DH0 with label AROS.
The real desktop booted; this is not a commercial AmigaOS installation.

Use A1200, CPU/FPU 68040, 64 MiB motherboard Fast, 2 MiB Chip, **zero Z2 Fast**,
and a 4 MiB UAEGFX Z2 card. Eight MiB Z2 Fast leaves no Z2 aperture for that card;
an initial configuration produced FS-UAE's too-large-Z2-memory warning. The
corrected positive run used zero Z2 Fast. Selecting a specific RTG desktop mode
has not been qualified by this evidence.

Build the unchanged #114 control with `build_local_player_control.sh` and the
OpenAmigaGCC 16.2.0-open3 x86-64 release. This portable control needs the shipped
C/C++ runtime, not SDL or AmigaOS API headers. Generate `rules.txt` and
`original.SC` using `test_local_player.py`. Mount a fresh DH1 containing those
files and the executable. Use an Amiga Shell stack of 524288 bytes and run:

```text
DH1:alpha-local-player-control DH1:rules.txt DH1:original.SC DH1:native.aclocal
```

The baseline executable stops before main with Alert 0x00068881. The independent
`fsuae_exec_fpu_flags.c` probe observes ExecBase AttnFlags 0xa04f: the internal
68040 FPU flag is set, while the 68881 flag is clear. The shipped libnix
`__fpucheck` only tests the latter. This is a runtime/AROS compatibility problem,
not evidence that the control ran and failed one of its assertions.

## Isolated runtime source fix

`toolchain/libnix-fpu40-check.patch` changes the check to accept the original
AFF_68881 flag **or** AFF_FPU40. The CPU guard, no-FPU Alert and FPCR initialization
remain. It is a source rebuild, not a disabled check or a replacement game API.

For the bounded experiment, apply it to the exact source from the open3 source
release, compile `__fpucheck.c` with `-m68040 -m68881 -O2`, and replace that member
in **a disposable extracted compiler's**
`m68k-amigaos/libnix/lib/libm020/libnix.a`. Preserve the original archive. Do not
change the user's installed compiler. The rebuild of the original source's text
matched the released object before applying the patch. Re-linking changed only
three instruction bytes in the 166564-byte control executable; all controller
source files remained unchanged. This does not qualify other compiler multilibs.

On a fresh DH1 output slot the corrected executable completed all 145 checks,
returned zero and produced a 144918-byte checkpoint with SHA256
`843665734047b513c4900a2fae0dc3acc714d3ad135ca3b55537c30e1793661a`, identical to
the host's entire checkpoint. This covers the existing bounded movement,
facility confirmation, mixed save/reload, corruption/alias rejection and
preserved exhaustion controls. It is not a full campaign test.

The **same rebuilt executable** on a 68LC040/no-FPU negative machine observed
AttnFlags 0x200f, retained Alert 0x00068881 before main and created no checkpoint.
The source fix therefore did not simply allow a missing FPU to proceed.

Exact binaries, ROMs, source releases, captures and scope are recorded in
`ports/alpha-centauri/native/FS-UAE-EVIDENCE-20261026.json`. No OS image or game
asset is committed. The actual game UI, its SDL/OpenGPU runtime and owned game
assets still need staging and testing. AROS m68k execution does not replace the
remaining AmigaOS 3.2.3 qualification gate.
