# Connected mineral support and pacifism policy

`world_base_support_policy` calculates fresh unit support for a loaded human
base-game base. It returns `SupportResult::mineral_cost` for the mineral stage
and projects the support/pacifism bits of every vehicle, without changing the
world. It never reuses saved mineral consumption, support maintenance, or saved
support/pacifism flags as calculated results.

This is an explicitly adopted restoration policy derived from Thinker's
`base.cpp::mod_base_support`, `engine_base.h::BaseSupportCosts`,
`engine_veh.h::is_native_unit` and `engine_types.h::is_fungus`, pinned at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`. Thinker is MIT licensed; its notice
is retained in `THINKER-LICENSE.txt` and in the new source headers. The pinned
OpenSMACX source (`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`) declares the original
support globals but does not implement `base_support`. Therefore this work
does **not** claim recovered-original support routine parity.

## Authoritative input and policy

The connected adapter derives population, pending SUPPORT and pending POLICE
from the owned base/faction; homed vehicles from the actual faction/home-base
records; offense/triad from qualified component definitions; clean-reactor
ability from the actual base-game design flags; and fungus, altitude, base
presence and territory from current map records. It supports base-game human
bases. Expansion and AI bases return `unsupported`, preserving both outputs.
Expansion ability grants, physical Brood Pit effects and configurable AI
support bonuses are outside this version.

SUPPORT is clamped to the table below. At +3, the free allowance is the larger
of population and four; support slots are consumed in global vehicle order.

| SUPPORT | Free units | Minerals per paid unit |
| --- | ---: | ---: |
| -4 or below | 0 | 2 |
| -3 | 0 | 1 |
| -2 | 1 | 1 |
| -1 | 1 | 1 |
| 0 | 2 | 1 |
| +1 | 3 | 1 |
| +2 | 4 | 1 |
| +3 or above | max(4, population) | 1 |

Plans through Terraform (9) participate. Supply, probe, artifact and later
missile plans do not. Clean reactors and predefined PSI-offense native units
on rendered fungus do not consume free support slots. Fungus below ocean
shelf altitude does not grant the exemption. A custom PSI design is not a
predefined native unit.

Pacifism remains independent of clean/native support exemptions. Homed
nonzero-offense units with eligible plans count when outside bases and their
own territory, or when aircraft without Air Superiority plan. Air Superiority
does not exempt a unit that also meets the outside-territory condition. At
POLICE -3, the first qualifying vehicle receives the free-skip bit and later
ones receive the pacifism-drone bit. At -4 or lower every qualifying vehicle
receives the drone bit. At higher values neither pacifism bit is set. The
count of qualifying vehicles is still reported.

The adapter recomputes only `0x10` (requires support), `0x200000` (pacifism
drone) and `0x400000` (pacifism free skip) for home units, preserving all other
bits. Foreign and homeless vehicles retain their entire state. No saved actor
flags are mutated. Territory identity hidden by diplomatic knowledge cannot
change equality with the acting faction, so the original territory-equality
test is answerable directly from map ownership.

No convoy harvesting, faction combat/social-support aggregates, energy-based
facility upkeep, support-shortage unit removal, full drone calculation,
production completion, AI dispatch or End Turn is performed. Projected flags
are explicit outputs for a future owning stage to apply transactionally.

## API and checks

```cpp
SupportResult support;
// projected must have world.game().vehicle_count entries, including foreign
// and homeless units; its storage must be disjoint from all inputs.
SupportError error = world_base_support_policy(
    world, definitions, base_id, support, projected, capacity);
// On ok, support.mineral_cost is fresh mineral maintenance for this base.
```

The pure `base_support_policy` can also operate on complete explicit per-unit
facts. Both APIs preserve their summary and state-array outputs on errors.
They reject insufficient buffers and overlap with input records, component
definitions or their other output. The world adapter rejects invalid homed
prototype/terrain facts. Neither API changes runtime RNG or owned state.

`test_base_support.py --out NEW_DIRECTORY` builds the controls with host
ASan/UBSan and strict warnings, then compares 3,403 ordered-unit cases and 87
imported-world cases with an independent Python index-set/byte-level oracle.
Cases cover all plans, triads, clean/native exemptions, stale flags, arbitrary
unrelated bits, clamped support extremes, pacifism thresholds and ordering,
zero units, invalid facts, AI/expansion rejection, and output aliasing.
Imported worlds retain deliberately poisoned saved accounting fields, so
success cannot depend on saved maintenance values. Every successful or failed
world call is checked against copies of all owned regions for immutability.

`build_base_support_probe.sh OUTPUT` builds a real 68040/FPU Amiga HUNK using
the normal `CXX` override. The same probe can run its generated binary fixtures
inside the separate FS-UAE lane:

```text
Stack 1048576
support-probe leaf support-cases.bin support-results.bin
support-probe world rules.txt world.SC world-result.bin
```

The expected host leaf-output SHA256 is
`0a1b717ec3a679378694e4b898e670603cf704a58553a14b1dc296fafb6eb0f4`.
The 24-vehicle imported-world expected output SHA256 is
`aa78022423dde8afc8a4fe8b5a402f4e57f0751920bcde3ed25e7125327ad650`.
These are calculation checks, not an assertion of game-turn playability.

On 26 October 2026 the real HUNK control ran in a separate FS-UAE 3.2.35 / AROS
20261023 68040/FPU copy. Both commands returned 0. The complete 1,943,044-byte
leaf output and 132-byte imported-world output matched their independent host
expectations byte for byte. The native control SHA256 is
`d21911d50ccf371ef38bb5a9ea0ca255e2c8b55b5664789c5cf5e37c5120599d`.
The disposable compiler used the already documented FPU40 libnix compatibility
patch. See `BASE-SUPPORT-EVIDENCE-20261026.json` for the scoped result.
