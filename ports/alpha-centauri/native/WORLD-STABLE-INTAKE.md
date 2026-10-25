# Fresh intake for an existing human worker assignment

`world_stable_intake` composes original satellite intake, fresh worked-tile
nutrient/mineral/energy yields and signed raw terraforming damage from an owned
world. Saved intake and damage totals are never inputs. The result includes
individual tile yields, satellite and tile subtotals, four intake channels,
proposed base flags and explicit outgoing scratch values. The fourth intake
channel starts at zero in this supported branch.

This is a bounded, read-only composition of the directed-human branch. It needs
an already valid assignment of population plus one first-21 radius tiles,
including the centre, with no specialists or automatic governor. The original
availability query must accept every selected non-centre tile. The centre must
contain a base marker for the actual owner. Missing assignments, specialist
handling, selected fungus and stages requiring allocation reject explicitly.
The caller supplies the runtime edition, expansion switch, worker inspection
stage, ControlUpkeep branch and incoming scratch values. They are not inferred
from saved flags or treated as live authority. Only normal worker inspection is
supported. Current technology-dependent fungus caches are not assumed fresh.

Satellite counts are capped by population. Physical Aerospace Complex 29 or an
owned Space Elevator project 27 enables full counts; SMAX additionally checks
owned Cloudbase Academy project 35. The original short-circuit order is retained.
Otherwise counts use `(count + 1) / 2`. Negative counts explicitly reject from
this owned API's safe domain; the original loop itself can produce negative
intake. Half-count INT32_MAX rejects overflow, while a full count is safely
capped. These are project indices, distinct from facility IDs.

Selected tiles use actual original map, rule, resource, technology, facility,
project and faction facts through the existing tile query. A selected ocean tile
sets proposed base flag 0x8000; flags 0x8100 are initially cleared. The original
resource-yield channel-three wrapper resets TerraformReduce after each selected
square, rather than accumulating that scratch across the radius. BaseEnergy
carries the real leaf result. Every subtotal addition is checked before output.
Raw ecology includes all in-bounds first-21 positions, preserving negative forest
contributions and per-tile Tree Farm/Hybrid Forest reductions.

Output must be disjoint from the whole world object, original wire container,
all decoded regions and all supplied rule/stage objects. Failure preserves the
output and every input byte. No base/faction record, RNG, worker mask, session
revision or persistence buffer changes. This adds no player control, allocation,
production completion, upkeep, AI, turn progression or complete-game claim.

Private evidence executes the complete unchanged `base_yield` function in both
owner executables for 64 supported contexts each. Real farm/resource/crop/mine/
energy helper entries are traced, with no replacement callbacks or PE startup.
The complete current base record and seeded global range match an independent
component model; the captured scratch page is not fully independently modeled.
The owned composition compares all four intake channels, worked mask and raw
ecology with those 128 actual observations. This does not qualify the original
governor, AI, specialist or general worker-allocation branches.

Public controls use synthetic complete version 7–12 containers and rules only.
They independently interpret wire records, poison cached accounting, exercise
both editions, satellites, invalid assignments, alias ranges, explicit scratch,
solar branches, ocean flags and checked sum overflow, and compare every result
byte. Private controls add the three unchanged tutorials with their original
rules. All world object, decoded storage and input bytes are checked unchanged.

Run `test_world_stable_intake.py --out NEW_DIRECTORY` for host sanitizers, or add
`--cc m68k-linux-gnu-g++ --m68k-linux` for independent Linux 68040 execution.
`build_world_stable_intake_probe.sh OUTPUT` builds with the normal Amiga SDK.
Actual AmigaOS execution remains unrun and is required before merge or UI use.

Fresh technology-effects integration supersedes the earlier selected/harvested-fungus refusal; see [WORLD-FRESH-TECHNOLOGY-YIELDS.md](WORLD-FRESH-TECHNOLOGY-YIELDS.md). All other stable-domain and mutation exclusions remain.
