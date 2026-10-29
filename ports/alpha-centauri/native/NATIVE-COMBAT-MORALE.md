# Original opponent-specific native morale

This read-only stage supplies `morale_alien` inputs for future real battle
composition. It does not perform combat or legalize an attack.

The checked raw helper accepts current turn, actual actor prototype/state/flags,
opposing faction and its atrocity/tectonic facts, and qualifying adjacent fungus
count. Turn thresholds are 45,90,170,250,330 (final tier6), with monolith state
0x100, Locusts prototype10 and flag bits3..4. Positive opposing factions add one
for any nonzero major atrocities. Only SMAX adds one for nonzero tectonic count.
SMAX Tower19 instead returns clamp(qualifying adjacent fungus count-2,0,6),
overriding all other terms. Base slot19 has no Tower special case. Unit raw
morale and actor faction are not inputs to this original helper.

World binding reads the actual actor, opposing faction, current turn and eight
adjacent tiles with original horizontal wrapping and bounds, excludes the
center, and requires fungus bit0x20 plus altitude bits>=0x40 (ocean shelf).
Version12 tectonic counts are decoded explicitly from their LE array. Version11
with positive opponent fails closed: its file omits the runtime tectonic array,
and this stage does not invent it. This conservative restriction also applies
to Tower queries, even though a fully reconstructed runtime could determine
the result without the opponent bonus. Base editions need no tectonic facts.

Host ASan/UBSan and independent Linux68040/QEMU both execute 18442 raw contexts
(18432 accepted,10 rejected) and 147 saved worlds/11412 queries including three
private original tutorials (10068 accepted,1344 absent-runtime rejects).
All saved bytes and actual World runtime RNG remain unchanged. Public World
workflow controls use144 synthetic containers, not owner files.

Private complete original helpers 0x515d50 (SMAC) and0x501350 (SMAX) execute
9216 contexts each with normal return and no patched instructions/callbacks.
All786432 mapped global bytes remain unchanged; the supplied map half-stride
outside that region is separately checked. Both owned architectures match all
18432 original returned values exactly. Unsafe absent-actor SMAX queries are
excluded; negative/malformed actor/prototype and out-of-range facts reject.

The original referee setup initially omitted mapped half-stride storage and
faulted before any SMAX output; the corrected final harness maps and checks it.
Early builds also exposed a missing header and link dependency, corrected
before the final executed controls. Failed history remains private.

No full basic offense/defense, human/opponent morale composition, terrain/base
combat bonuses, damage, casualties, RNG draws, player input, order execution,
AI, discovery award or turn is restored here. Native builds are build-only;
actual AmigaOS and final external CI remain unrun. All controls stay hidden.
