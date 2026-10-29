# Original five-phase psych proposals

`base_psych` computes talents, drones and superdrones from explicit current
facts. It returns all five original display rows: initial population effects,
psych spending, facilities, police/pacifism, and projects/stapling. Stage1 also
proposes the72 hypothetical social-rating contributions, retaining the other
ten slots in the caller's82-slot array.

The caller supplies the physical facility mask, original project indices,
current population/specialists, capture timer, social ratings, faction rules,
map factor, base count, psych spending and actual ordered garrison. This
component does not extract a garrison or resolve unit abilities. The supplied
double-police fact must already reflect the correct edition's ability rules.
Allied garrison units participate as in the originals. Double-police and
eligible SMAX worm-police units are visited before ordinary police, preserving
stack order within each group. Unit15 retains the original native-unit exception.

SMAC does not apply the SMAX NODRONE faction bonus, WORMPOLICE rule or Brood Pit
police increment. The Clinical Immortality contribution remains two talents;
no manual-inspired patch is substituted. The adjustment order can cancel
talents against drones and superdrones. These are original behaviors, rather
than the modified Thinker psych algorithm or OpenSMACX's incomplete WIP body.

Input bounds and transactional error handling are declared in `base_psych.h`.
Every executed signed32 arithmetic operation is checked. Overlapping facts,
garrison and output storage are rejected before writing. No heap allocation is
used. A bulk adjustment preserves the original loop's result without iterating
billions of times on a large explicit talent value.

## Validation and limits

The synthetic host ASan/UBSan and independent big-endian68040 controls each
execute3121 cases:3080 accepted,32 invalid and9 arithmetic overflow. Every case
checks three facts/output overlaps; nonempty arrays also check garrison/output
overlap. Six direct controls additionally check null/overlapping arrays, zero
units,2048 units, invalid final-array entries and the2049-unit limit.

Private complete original routines execute1536 contexts per edition with
unchanged instructions and genuine has_fac/bitmask/veh_at/stack_fix/has_abil
helpers. All100 returned fields match this implementation on both architectures.
Entire seeded runtime memory is checked against the independent Python model.
The source observations cover population1..25, specialists0..population,
capture/rules/facilities/projects, both upkeep stages, all72 projections,
police ratings-6..4, and0..8 linked garrison units. Targeted cases exercise
the unit15 exception with other units ineligible for police.

Those original runs use raw-ability prototypes0..6 and15 (design owner0), with
stack repair disabled explicitly. They do not exercise the SMAX FREEABIL
grant path or repairing broken stacks. The API accepts already resolved ability
facts; it does not implement either operation. Population0/127 and the2048-unit
bound have synthetic controls, rather than original-code differential coverage.

```sh
python3 ports/alpha-centauri/native/test_base_psych.py --out /tmp/base-psych-host
python3 ports/alpha-centauri/native/test_base_psych.py --out /tmp/base-psych-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Two normal Amiga SDK builds are byte-identical; AmigaOS execution is UNRUN.
Full workflow CI is UNRUN. This is a separate accounting proposal component:
no World binding, session authority, base/faction mutation, upkeep, turn,
player input or save command is added. Full energy calculation, specialist
output/bonuses, production completion, AI/combat/diplomacy and complete games
remain unfinished. No owner data, executable bytes or private receipts are
included here.
