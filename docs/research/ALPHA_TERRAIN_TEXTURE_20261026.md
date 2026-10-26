# Bounded indexed terrain texture pixel stage

This independently written arithmetic adapter closes the visible, opaque,
three-vertex Gouraud texture sampling effect. It does not choose a terrain
material or infer map visibility. The caller supplies three screen-space points,
UV coordinates, signed light and an already prepared indexed texture.

The original RAM-backed Buffer path was executed privately with unchanged owner
instructions (Texture_draw_2 0x61b1f0, edge preparation 0x61fc90, fixed division
0x628ad0, Buffer_get_data 0x5e3373, Buffer_free_data 0x5e34a3). Buffer's real
handle-null branch acquires its supplied RAM storage, then balances its lock
count back to zero. No surface/import/DirectDraw callback was replaced or run.
No original instructions, assets or executable tables are included here.

## Numeric contract

Edges follow the original clockwise chains, horizontal edges are skipped, top
is inclusive and bottom exclusive. X is the ceiling of the edge interpolation.
UV edge values begin half a texel above the input integer coordinate. Their edge
increments truncate toward zero. Surface clipping is half-open. The interpolated
signed light clamps to -4..4, then adds to the indexed texel modulo 256.

The original draw routine sets x87 precision to a 24-bit significand and rounding
toward zero. Its span reciprocal and product are rounded separately in that
mode. Ordinary integer division or the host's default floating arithmetic gives
incorrect UV samples: the first controlled triangle originally differed at
seven pixels. The owned kernel emulates the reciprocal and product rounding
using unsigned integer significands, without changing the caller's floating
control state. This is an explicit pixel-effect adapter: original mutation of
the caller light array and floating-control side effect are not reproduced.

The bounded adapter accepts three vertices, XY within +/-4096, UV 0..255,
textures up to 256x256, surfaces up to 4096x4096 and pitch up to65536. Any sampled
UV outside the supplied texture fails transactionally. Source code performs a
complete validation pass before committing pixels. Destination overlap with
input/surface descriptors or sampled texture storage rejects before writes.
Counterclockwise/degenerate empty coverage is retained, not transformed.

## Evidence and limits

Public tests generate 113 controlled triangles (clipping, signed light/clamp,
unequal slopes, rotation/winding and deterministic random coordinates). All
462,848 pixels match private original execution; the stable public fixture
output including acceptance bytes has SHA256
`deff5030da7efbfc6e385da8b0d9400a24e125f872ec81e78667f053a3de4fd6`.
Eight transactional alias/boundary rejection cases and a partial-record check
also pass host ASan/UBSan and independent big-endian Linux68040 execution.
The normal Amiga SDK builds the same source, but actual AmigaOS execution and
any UI integration remain unrun for this stage.

Four private56x56 raw indexed crops at loader-observed coordinates (1,1),
(58,1), (1,115), (280,79) also matched every pixel across the113contexts.
This tests actual art's indexed byte sampling, not loader palette remapping:
original Buffer_load_pcx, Texture_extract and global material preparation were
not executed. No full world scene is enabled by these controls.

Unclosed next dependencies include actual terrain-type/material/UV and light
selection, fog/dither (Texture_draw_3), transparent and coast-mask composition,
water/coast overlays, map visibility, complete map caller assembly and the
physical screen copy. Existing controlled unit camera/append/projection and
terrain mesh proofs retain their original scopes. No fitted substitute art,
full-map, native UI or restored-game completion claim is made.
