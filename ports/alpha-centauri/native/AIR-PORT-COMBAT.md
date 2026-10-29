# Original air/noncombat/port numeric stage

`air_port_combat` applies the conventional normal-combat stages after
sensor/Trance/Resonance and before Pulse/Jammer/Tracking. Incoming strengths,
triads, effective Air Superiority, missile chassis classification, defender raw
weapon/armor, attacker faction and resolved defender base presence stay explicit.
This is not mode detection, World binding, legal attack or live action authority.
The caller must already establish the conventional normal-combat domain.

Both originals apply Air Superiority offense against air only if the defender
lacks that ability; against non-air they apply the ground-strike penalty. The
separate defensive Air Superiority bonus requires both triads air, defender with
the ability and attacker WITHOUT it, and neither chassis missile. These are
original conditions, not the inversions in the source rewrite. Noncombat defense
penalty requires raw weapon0 and armor1, except native-faction0 attack against a
base. In Port offense requires attacker triad NOT sea, defender sea and resolved
base presence; the source rewrite's both-sea condition is not used. Ordering and
multiply-before-signed-divide toward zero match both originals. All int32 factors
and products are checked; errors and aliases preserve output. Applied mask bits
report numeric branches; they do not write the four-entry presentation ledger.

Host ASan/UBSan and actual independent Linux68040 each pass12,474 controls:
11,847 accepted,10 invalid,617 overflow,25,575 guards. Cases cover all triads,
effective Air Superiority combinations, both missile classifications, base/non-
base, raw noncombat facts, native/non-native attacker, negative/positive/zero
percentages, signed strengths/extremes and invalid/alias preservation.

Actual complete unchanged original battle_compute returns in3,456 contexts per
edition (6,912 calls total), including1,152 zero-percentage baselines each.
The owned stage receives that paired original baseline's incoming strengths for
each identical context; its changed-rule strength and ordered modifier output,
composed with the baseline's morale observations, matches all56 numeric bytes
on BOTH host and independent68040. This explicitly does NOT independently
reconstruct earlier battle stages or claim complete owned battle execution.
There are no instruction patches, callback stubs, imports or PE startup. Named
battle/cache/string/nearest/scratch spans may change;851,144 SMAC/851,136 SMAX
other mapped global bytes remain unchanged. Synthetic bounded label strings
are supplied to genuine original helpers; label contents are not compared.

Two normal SDK builds reproduce identically; actual AmigaOS and remote final-
head CI are UNRUN. No World binding, label/cache installation, input/UI, combat
application/damage/casualty, RNG, AI, turn or full-game playability is claimed.
The human watch is untouched.
