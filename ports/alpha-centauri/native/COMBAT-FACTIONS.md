# Original airdrop, fanatic, faction and Planet combat phases

`combat_faction_modifiers` is a checked numeric stage for both present actors,
normal flags0 and a non-Probe attacking plan. Its offense input follows the
separate early fungus/Resonance/Empath stages. Its defense input follows basic
defense and precedes terrain/base/sensor. It does not combine those other phases,
derive World facts, detect battle mode or authorize an attack.

Original order: made-airdrop state AND Drop Pods AND nonzero rule applies
(100-penalty)*offense/100. Fanatic applies (100+bonus)*offense/100 only without
psi. SMAX then processes each matching OFFENSE multiplier in faction-record
order as multiplier*offense/100. Non-native attacking factions in psi apply
(100+activePlanet*rulePlanet)*offense/100. Independently SMAX processes matching
DEFENSE multipliers before subsequent defensive terrain/base stages. SMAC does
not process those faction OFFENSE/DEFENSE records. An original multiplier of100
still executes multiplication, even though the display ledger omits its zero
bonus. Caller-supplied arrays retain original matching-record order, up to8.

This retains original multiply-before-divide and signed division toward zero;
the source rewrite's division-before-multiplication expressions are incorrect.
The airdrop penalty is the actual airdrop rule, not the Empath percentage used
by the rewrite. Fanatic is conditional on conventional combat; Planet depends
on actual nonzero attacker faction and psi. Overflow in percentage addition,
Planet multiplication or any signed int32 product rejects before output. Invalid
inputs and output aliases also preserve output. No labels/caches/World/RNG/input
or UI changes occur. Applied bits1/2/4/8/16 describe numeric branches only.

Final host ASan/UBSan and actual independent Linux68040 each pass11,549 controls:
3,430accepted,13invalid,8,106overflow,31,217guards. Output SHA256:
b15231515a2649a07d66bfc2a793e814d9e7290de1c94c6a31aae8a1c288e418.
Both editions/allpsi masks, drop state/ability independence, fanatic, native and
human factions, signed Planet/rules/strengths,0..8 factor counts and int32 limits
are covered. Sequential127/73 multipliers exercise intermediate rounding.

Actual COMPLETE unchanged original battle_compute executes1,536 SMAC plus1,536
SMAX normal returns, including1,536 paired neutral baselines total. Supplied
phase strengths come from the paired original baselines: offense is used directly;
defense is divided by its subsequent fixed terrain/base factor4. The actual owned
stage then runs on BOTH architectures, and factor4 is reapplied afterward.
Ordered numeric ledger and baseline morale match all56 original output bytes.
This is not independent earlier-stage reconstruction, a whole owned battle or
original whole-World/action execution. Actual original basic/morale/prototype/
ability/bitmask/base/Strings_get/CRT and drop-range technology helpers execute;
zero patches, callback stubs, imports or PE startup. Named display/cache/string/
scratch/nearest spans may change; all851,144 SMAC/851,136 SMAX other mapped global
bytes remain unchanged. Label text is not compared.

The initial smaller original1 and host1/BE1 corpora passed; final original2 and
host2/BE2 add the non-round multiplier profile. Initial original1 receipt text
inherited a missile-harness description and was corrected without changing
execution/output, retaining the historical receipts privately. No failed original
or owned numeric comparisons occurred in this candidate.

Two normal Amiga SDK builds reproduce byte-identically:
85f6cdce883cdf2f5fd02b03ab46748e4d7d77baa9dbca0f5fee51ebb39e7003.
BUILD ONLY. Actual AmigaOS, remote final-head CI and World binding remain UNRUN.
No full battle/mode/attack/session authority, input/damage/casualties/AI/upkeep/turn
or full-game playability claim. User-owned watch remains untouched.
