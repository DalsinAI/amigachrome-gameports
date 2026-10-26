# Original world terrain lighting and material-dispatch boundary

This child closes ordinary world terrain point lighting. It preserves the
prepared index/fog/keyed kernels and does not enable the map viewer.

## Procedural lighting

The original MapWin_compute_lighting_table470420, quick_root6290e0,
MapWin_get_brighting470550, MapWin_get_point_light470790 and
MapWin_get_alt46fe70 executed unchanged privately. The real height cache and
mathematically generated brightness table were used; no mocked callbacks,
physical surface, original game entry or OS calls ran.

The owned implementation generates the same integer effect from four relative
corners W,N,E,S in1..7. The triangle gradients rotate with the map geometry.
Integer square root and truncating signed division recover each triangle's
brightness. A centre point averages four triangle values with arithmetic right
shift; each shared corner averages eight values from four surrounding tiles.
Negative averages therefore round down, not toward zero. Wrapped longitude is
one-step source wrapping; out-of-map and ocean brightness contributions are0.

All2,401corner combinations /9,604triangle values match the actual original
procedurally generated table, SHA256
`db4f90df0506210d071de241c340f5215e78460a13f260e71664ce1343353930`.
No original executable table bytes are included in the implementation.

The owned query matches all17,280point lights from3,456tiles across the owner's
original Explore, Build and conquer tutorials, on host ASan/UBSan and independent
big-endian68040. This includes ocean points and land/ocean shared corners.
Each tutorial's original map has no special landmark-height/light features;
that fact was checked before executing the private original helper chain.

Public asset-free controls cover72world contexts with an independent Python
oracle: wrap/flat edges, coast neighbours, invalid parity/coordinates, elevation7
and special feature rejection. There are eight invalid-corner controls plus
16output/world-storage alias checks per loaded fixture. The query preserves
its output on unsupported contexts. A fixture-test setup initially referred to
an absent metadata-offset key; the preserved failed run was corrected to the
actual metadata+28 flat-map flag. Production numeric output did not change.

Normal SDK build passes. Native AmigaOS, UI, full-map rendering and final CI
execution remain unrun. All90parent workflow commands are retained, plus two.

## Material selection is not yet closed

Instruction-backed caller evidence identifies a dependency omitted by the old
viewer: at463390 the original tile RAM address is passed to625750, seeding a
private rendering LCG. The material variation request at463e68 calls625770 with
range[0,2). Its update is1664525*state+1013904223 modulo2^32 and uses the low16
bits for scaling. It is not the game's world RNG, a coordinate hash, or the save
footer tick. No original allocation base is silently guessed here.

The save metadata retains historical PC pointer bytes, including the original
map-tile pointer slot. They remain inert evidence, not memory addresses for the
port or proof of the original address after a later reload. The owned lighting
query neither dereferences those bytes nor invents a rendering seed policy.

Original land triangle draw selectors at4641b4..464353 choose prepared texture
objects based on source feature flags, rockiness, transition-mask variant,
moisture and visibility. UV rotation depends on the neighbouring rockiness
transition mask. The original caller's point-light values can now be supplied
exactly where the already-qualified pixel stages expect them; material/UV and
coast dispatch still require their own closed controls.

Next boundaries remain terrain material/transition selection with explicit seed
provenance, exact UV rotation, coast/water masking and layered scene ordering.
No guessed terrain material, fitted unit placement, replacement imagery, viewer
change or full restored-scene claim is made by this stage.
