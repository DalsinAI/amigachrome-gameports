# Original late defense abilities

`defense_abilities` is the read-only numeric Pulse/Jammer/Tracking stage after
In Port in normal battle_compute. Incoming defense and current effective ability
observations are explicit. It does not calculate preceding stages, resolve an
owned World, install modifier labels/caches, validate attacks or apply combat.

Both originals use the attacker's raw chassis speed>1 for Jammer, with land
triad required. Tracking requires an air attacker. SMAC does not test
Dissociative Wave; SMAX suppresses both abilities when the attacker has it.
SMAX Pulse armor ordinals10/12 require BOTH land triad and chassis speed>1;
the source rewrite's missing parentheses are not followed. Pulse applies before
Jammer and remains active under Dissociative. Armor ordinals are edition bounded
(SMAC0..9, SMAX0..13). Checked int32 multiply occurs before signed division by100
toward zero. Invalid, aliasing and overflow leave output untouched.

Host ASan/UBSan and independent Linux68040 each execute8,877 controls:
8,435 accepted,9 invalid,433 overflow,18,196 preservation/alias guards.
Both actual owned outputs match all56 numeric bytes from COMPLETE unchanged
original battle_compute calls in216 SMAC and648 SMAX contexts. Incoming576,
independent basic offense864 and morale3 are supplied; this is not a whole owned
battle execution. Original functions return normally, no instruction patches,
fake callbacks, imports or PE startup. Named battle/cache/string spans may change;
851,144 SMAC/851,136 SMAX other mapped global bytes stay unchanged. Label strings
are supplied bounded test labels and their contents are not compared.

The initial private exploratory matrix included out-of-edition SMAC armor10/12,
which overlaps another original table, and nonzero earlier road rules. Its
receipts remain as history; only original2's edition-correct armor domain and
zero preceding rules qualify this stage. A Python test generation syntax error
was fixed before compilation; neither production behavior nor tests were relaxed.

Two normal SDK builds are byte-identical. BUILD ONLY; native AmigaOS and remote
final-head CI are UNRUN. No World binding, runtime authority, input/UI, damage,
casualty, RNG, AI, turn or full-game playability claim. Watch remains untouched.
