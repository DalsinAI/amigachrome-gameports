# Original fungus attack phase and owned binding

The original normal flags0/non-Probe attacker phase precedes Resonance/Empath.
It applies only when the defender tile has fungus item0x20 and climate upper
altitude bits are at least0x40; the attacker is either a prototype below64 with
negative raw weapon offense OR its actor faction owns Pholus Mutagen project16;
and the defender's raw weapon offense is nonnegative. A native actor faction is
not the native-design criterion, and custom prototype64 or higher with a negative
weapon needs Pholus. Both editions behave identically.

The raw API adds incoming offense/2, signed division toward zero, to incoming
offense. It does not multiply by150 first: a valid large result must not be
rejected due to an intermediate multiplication that the original never uses.
Checked addition, invalid facts, Probe plan and output aliases reject without
output changes. This stage reports only numeric strength and applied state.

The World API resolves actual current actor/prototype components, defender map
tile and actual Pholus base owner. Malformed component tables, prototype facts,
coordinates or stale project locations remain refused. Project validation is
conservative even on branches where the original could short-circuit its read.
No mode/attack/session authority, World/cache/label/RNG/UI writes or inferred
runtime defaults are introduced. This is not a complete battle calculation.

Raw host ASan/UBSan and actual independent Linux68040 each pass9,231cases:
8,992accepted,15invalid,224overflow,18,701guards; output
144220dbfa1a0c8974ddcb1ee00495d113c16a14216e7a5b1b272f511ab6b887.
Cases include prototype63/64, signed raw weapons, zero defending weapon, altitude
boundary63/64, fungus absence, distinct factions/Pholus, signed odd inputs and
large valid additions versus overflow. The World suite passes108Worlds,
26,244queries and1,512guards on both architectures, output
668f7ef8282ca203102349571e5ee60d860ebe9bfbe8f1afc3ef583bfed72e25.
The same binaries pass438queries42guards across the three unchanged owner
tutorials, outputd87f5a82f9d78b1e89fa0daa6ab0b537f97031f8b6de9b4eec2415786730f0fe.

Actual complete unchanged original battle_compute executes144 SMAC plus144 SMAX
normal returns, including144 paired fungus-absent baselines total. The paired
baseline supplies offense with the same raw profiles, project owner and altitude.
Actual owned raw probes on both architectures reproduce offense plus the ordered
offensive modifier ledger, all24compared bytes per context, SHA256
1b06e347abe1fbbf06bd69ab068dd7f03e44b67056157f9e193373145aabe61a.
Defensive terrain/ledger and the rest of battle are explicitly outside this
comparison. No new World-binary original differential or complete owned battle
is claimed. Zero original patches/stubs/imports/PE startup; real original helper
closure executes. All851,144 SMAC/851,136 SMAX other mapped global bytes outside
named battle/cache/string/scratch/nearest spans remain unchanged.

Initial generated World test had a whitespace syntax error before execution.
Initial host/BE compilation then caught implicit signed-plan narrowing; explicit
conversion retains negative-plan refusal. Final world-host2/world-be2 qualify
the corrected artifact; raw/original arithmetic was unchanged. Histories remain
private in failure-history.json.

Two normal SDK builds reproduce rawdc2a75a265ec00b33821434406a330be9aab70a4b41b90ba26d94e8d17bd0ca5
and World3851765e992ac882a522be88fe71d5c81742e733a796b10c19642c77e1c28f17.
BUILD ONLY. Actual AmigaOS, remote final-head CI and frontend remain UNRUN.
No input, legal attack, damage/casualty, AI/upkeep/turn or full-game claim.
The human watch remains untouched.
