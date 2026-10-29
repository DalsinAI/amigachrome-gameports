# Owned World psi ability phases

Read-only `world_psi_abilities` derives actual actor/prototype components and
edition-specific psi classification, Empath, Trance and opposing Dissociative
facts. The caller supplies separate early offense (before Resonance/Empath) and
late defense (after terrain/base/sensor, before Trance/Resonance). These are not
one shared incoming battle stage. Normal flags0 and a non-Probe attacker are the
supported domain; this API supplies no live battle or legal attack authority.

SMAC psi classification requires a nonmissile attacking chassis. SMAX permits
that same case plus either prototype ID15, even when the attacker is a missile.
Raw negative attacker weapon and defender armor set mask1 and mask2 respectively.
SMAC abilities are raw prototype bits. SMAX custom prototypes additionally use
the design owner's FREEABIL grants with actual prerequisite technology, not the
actor owner's grants. Opposing Dissociative suppresses Empath/Trance only in SMAX.
The frozen original psi arithmetic retains its original phase and rounding order.
Component validation reuses saved_prototype_speed only as validation; its returned
speed is not a strength or damage input. Invalid facts, aliases and overflow leave
output unchanged. No World, cache, label, RNG or UI writes occur.

Host ASan/UBSan and actual independent Linux68040 each execute71 synthetic Worlds,
17,253 queries and1,207 guards with identical output SHA256
ccb3f04bdfbc60a2ffbdf14392e9f32dfcf97667f9ffd2376578b373f3843356.
Cases cover both editions, all chassis, all psi masks, missile/ID15 combinations,
raw and granted abilities, opposing suppression, prerequisites, Probe holds,
signed phase inputs and aliases. The same binaries inspect the three unchanged
owner tutorial files:438 queries and51 guards per architecture, output
648477c4ea0ef2bd1e116d3fc7733118ed4befac8a057a3a5c979011ee8b0a3b.

Additional private actual COMPLETE unchanged original battle_compute returns:
48 SMAC and48 SMAX calls, including paired neutral baselines, exercise missile,
ID15 and psi masks with Empath/Trance. Zero instruction patches, stubs, imports
or PE startup. All851,144 SMAC/851,136 SMAX mapped global bytes outside named
battle/cache/string/scratch/nearest spans remain unchanged. Independent decoded
classification plus the FROZEN PARENT raw psi probes on both architectures match
all56 original numeric output bytes using the paired original phase inputs.
This is not an original differential of the new World binary, independent earlier
stage reconstruction or complete owned battle execution. Parent original psi
proofs remain separately scoped in PSI-ABILITIES.md.

Two normal Amiga SDK builds are byte-identical, SHA256
e4747733ce369231791201f9f18096c7a89e3d5754a1db6ed66bd7ac5226bc17.
BUILD ONLY: actual AmigaOS execution and remote final-head CI remain UNRUN.
No input/player command, damage/casualties, AI, upkeep, turn or full-game claim.
