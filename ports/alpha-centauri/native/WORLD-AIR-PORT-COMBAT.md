# Owned air/noncombat/port binding

`world_air_port_combat` reads actual loaded actors, prototypes, chassis triads and
missile classifications, raw defender weapon/armor, current attacker faction,
resolved defender base presence, current rules and effective Air Superiority.
SMAX custom designs additionally resolve actual design-owner FREEABIL grants
with original prerequisite technology observations; SMAC/predefined designs use
raw flags. Current actor faction does not replace design-owner grant membership.

This API requests a read-only conventional normal-combat inspection. Negative
attacker weapon or defender armor (psi components) reject before output; it does
not infer a legal attack, active battle or live session authority. Actual source
base bits must match an actual base record; stale set bits reject. Chassis missile
classification comes from CHASSIS, not a guessed weapon role or damage/morale.
Incoming strengths remain explicit and the frozen numeric stage performs checked
ordered air/noncombat/port arithmetic. No earlier stages, labels/caches, World,
RNG or action state are installed.

Host ASan/UBSan and actual independent Linux68040 each pass64 synthetic Worlds,
15,552 queries and1,088 guards. Output SHA256 is identical:
cdc9144810c7c0ad2359a44fcb28bce336fc80b2d2d37ab025a4550945f47bf9.
All9 chassis/3triads, missile0/1, effective Air Superiority combinations, ordinary
and noncombat weapon/armor, native/non-native actor faction and physical base
facts are covered. Both editions' None/unknown/known grant prerequisites,
explicit psi-component and stale-base refusals, signed strength extremes and
all World/rules/table output aliases are checked. Complete source containers
and owned RNG stay unchanged. Expected World facts are decoded independently
in Python; these are not original whole-World battle calls. The parent's6,912
complete original paired-baseline observations retain their separate scope.

Two normal SDK builds reproduce byte-identically. BUILD ONLY; native AmigaOS and
remote final-head CI remain UNRUN. No UI/input, attack legality, damage/casualty,
AI, upkeep/turn or full-game playability claim. The human watch is untouched.
