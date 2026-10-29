# Original artillery numeric modifiers

`artillery_combat` handles the battle_compute artillery modifier branch for
both present actors. The explicit battle flags must have bit0x2 set and lie in
0..31; bit0x1 separately controls bombardment behavior in earlier calculations.
Flags0x18 skip this numeric stage. No legal artillery order, range, active battle,
psi/basic-mode or session authority is inferred from imported facts.

Land attacker versus sea defender applies the land-gun percentage to offense;
sea attacker versus land defender applies it to defense. These branches exclude
altitude even when the land-gun rule is zero. Every other triad pairing applies
altitude to the higher side, using climate byte>>5 and actual level difference
multiplied by the per-level rule. Equal altitude does nothing. Original signed
int32 difference-percentage/factor/product intermediates are checked, followed
by signed division toward zero. Invalid, alias and overflow leave output intact.
Applied bits1/2/4/8 report attacking gun/defending gun/attacking altitude/defending
altitude numeric branches, not writes to a four-entry UI ledger.

Final host ASan/UBSan and actual independent Linux68040 each pass36,664 controls:
36,107 accepted,12 invalid,545 overflow,73,885 guards. Public cases cover every
triad pair, all8height levels, all16 supported artillery flag combinations,
zero/positive/negative rules, lower climate bits, signed strength extremes and
invalid input/alias preservation. Output SHA256 is identical:
5c9a66b1218b7452373e0389ca5efa8bc5db2e7130e4d14231b4fe0450bcb28d.

Actual COMPLETE unchanged original battle_compute returns in1,215 contexts per
edition (2,430 total, including405 zero-rule baselines each). With each context's
paired original zero-rule incoming strengths/morale, the owned stage reproduces
all56 numeric output bytes and ordered modifier values on both architectures.
This is NOT independent reconstruction of earlier stages or a complete owned
battle execution. No instruction patches, callback stubs, imports or PE startup.
All851,144 SMAC/851,136 SMAX mapped globals outside declared battle/cache/string/
scratch/nearest spans remain unchanged. Supplied bounded synthetic label strings
feed genuine original helpers; label contents are not compared.

The initial candidate incorrectly selected odd bit0x1 contexts as artillery.
Host1/BE1 agreed with that incorrect model, but the actual original comparison
rejected its assumption of an empty prior modifier ledger. Source reinspection
confirmed defender branching uses0x2. Final code/public oracle, original2 matrix,
host2/BE2 and SDK3/4 all use the corrected domain; original1 and earlier binaries
remain historical, unqualified. No validation was skipped or relaxed.

Normal SDK3/4 builds are byte-identical, BUILD ONLY. Actual AmigaOS and remote
final-head CI remain UNRUN. No World binding, cache/label install, input/UI,
attack/damage/casualty/RNG/AI/upkeep/turn or full-game playability claim. Human
watch remains untouched.
