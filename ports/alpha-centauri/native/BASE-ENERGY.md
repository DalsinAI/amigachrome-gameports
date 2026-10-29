# Complete explicit base energy proposals

`base_energy` composes the original calculation order through the five psych
rows and social-rating contributions. It returns accounting and proposed packed
specialist slots without changing its inputs, a world or a session. Commerce
ranks and imports, technology availability, physical facilities, project
ownership/location, citizen definitions, current garrison and edition-correct
unit abilities must already be supplied as actual current facts.

This component performs both original inefficiency calculations. Stage1 faction
energy accumulation uses the first surplus before commerce/grid, while final
allocations use the second recomputation. It then processes specialists, flat
research bonuses, physical facility multipliers, selected versus faction-owned
projects, Network Backbone, slider imbalance, stockpiled minerals and psych.
It does not append bonuses to the older plain allocation result, which already
applies the slider penalty.

The first16 specialists use two packed nibble words. An invalid nibble is read
as type6 without rewriting it merely because it was clamped. An obsolete type
changes to the first available nonobsolete row that dominates all three current
outputs. Only that replacement rewrites a stored nibble. Extra specialists use
the original weighted chooser, which tests prerequisites, keeps the first
strictly better score and does not filter obsolescence until the later step.
The seven citizen rows are numeric caller facts; a reader for the owner's
CITIZENS section is still required.

## Original alien-grid bug and explicit policy

The original SMAX routine does not initialize a local used by alien energy-grid
calculation unless a foreign commerce rank qualifies. Private complete original
calls with identical game facts and different leftover caller-stack values
produce intake14,64,114 and576. The same local may feed Network Backbone when
runtime expansion is disabled. Zero is not an original save or World fact.

`AlienGridPolicy::reject_uninitialized`, the zero-valued default, rejects a path
that would consume that undefined original local. The explicit
`initialize_zero` policy is a deterministic restoration fix: start from zero,
count physical facility bits0..69 and five per project built at this base,
divide by two, then apply sanctions. It also defines the otherwise unset
Network Backbone contribution as zero when no grid was computed. This fix is
deliberate, not a claim of equivalence to arbitrary original stack contents.
No player UI or engine-wide policy selection is introduced here.

## Validation and remaining work

Both host ASan/UBSan and independently executed big-endian68040 controls pass:

- 1568 synthetic cases:1516 accepted,44 invalid and8 arithmetic overflow.
- 3072 private original-derived cases:1536 SMAC and1381 SMAX defined results
 match all120 returned fields;155 paths consuming undefined original scratch
 reject transactionally under the default policy.
- Two policy cases compare all120 fields with the actual original controlled
 zero-stack observation, and verify rejection leaves output unchanged.

Each serialized case checks three facts/output overlaps and also checks
garrison/output overlap when nonempty. Seven direct controls cover null or
overlapping arrays, zero/2048/2049 units, an invalid final-array entry, and an
ignored preexisting psych value. All executed signed32 arithmetic is checked;
errors preserve every input and output byte. No heap allocation is used.

The private comparisons supply actual original commerce outputs as explicit
composer inputs. They do not qualify owned commerce computation, a world
binding or a full turn. Original garrison observations use raw-ability designs
owned by faction0 with stack repair disabled; they do not exercise the SMAX
FREEABIL lookup or broken-stack repair. Synthetic capacity controls extend to
2048 garrison entries. Owner citizen-table loading is unimplemented.

```sh
python3 ports/alpha-centauri/native/test_base_energy.py --out /tmp/base-energy-host
python3 ports/alpha-centauri/native/test_base_energy.py --out /tmp/base-energy-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Two normal Amiga SDK builds are byte-identical; AmigaOS execution is UNRUN.
Full CI is UNRUN. Existing allocation/psych/inefficiency sources stay unchanged.
World phase binding, session revision/ownership gates, accounting mutation,
explicit LE persistence, production completion, upkeep, AI/combat/diplomacy,
player input and genuinely playable games remain unfinished.
