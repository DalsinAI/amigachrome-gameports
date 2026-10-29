# Original psi ability arithmetic

`psi_abilities` covers two independent numeric phases of normal battle_compute
with both actors present and a non-Probe attacking plan: early offense before
Resonance/Empath, and late defense after terrain/base/sensor but before Trance/
Resonance. Those separate phase inputs are explicit. This is not a complete
battle pipeline, mode detection or live battle/attack/session authority.

Psi mask1 denotes the attacker's psi weapon; mask2 the defender's psi armor.
Both originals apply Empath only under mask2, unlike the source rewrite's broad
any-psi condition. Trance requires mask1. SMAX applies25percent Resonance offense
for weapon12/13 under mask2 before Empath; armor11/13 under mask1 adds25percent
after Trance. These bonuses do not exist in SMAC. SMAX defender Dissociative
suppresses Empath, attacker Dissociative suppresses Trance; Resonance remains.
SMAC has no such suppression checks. The attacking Probe-plan11 whole-battle
shortcut is outside this API and rejects before output. Edition component bounds,
psi mask and plan bounds validate before checked signed int32 multiplication
and division toward zero. Invalid, alias and overflow preserve output. Applied
bits1/2/4/8 describe numeric branches only, not UI modifier ledger writes.

Host ASan/UBSan and actual independent Linux68040 each pass18,231 controls:
17,219accepted,11invalid,1,001overflow,37,474guards. Both output SHA256:
892314e93f00f9ae91b13492a80070ac979d9cc36772e4d87811d5ccbd69b85a.
Public cases cover both editions/allpsi masks, ordinary/Resonance/Pulse component
ordinals, raw effect observations and suppression combinations, zero/positive/
negative rules, signed strengths/extremes, Probe and invalid/alias refusals.

Actual COMPLETE unchanged original battle_compute returns in192 SMAC and2,304
SMAX contexts (2,496 total, including624 neutral baselines). Each paired original
baseline disables abilities/percentages and substitutes ordinary component IDs
while preserving raw ratings and actual psi classification. That baseline supplies
the two phase strengths and morale observations. Actual owned output plus ordered
numeric modifiers matches all56 original output bytes on BOTH architectures.
This is not independent reconstruction of earlier stages, full owned battle
execution or original whole-World/action execution. Actual raw basic/psi/ability/
bitmask/base/Strings_get/CRT and abil_index helpers run unchanged; no instruction
patches, fake callbacks, imports or PE startup. Bounded supplied label strings
include the Empath ability label. Label contents are not compared; named battle/
cache/string/nearest/scratch spans may change. All851,144 SMAC/851,136 SMAX other
mapped global bytes remain unchanged.

Two normal SDK builds reproduce byte-identically. BUILD ONLY; actual AmigaOS,
remote final-head CI and World binding remain UNRUN. No cache/label installation,
input/UI, legal attack/damage/casualty/RNG/AI/upkeep/turn or full-game playability
claim. The human watch remains untouched.
