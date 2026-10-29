# Original terrain defence

`terrain_defense` restores the complete `defense_value` calculation at SMAC
515ab0 / SMAX5010c0. `world_terrain_defense` derives its inputs from the defender's
actual tile, actual actor/prototype components, and project16's actual base owner.
An attacker of -1 retains the original absent-attacker inspection path. Other
negative or nonexistent actor IDs reject. Stored tile byte2's owner nibble is
used directly; territory byte7 and geometric base lookup do not replace it.

Both originals return2 without replacing the terrain label for ocean tiles,
stored base bit with owner nibble0..7, or fungus against Planet/Pholus/predefined
negative-weapon attackers. Other paths initially select Rocky, give rocky terrain
+1, fungus +1 or +2 for eligible predefined negative-weapon/Pholus defenders,
and forest +1 only with no attacker or a land attacker. Aircraft defenders do
not receive fungus defence. The numeric result is2..4. The query reports label
replacement (Rocky91, Forest291, Fungus338) or unchanged; it never installs a
pointer or changes a battle/UI cache.

Raw host ASan/UBSan and independent Linux68040 execute40,347 cases:40,323 accepted,
24 invalid and80,718 guards each. Actual complete unchanged original routines
return normally for38,019 coherent actor/prototype contexts per edition. All
851,968 mapped global bytes compare, allowing only the exact expected terrain
label pointer change. The actual original halfstride, outside that range, also
remains unchanged. There are no instruction patches, callback stubs, imports,
PE startup, or proprietary data in these public tests. A supplied synthetic
label-pointer table permits checking the exact ordinal selected; original label
text/rendering is not executed. Both owned architectures match every one of
those original observations. Contexts with inconsistent components for the same
prototype are included in explicit-fact raw controls but excluded from the
original coherent-actor comparison.

World controls use54 synthetic saved worlds across both edition schemas,
4,860 queries and810 guards on each architecture. They vary stored owner bits,
roughness/altitude/items, all9 chassis, actual opposing actors and Pholus base
ownership, and include absent-attacker cases. Source byte buffers and World RNG
remain unchanged. Output aliases into the World, its records/bytes, or unit
tables reject before writes. Malformed project references/components reject
instead of inventing facts. Original owner game files are not needed by the
public controls.

Normal SDK builds are compilation only; AmigaOS execution and remote final-head
CI are UNRUN. This is read-only terrain defence, not base infrastructure,
strength application, legal combat, damage/RNG/casualties, an attack command,
AI, a turn loop, or a playable-game completion claim. No player controls change.
