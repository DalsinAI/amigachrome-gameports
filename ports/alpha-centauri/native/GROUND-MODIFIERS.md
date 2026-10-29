# Original conventional ground modifiers

`ground_combat_modifiers` restores the conventional normal-combat (`combat_type0`)
land-attacker branches between earlier faction calculations and terrain/base
multiplication. It applies, in order: mobile in open, rough versus mobile, road,
uphill and downhill. The output still requires terrain/base/sensor and later
ability stages. It is not a final battle result or legal-attack decision.

Both originals grant the mobile-open bonus when the defender's prototype speed
is **less than or equal to** the attacker's. It requires no physical base,
terrain multiplier2, non-rocky terrain and attacker speed above the road rate.
Rough-versus-mobile requires terrain multiplier>2 OR a physical base, and an
attacker faster than the road rate; it does not exclude rocky terrain. The road
percentage is unconditional within this land normal-combat block; no road bit
check is invented. Uphill checks the attacker's Antigrav, downhill the defender's.
Altitude uses the actual climate byte's upper3bits. Percentages multiply before
signed division toward zero. Each original int32 intermediate is checked before
any output write. Applied bits1/2/4/8/16 describe numeric branches, not additions
to the original four-entry UI ledger.

`world_ground_combat_modifiers` requires both actual actors to have land chassis.
It resolves true prototype speed through the qualified source helper, including
actual design-owner FREEABIL prerequisites for Antigrav/Slow in SMAX. It does
not use actor damage/elite speed. Terrain, altitude, roughness and physical-base
facts come from the owned World. A stale set base bit with no matching record
rejects before the original callback path. Incoming strengths remain explicit;
no psi mode, battle/session authority or player action is inferred from a save.

Raw host ASan/UBSan and independent Linux68040 pass14,616 controls each:
10,265 accepted,14 invalid,4,337 overflow rejections,33,583 guards. Both
architectures' actual stage outputs composed with independent terrain/base
multiplication match all56 numeric bytes from COMPLETE unchanged original
battle_compute in2,592 contexts per edition (5,184 total). Those original calls
return normally and execute real strength/morale/prototype speed/ability/base,
sensor and string/CRT helpers. They cover base/open terrain, rough/forest/fungus,
relative heights, equal/slower/faster units, Antigrav and zero/positive/negative
rule profiles. There are no instruction patches, callback stubs, imports or PE
startup. The owned stage is not claimed to execute the complete battle routine.

Original proof checks851,144 SMAC/851,136 SMAX global bytes unchanged; declared
battle/string/cache/nearest-distance/terrain-label/scratch spans may change.
Numeric strengths, ordered modifier counts/values and morale caches are compared;
label string contents are not. Original string helpers receive a bounded supplied
label table and genuine read-only punctuation bytes.

World controls pass59 synthetic worlds,14,337 queries and1,003 guards on each
architecture. All9 chassis, actual map facts, raw and technology-gated Antigrav,
non-land refusal and checked overflow are covered. Input containers and owned RNG
remain unchanged; World/table storage aliases reject before output.

Normal SDK binaries build reproducibly; AmigaOS/native execution and remote
final-head CI remain UNRUN. No input/UI, cache install, applied combat/damage,
casualty, AI, turn or full-game playability claim is added. The watch is untouched.
