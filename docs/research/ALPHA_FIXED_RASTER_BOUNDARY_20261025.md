# Original projection-to-fixed-raster boundary

Host-only research and bounded owned implementation,25 October2026. No lab was
accessed and none of these changes is wired into the frozen game viewer.

## Executed original boundary

Unchanged original0x63fa80 fixed-basis conversion and0x6404f0 direction preparation
ran for262 matrices:256 prior projection contexts plus six half-way/narrowing
controls. The owned implementation matches all115,280 output bytes, including
all26 direction offsets per matrix. This establishes rounding and coordinate
increments; it does not establish whole-scene placement or asset-axis mapping.

The actual complete clipped object path0x63e950 also ran for eight synthetic
objects, through original fixed-basis preparation, original shade preparation,
original direction preparation, the REAL original0x664000 pixel callback and its
self-modifying address patcher. There are no substitute callbacks/imports.
Cases vary scale1/.5/.125/1.001 and zero/non-zero group seeds. An independently
written fixed seed/position adapter plus the existing owned procedural shade
and raster stages matches all1,536 pixel/depth bytes exactly. Non-normalised
light coefficients are retained. Synthetic geometry and colour tables are
caller-owned; no original asset/code/table is included publicly.

## Immutable arithmetic contract

* Basis coefficients first multiply by256 with a float store. Conversion rounds
  half away from zero, not truncation or ties-to-even. XY basis coefficients
  then narrow/wrap to signed16 bits; Z coefficients retain signed32 bits.
* A group's original local seed is transformed by the floating object basis,
  added to the projected origin (with the original intermediate float stores),
  then multiplied by256 and rounded half away from zero. XY is24.8; Z is shifted
  left8 with explicit32-bit wrapping to become16.16.
* Each raw direction advances those quantised basis increments BEFORE plotting.
  This is not generally equivalent to transforming each decoded absolute voxel
  in floating point then truncating. Normals use the existing11-bit stream
  index and retain the qualified2048-entry shade lookup.
* The raw-coordinate order is explicit. The adapter does not silently infer
  physical axes from fields named x/y/z in an intermediate decoder.
* Grouped CVRs require the exact group seed and boundaries. The existing flattened
  `CvrModel` representation loses these boundaries. Re-deriving a seed from a
  decoded absolute voxel can change rounding; preserving metadata is an
  integration prerequisite, especially for flags10 jump groups.

`cvr_fixed_transform.h` exposes these arithmetic operations. Checked preparation
is transactional; point accumulation uses explicit integer wrapping. The probes
build with the normal SDK. Native and independent big-endian execution remain
UNRUN; this branch is not published as a PR.

## Newly checked source defaults and unresolved final blit

Instruction/data inspection identifies the normal Caviar constructor's near/far
as−65535/+65535 and depth interval0..65535. The normal map camera position is
camera matrix's final row times−10000, so its flat model-origin depth baseline
is37767.5. These defaults are inspected, not claimed as executed constructor
state: its allocator/initialisation path was not stubbed to manufacture proof.

The shared infantry/reactor append offset at0x687ae8 is(-400,200,0). The earlier
frozen renderer implements only its−400 X term; the omitted+200 term remains a
known placement discrepancy. No frozen candidate was altered silently.

Original Caviar blit0x617230 has a256-square private surface, source clipping,
transparent index9 and an optional double-scale/downsample path for flag0x20.
The inspected constructor sets flag1, not0x20; later runtime selection has not
been fully reconstructed. The full blit has not run: its Windows rectangle and
buffer/device dependencies are unresolved, and no replacements were invented.
The normal map's flags1/3 do not request its optional silhouette-centre bit4.

Original VI/rover full draw comparison remains UNRUN. Before in-world integration,
recover/preserve raw grouped geometry seeds, verify decoded model matrix/axis
mapping and true camera/default state, then execute real components through the
original object path and qualify final blit placement. Terrain slope/shadows,
full renderer/default mutations and complete native fidelity remain outside
this bounded proof. Existing frozen87/90/palette/animation/projection heads
remain unchanged.
