# Fresh owned-world mineral proposal

`world_base_minerals` connects the mineral accounting leaf to an actually loaded
`OriginalWorld`. It first computes real convoy contributions in actor order,
then recomputes stable human intake and raw terraforming ecology. It never uses
saved intake, consumption, surplus or saved ecological damage as fresh input.

The caller supplies a `WorldMineralStage` with the existing stable-intake stage
and a **fresh forces-maintenance result** from the support engine. That integer
is an explicit dependency, not proof that support was calculated. The caller
also supplies the parsed `WorldRules` from the same rules input; its
`sea_level_rises` replaces an otherwise missing runtime WorldBuilder pointer.

```cpp
WorldMineralStage stage{intake_stage, freshly_calculated_maintenance};
WorldMineralResult proposal;
auto error = world_base_minerals(world, technologies, basic, world_rules,
                                resources, base_id, stage, proposal);
```

The result retains the actual convoy and fresh-intake proposals, every bound
`MineralFacts` value, and the final mineral/ecology accounting result. The
adapter binds:

- Physical facility bits 19/20/21/22/31/32 and actual project owners for
  Pholus, Space Elevator, Singularity Inductor and Bulk Matter Transmitter.
- Faction mineral satellites, clean-mineral modifier, major atrocities,
  difficulty, technology ranking/transcendent count and pending Planet effect.
- Actual population, sea level and native-life selection (`world_settings[6]`).
- The original global Voice predicate: project slot31 being anything other
  than unbuilt, including destroyed, combined with the transcendence rule.
- The actual perihelion flag and edition-specific orbital queue IDs.
- Version12's little-endian tectonic counter for the base owner. Earlier
  versions use zero, as the pinned original-compatible loader explicitly does.

The original OpenSMACX function/enum references are pinned at
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`; the versioned tectonic initialization
comes from Thinker `savegame.cpp` at
`dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT notice retained in the project).
The leaf's documented original convoy multiplication, ecological reduction
and orbital `surplus_final` quirks remain unchanged.

## Stage and mutation limits

This remains a read-only proposal for the existing qualified stable-human
inspection stage0. Nonhuman/governed/reallocation/fungus contexts retain the
upstream helper's explicit rejection; this adapter does not pretend the saved
worked mask is a completed upkeep allocator. Mismatched edition/version,
negative maintenance, malformed project references, unsupported native-life
values and arithmetic overflow reject without replacing output.

All parsed rule objects, stage storage, raw original bytes and every decoded
world region must be disjoint from output. Whole and partial aliases reject
before an output write. The caller must not mutate inputs concurrently.

No base/faction/actor state, RNG, current minerals, production queue or session
revision changes. No original-world turn, production completion, support
authority, mineral accrual or save-writer support is added. Existing snapshot
encoders cannot yet persist these accounting fields, so no mutation is staged.

## Checks and actual Boreholes execution

```sh
python3 ports/alpha-centauri/native/test_world_base_minerals.py \
  --out /new/output \
  --original /private/Borehole.SC --original-rules /private/alpha.txt

CXX=/path/m68k-amigaos-g++ \
  sh ports/alpha-centauri/native/build_world_base_minerals_probe.sh \
  /private/world-base-minerals-probe
```

The host ASan/UBSan run passed **144 worlds / 174 queried bases**, with 134
accepted proposals. Synthetic original containers cover both editions,
versions7..12, actual convoy actors, facility/project combinations, events,
orbital queues, negative and overflowing maintenance, tectonic overflow,
invalid project/difficulty/native-life fields, worker-stage rejection and
specialist rejection. Saved accounting fields are deliberately unrelated
poison values. Independent Python wire, worker, tile, convoy and integer models
calculate expected results; the probe compares all immutable world/rule/stage
storage and tests partial aliases.

The unchanged official Boreholes world was also executed by the real Amiga
HUNK probe under **FS-UAE3.2.35 / AROS20261023 / 68040 FPU**, using the existing
documented disposable libnix FPU40 source fix. It returned **RC0**. All 269
output bytes matched the host and independent wire oracle:

| Base | Fresh mineral intake | Raw terraforming ecology | Explicit maintenance | Consumption | Mineral surplus | Final ecology |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 9 | 6 | 12 | 3 | 3 | 3 | 0 |
| 10 | 7 | 8 | 3 | 3 | 4 | 0 |

Maintenance3 was deliberately supplied to exercise the binding; it is not a
claim that the original support engine produced3 for those bases. The other
nine nonhuman bases correctly reject the stable-human stage. The host corpus
also checks all eleven original bases with explicit maintenance0,3,7. Only the
oracle's private in-memory copy renumbers each target base to reuse its
existing base0 model; the native input and actual scenario bytes are unchanged.

Exact input/binary/result hashes and remaining qualification gates are in
`WORLD-BASE-MINERALS-EVIDENCE-20261026.json`. Original executable comparison,
AmigaOS3.2 execution, full support derivation and transactional turn integration
remain separate work. No game files or executable binaries are committed.
