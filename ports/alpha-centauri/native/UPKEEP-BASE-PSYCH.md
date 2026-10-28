# Fresh Base Psych And Drones

`base_psych_policy` implements the complete five-stage `mod_base_psych` and
`adjust_psych` calculation from Thinker default settings at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, with no runtime configuration
knobs. The MIT notice is in `THINKER-LICENSE.txt`. OpenSMACX's recovered
`base_psych` body is incomplete; this implementation is explicitly the pinned
Thinker restoration policy. It includes Thinker's default diminishing returns
for excess psych, forced Paradise Garden talents, and one forced talent each
for Human Genome Project and Clinical Immortality. These intentional defaults
are not claimed to match the original's Clinical Immortality quirk.

The pure result exposes talents, drones, superdrones, efficiency/capture/police
counts, all five intermediate rows, and fresh `can_riot`/`will_riot` decisions.
No saved talent/drone count is read. Bounded population0..127 and pending
TALENT/POLICE[-100,100] are explicit accepted domains. Signed overflow rejects
and leaves the output unchanged.

`world_base_psych_policy` accepts an explicit proposed specialist count and
fresh psych-energy total from the same current assignment/allocation. It
derives current faction bonuses, pending social choices, actual physical
facilities/project owners, difficulty, assimilation and nerve-stapling state,
and fresh support/pacifism. It counts actually owned bases rather than using a
saved faction base-count cache. Content population follows the pinned policy's
global difficulty, while capture drones use the owner's difficulty. Map root
uses the complete recovered OpenSMACX `quick_root` integer algorithm applied
to actual map area, without trusting an imported cache.

Police effects are freshly rebuilt from physical actors' current plans,
chassis, weapons, raw base-game Police2x ability and native/worm-police facts,
plus Self-Aware Colony, then ranked before selecting the allowed strongest
units. The pinned source iterates all actors on the square without a faction
filter; this behavior is preserved and covered by an explicit foreign-unit
fixture, rather than silently importing a diplomacy correction. Sea units and
plans above Recon do not qualify; actual air/Recon behavior is covered too.

The connected API supports the current base-game human profile. It does not
modify the world, set riot/Golden Age state bits, apply faction hypothetical
social-psych statistics, execute riots, allocate workers, or advance a turn.
The caller must use the result with the same fresh assignment/resource stage;
old psych energy or old specialist counts are not qualified facts.

Validation includes an independently authored 15,356-case pure integer model
with every one of 8,192 facility/project flag combinations, all five psych rows,
alias/overflow/failure sentinels and fresh riot predicates. All cases pass
ASan/UBSan and genuine 68040/FPU HUNK execution in FS-UAE/AROS. The 1,474,176-byte
native result is identical to the oracle, SHA-256
`b4b15a9bed57ce4c1ba130c9fcece87f9bc4a3e6175d1d6407effa7eb8ce6b16`.
The connected independent-wire suite passes 82 worlds, 79 accepted, covering
poisoned saved psych/drone/base-count caches, fresh sliders and faction bonuses,
physical facilities/projects, strongest police ordering, sea/air/Recon cases,
actual foreign-unit behavior, sphere immunity, distinct difficulty sources,
proposed specialists, large psych and error/SMAX rejection. Every one of these
82 worlds also passed genuine native execution with per-case RC0 and complete
16,447-byte aggregate output identical to the independent oracle, SHA-256
`d33153be6e90c8ca3767170160809b53a3e6c85b81549c9b9a76f57e020a310a`.
Four additional native reruns verify corrected pre-v11 wire offsets for the
poisoned saved psych/talent/drone fields; all outputs remain identical and the
receipt records the changed source-world hashes. This supplements rather than
overwrites the original 82-world evidence.

Host checks are `test_upkeep_base_psych.py` and `test_world_base_psych.py`. Native
builds are `build_base_psych_probe.sh` and `build_world_base_psych_probe.sh`.
