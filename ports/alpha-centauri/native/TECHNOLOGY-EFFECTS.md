# Complete callback-free technology effects (local draft, native unrun)

`recompute_technology_effects` implements the complete original `tech_effects`
commerce/fungus stage with checked owned inputs. `world_technology_effects` binds
those inputs to actual decoded ownership, faction definition/bonuses and pending
social economy. It does not read the saved commerce/fungus caches to calculate
the new result. Those caches can therefore be stale without corrupting this
fresh recomputation.

This is a reusable complete effect stage, **not a research award or transaction**.
It does not set ownership, refresh the world/tile records, consume labs, select a
research target, discover a technology, increment a turn or run callbacks.

## Range-alias correction

The frozen first candidate3ef0e6b checked numeric failures but did not reject a
Result stored inside input objects. This separate correction rejects the complete
output range against complete Rules/Facts inputs and, for the world wrapper, the
World object, raw allocation, decoded tile/vehicle/base arrays, all512 prototypes,
and all8 faction/definition arrays. Start, middle, end and before-start partial
overlap reject with bad_state before any write. Address/array-size wrap is also
rejected; zero-length actor arrays and genuinely adjacent output objects remain
valid. Both input bytes and the aliased output region remain unchanged on reject.

Alias controls pass four legacy/current/empty/populated worlds:32 pure aliases,
96 World/Rules aliases,16 valid adjacent outputs,40 boundary/wrap/product controls
and4 disjoint results. Entire World object/decoded/raw state and Rules/Facts
snapshots remain unchanged after every rejected call. The same controls against
the frozen old source fail with RC21 at the first Rules alias, demonstrating the
regression. Existing1,533 raw and1,048 owned queries retain identical numeric
hashes. Normal SDK raw/world/alias builds pass; no native execution is claimed.

## Exact inputs and outputs

The pure stage takes qualified same-profile89-row Rules and typed facts:
faction0..7, shared owner bytes[89], rule_commerce, pending SE economy, bonus
count0..8 and8 original bonus-kind/value slots. The owned-world wrapper derives
all of these from its real world/rules/faction; callers cannot accidentally feed
saved effect counters through that wrapper. Bonus value2 is not used by the
original effect stage. Unrelated bonus kinds remain ignored as in the original.

The result contains exactly five fresh signed32 fields:

| Result | Original faction field | Current layout offset |
| --- | --- | --- |
| commerce | tech_commerce_bonus |0x304|
| fungus_nutrient | tech_fungus_nutrient |0x314|
| fungus_mineral | tech_fungus_mineral |0x318|
| fungus_energy | tech_fungus_energy |0x31c|
| fungus_unused | tech_fungus_unused |0x320|

These offsets describe the qualified schema, not permission to write raw bytes.
The stage returns a value and leaves both decoded world and owned wire unchanged.
A later complete award transaction must own any cache/write/persistence changes.

## Original semantics retained

Commerce starts at the faction definition's commerce bonus; all four fungus
cells start0. For each technology, original known-tech semantics determine
whether flags contribute. Faction0 and future88 do not count as known. A disabled
first prerequisite excludes ownership; a disabled second prerequisite excludes
it unless the first prerequisite is None. This None/Disable asymmetry is not the
same as research eligibility and must not be replaced by that query.

Known flags add energy fungus0x40, minerals fungus0x80, nutrients fungus0x100 and
commerce4. Faction bonus kinds7/8/9 are then added in original slot order. Negative
fungus totals are clamped at0 only after all slots; the fourth unused cell remains
0. Commerce is not clamped. Pending economy>2/>3/>4 adds1/2/3 commerce, respectively.
The original uses pending SE economy, not guessed current effects or a saved
commerce total.

Every signed addition is checked before output replacement. Invalid faction,
bonus count, Rules count or prerequisite references reject transactionally.
Original x86 wraps on arithmetic overflow; this implementation rejects that
unsafe domain rather than silently wrapping or computing a different large-sum
result. Intermediate bonus overflow rejects even if a later negative slot would
bring the final total into range. No partial result escapes.

## Editions: original base and expansion are both proved

Public source adaptation: Brendan Casey's OpenSMACX technology.cpp::tech_effects,
(C)2013–2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c; Thinker tech.cpp,
pindabe62f2d1abba03de40f3dd3048494843f6a9af. The implementation is independently
owned and checked against original instruction behavior; no proprietary code,
executable bytes, tables or assets are distributed.

Private original references execute the **entire** callback-free helpers:
SMAX0x5bae60,411 bytes, and base0x5d22e0,403 bytes. Both have inlined ownership
logic and make no callback/library calls. Their runtime addresses and faction
strides differ (0x20cc versus0x20bc); their numeric effect semantics match.
Neither helper reads ExpansionEnabled or adds a blanket78..86 exclusion. The
same-profile original rules/known state supplies the edition difference. Adding
a second research-eligibility edition filter here would change original effects.

Both originals pass1,248 identical contexts, including992 safe contexts exported
from owned-world tests. Every one of the five result fields matches our checked
stage. A full589,824-byte original global snapshot after every call proves that
only those five effect cells may change. No original `tech_advance`,
`tech_achieved`, chooser, UI/network/AI/discovery callback, PE startup or full game
is executed by this proof.

## Validation and limits

Public host controls pass285 raw contexts plus one extra invalid-Rules-count
control, including14 overflow and6 invalid cases. Expanded host controls include
1,248 original-helper contexts (1,533 combined). Owned-world controls independently
interpret64 synthetic worlds/512 faction queries; expanded controls use131 worlds/
1,048 queries, including both original rules files and three original base
tutorials. No original expansion save is claimed; its save contexts are synthetic.

Saved effect counters are deliberately poisoned with signed extrema in synthetic
worlds, proving they are not reused. Whole decoded world/wire/RNG state remains
unchanged. Unsafe arithmetic produces28 public/56 expanded rejected queries;
those are excluded from original safe-domain comparisons, not called successful
original wrap matches.

```sh
python3 ports/alpha-centauri/native/test_technology_effects.py --out /tmp/effects-host
python3 ports/alpha-centauri/native/test_technology_effects_world.py --out /tmp/effects-world-host
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_technology_effects_probe.sh raw /tmp/effects-raw
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_technology_effects_probe.sh world /tmp/effects-world
```

Normal SDK builds pass. Actual AmigaOS and independent68040 execution are UNRUN;
no lab access, publication or merge occurred. Test runners support the future
independent68040 option but its availability is not execution evidence.

## Parent integration boundary

For a fresh tile-yield calculation, consume the returned fresh effect fields
from the actual current ownership/rules snapshot rather than blindly trusting
saved fungus counters. For a future award, first stage the genuine ownership and
all other required gameplay changes in the parent's owned transaction candidate,
then recompute effects from that candidate. This helper does not make a selected
research target owned, authorise an award or resolve missing discovery branches.

Prototype/design, FREEFAC, map reveal/intel, history/Secrets, recursive sharing,
social/interlude and notification/persistence dependencies remain part of the
complete award contract. No research commit/award path should be added until
that whole dependency set is implemented or source-backed pre-commit rejection
covers each triggered unsupported effect.

Draft validation: the workflow retains every parent103 command and adds public synthetic raw, owned-world and alias suites on host sanitizers and independent Linux68040/QEMU. Final-head CI is pending at publication; actual AmigaOS execution remains UNRUN. No original media or executable-derived vectors are published.
