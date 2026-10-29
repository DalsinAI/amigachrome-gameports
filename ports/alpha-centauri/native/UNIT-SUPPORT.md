# Original unit mineral support query

`world_unit_support` reads owned original bases, pending social SUPPORT, actors,
designs, original ecology and eligible Clean Reactor facts. It returns the free
allowance, supported/maintained counts, exemptions and mineral cost. Its result
is transactional; invalid metadata or signed32 overflow leaves output unchanged.

This is a read-only aggregate, not a turn or the full `base_support` routine.
It does not mutate actor support/pacifism flags, calculate convoy/resource yields,
project social settings, update base accounting, spend RNG or execute upkeep.
It uses pending SUPPORT because that is what the original routines read.
Plan comparison is unsigned-byte `<10`; negative automatic sentinels are excluded.
Original free allowances for SUPPORT -4..3 are 0,0,1,1,2,3,4,max(4,population),
clamped outside that range. Each maintained unit costs two below -3, otherwise one.
Native predefined units on fungus are exempt in shallow water or on land, not deep
water. The expansion also recognizes predefined spore launcher 15 as native.
SMAC Clean Reactor checks raw flags; SMAX additionally resolves eligible grants
using the design owner and actual prerequisite technology.

Host ASan/UBSan controls compare 558 contexts with an independent wire-decoding
Python model, including all 256 plan bytes, home/foreign actors, social bounds,
Clean Reactor grants, ecology and no-write rejection. The shared ability resolver
also retains all 462 speed and 248 movement-price regressions. Three original
owner tutorials match the independent aggregate and remain byte-identical.

Private bounded execution of unchanged original SMAC and SMAX support helpers
matches supported count, maintained count and cost in all 550 accepted contexts.
Those original controls use non-convoy ground/base-marked fixtures and upkeep
stage zero; they do not qualify unobserved convoy, pacifism or full-world effects.
No executable instructions or original assets are shipped in this change.

`build_world_unit_support_probe.sh OUTPUT` builds with the normal Amiga SDK.
The probe accepts `base|expansion RULES GAME` and checks original bytes/RNG stay
unchanged. Actual AmigaOS execution and final independent big-endian CI remain
required before merge. No player commands are exposed.
