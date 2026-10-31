# FS-UAE runtime test — 30 October 2026

Binary: native GrafX2 2.9, OpenAmigaGCC 16.2.0-open3, 68040 + FPU.
Emulator: FS-UAE 3.1.66. Guest: official AROS 20261030 m68k nightly.
Hardware: A1200/68040/FPU, 2 MiB chip, 8 MiB fast, 64 MiB Z3, 16 MiB UAEGFX.
Guest libraries: exec 51.11, graphics 45.21, datatypes 45.4.
Screen: native owned 640x480x8; P96 monitor enabled.

| Check | Result |
| --- | --- |
| Shell launch, screen, palette, editor UI | Passed |
| PNG skin and bitmap font import through installed DataTypes | Passed |
| input.device → IDCMP keyboard and pointer events | Passed |
| Freehand drawing and keyboard undo | Passed; stroke disappeared on U |
| GIF save and overwrite confirmation | Passed; host decoded 640x480 GIF |
| GIF reopen after undo | Passed; saved stroke restored |
| Quit to Shell | Passed; launcher returned 0 |
| DataTypes PNG save probe | Failed: installed class lacks compatible write support; no false success |
| AmigaOS 3.2.3, AGA, physical AC090, Workbench launch | Not tested |
| Successful OpenPNG output, IFF roundtrip, redo, mode changes | Not runtime-qualified in this run |

The final saved GIF has 307018 pixels of index 0, 111 of index 3 and 71 of
index 4 (640x480 total). Screenshots capture undo, reopened image and Shell exit.
Native test input was injected through the guest input.device, reaching real
IDCMP handling; the host unit tests alone were not used to claim interaction.

Runtime failures found and fixed before this result:

- libnix getcwd(NULL,0) did not allocate a usable buffer; native code now allocates
  an explicit bounded buffer rather than corrupting the heap.
- Legacy NDK varargs wrappers produced malformed tags at GCC16 -O2; native calls
  now use explicit A/tag-list APIs.
- The special-key helper returns zero for ordinary keys; the native loop now
  reports every received key, matching upstream event handling.
- The libm881 startup check assumed one FPU flag; native detection accepts the
  68881, 68882 and integrated 68040 FPU flags.

PNG output needs OpenPNG 47.4+ and its compatible dependencies or another PNG
DataType with DTM_WRITE/DTWM_RAW. The stock tested AROS PNG class cannot qualify
that feature. Its installed z1.library is not interchangeable with OpenPNG's
z.library dependency. PNG read success is not evidence of PNG write support.

This is an AROS smoke-tested test candidate, not an AmigaOS 3.2.3 release.
No ROM, OS image or third-party runtime libraries are included in the package.
