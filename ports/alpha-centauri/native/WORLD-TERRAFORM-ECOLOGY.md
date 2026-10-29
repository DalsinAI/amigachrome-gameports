# Fresh owned terraforming ecology

`world_terraform_ecology` returns raw terraforming damage, the valid-radius mask
and 21 individual contributions without changing the owned world. It reads actual
tile items, physical facility bits 13/14 and the current base `worked_tiles` mask.
Saved damage, mineral or nutrient intake, queued facilities, visibility and
worker-availability flags are not inputs. The caller still needs the original
worker-allocation stage to establish a freshly selected worked mask.

The original algorithm counts nine improvement bits (`0x1c8805c`), doubles that
count for worked tiles, adds eight for a borehole, six for an Echelon Mirror and
four for a condenser, then subtracts one for a forest. Either Tree Farm or Hybrid
Forest halves each signed tile contribution toward zero. Both facilities zero
it. The sum can be negative. Dividing the sum or clamping negative forests would
change original behavior. The later mineral stage applies a separate arithmetic
right shift by three; it has its own signed-input extension and qualification.

All first 21 radius positions participate when in bounds, including unworked,
unrevealed or otherwise unavailable squares. Horizontal wrapping is a single
width adjustment on cylindrical maps; flat maps clip. The centre follows the
supplied mask. The earlier original worker stage may force it worked; this query
does not invent that mutation. Narrow cylindrical maps retain repeated wrapped
positions instead of deduplicating them. No runtime edition switch is necessary
for this shared algorithm, which was checked independently in both originals.

Output must be disjoint from the world object, original container and every
decoded storage region. Invalid worlds, base IDs or coordinates leave output
unchanged. Every contribution is bounded, so the 21-term sum fits signed32.
Original-world import validation remains in force. The query needs no stack
repair, live authority, transient upkeep flags, callback or random draw.

Private original evidence executes the unchanged final ecology suffix from each
owner executable, with the corresponding entry frame, original offset tables
and real bitmask/popcount helpers. All 4,096 contexts per edition match an
independent integer model. Each call compares the complete seeded global range
after restoring its sole permitted damage output. This is a suffix observation:
the original prefix, worker selection, specialist handling, satellite intake,
governor and full `base_yield` are not executed. No proprietary bytes or tables
are published.

The owned controls import complete version 7–12 containers, compare every output
against an independent wire interpretation, test partial output aliases and
unchanged whole world/container bytes, and optionally compare the first 512
totals with the actual original observations. Further controls cover tiny and
flat/cylindrical boundaries, ignored queued facilities and poisoned cached
accounting. Private controls include the three unchanged owner tutorials.

Run `test_world_terraform_ecology.py --out NEW_DIRECTORY` for host checks, or add
`--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian controls.
`build_world_terraform_ecology_probe.sh OUTPUT` uses the normal Amiga SDK.
Actual AmigaOS execution is unrun. No player controls, worker assignment,
ecological events, intake pipeline, upkeep, AI or turn progression are added.
