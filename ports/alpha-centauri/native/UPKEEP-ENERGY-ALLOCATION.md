# Fresh Economy, Labs And Psych

`allocate_energy` completes the allocation arithmetic after fresh convoy,
commerce and HQ inefficiency accounting. `world_energy_allocation` derives
current faction sliders, pending efficiency, physical facilities, actual project
locations, packed specialist types and technology ownership from the owned
world. Persisted economy, labs, psych, energy intake and surplus are never used
as fresh results. The caller supplies freshly computed surplus and commerce.

The world adapter currently qualifies human base-game worlds (versions 7–10).
Unsupported expansion/AI contexts return `bad_state`; it does not invent alien
energy grid policy. The independent leaf accepts explicit qualified facts.
Neither API mutates the world, sliders, credits, research, psych/drone state or
turn counters. Stockpile Energy belongs to production and is not included.

## Policy And Sources

OpenSMACX at `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c` does not contain
the complete original `base_energy` body. Allocation is explicitly a restoration
policy adapted from Thinker at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, `src/base.cpp` lines 2176–2306,
`mod_base_energy`, and lines 4514–4538, `pick_specialist` / `best_specialist`.
Thinker is MIT licensed; see `THINKER-LICENSE.txt`. This is not claimed as
instruction-backed original-engine parity.

The policy uses the pinned default Biology Lab +2 (`src/main.h:232`) and no
configurable mod options. Other supported behavior includes:

- Clamp distributable surplus to 0–9999; psych and economy rounded with +4/10,
  labs receiving the remainder, and the original auxiliary economy total.
- Seven parsed specialist definitions, packed 16-slot nibble selections,
  first dominating available replacement for obsolete types, and weighted
  economy + labs + twice psych selection beyond 16 specialists. Upgrade slots
  are returned as proposals. Negative custom bonuses reject.
- Physical Bank, Node, Theatre, Hospital, Nanohospital, Tree Farm, Hybrid Forest,
  Fusion and Quantum Lab quarter coefficients with +3 rounding. A faction's
  Virtual World supplies Theatre effect only where a Node is physically built.
- Supercollider and Theory lab doubling in the project base; Space Elevator
  economy doubling; Longevity's extra half economy in Free Market; Backbone
  global physical Node count and fresh commerce in the project base.
- Pending-efficiency slider imbalance penalties, preserving operation order
  and rounding. Original signed intermediate overflow rejects atomically.

Base-game project indices are 6, 12, 14, 18, 21 and 27. Planetary Energy Grid
is expansion-only, so no hidden Bank effect is assumed in the base adapter.
Queued facilities confer no effects.

`read_citizens` follows the complete OpenSMACX `src/alpha.cpp:1087–1100`
parser: ten rows, the first seven containing technology/economic fields. It
ignores trailing sprite/annotation columns as the original does; these exist
in the genuine demo. The table is owned, bounded, and committed only on success.

## Verification

`test_upkeep_energy_allocation.py` passes **5,410** independent integer-oracle cases
under host ASan/UBSan. Every legal slider split is exercised with rounding
boundaries and efficiency extremes; random specialist/facility/project mixes,
invalid domains and late overflows are included. Complete 302,960-byte output
SHA-256 is `703917c0907df292ab9d3631499d21c09f2c310117d9cad8aed5b2b74fdaa43f`.

`test_world_energy_allocation.py` passes **328 world cases and 5 parser cases**
under ASan/UBSan, including all six import versions, disabled technologies,
obsolete specialists, more than 16 specialists, projects, poisoned saved totals,
bad sliders/counts/project references and unsupported expansion contexts.
The probes verify byte-for-byte preservation of all decoded and wire inputs,
output sentinels on failure, and output-alias rejection at multiple offsets
inside every input region. The world oracle reads the wire independently.

The genuine demo `alpha.txt` and unchanged Boreholes world also pass the host
probe. Base 9 with fresh preallocation energy 6 and commerce 0 produces economy
5, psych 0 and labs 5, with coefficients 6/6/6 and no specialists. Those totals
include current facility multipliers; 6 energy is not itself 6 credits.

Strict native HUNK builds use the provided script with 68040/FPU, project
warnings as errors and no exceptions/RTTI. All **5,410 leaf cases** also pass in
genuine FS-UAE/AROS execution with guest return code 0 and complete 302,960-byte
output matching the independent oracle. `evidence/alpha-energy-allocation-20261027/leaf-native.json`
records the binary and full output hashes. A successful build alone is not
runtime validation.

All **328 connected world cases** also pass in the real AROS guest, each with
return code 0 and complete text output matching the independent wire oracle.
The aggregate output is 21,047 bytes, SHA-256
`aa4ef8962b223d929ffd9dae9b2e506d6c046b7f14e2bb072b06cdd1278b3b65`;
`evidence/alpha-energy-allocation-20261027/world-native.json` records every case.

## Proposed Worker Assignments

`world_energy_allocation_for_assignment` takes a `ProposedCitizenAssignment`
from a freshly qualified worker stage. It derives all other live world facts
as above, while replacing the old specialist count and packed selections with
the proposal. It does not mutate a const world or pretend that its decoded
records have changed.

The proposed population must equal the current world population, the centre
must remain worked, only the first 21 mask bits may be selected, and worked
tile count plus specialists must equal population plus one. Every active
encoded specialist slot must be 0–6; unused nibbles remain opaque.
Effective types after obsolete upgrades must have their prerequisite, lack
their obsolete technology and satisfy minimum population unless psych is at
least two. A proposal cannot grant unavailable specialist output. Physical
tile availability and allocation priorities are still the worker stage's
prerequisite. Population growth must update/recompute its genuine population
before using this API. Upgraded specialist nibbles are returned as proposals.

The additional oracle test deliberately poisons old specialist counts while
passing valid fresh assignments, exercises changed specialist types and
more than 16 specialists, and rejects mismatched population, missing centre,
high mask bits, incomplete assignments and invalid active specialist IDs.
All **280 final proposed-assignment cases** pass both host ASan/UBSan and genuine
AROS guest execution with every return code 0 and complete outputs matching
the independent oracle. There are 114 accepted proposals, including 16 with
more than 16 specialists; remaining cases reject malformed or unavailable
specialists. The aggregate final native output is 28,182 bytes, SHA-256
`d35b8fbe5abe0ea5ddd37a3b74d1c6b6c5265837ac60f93de9648408d802686b`.
`evidence/alpha-energy-allocation-20261027/proposed-native.json` identifies
source `70895a4` and the final binary. Earlier native runs before the additional
availability check do not qualify this version.

```
python3 test_upkeep_energy_allocation.py --out /tmp/alpha-allocation-leaf
python3 test_world_energy_allocation.py --out /tmp/alpha-allocation-world
CXX=m68k-amigaos-g++ ./build_energy_allocation_probe.sh leaf /tmp/allocation
CXX=m68k-amigaos-g++ ./build_energy_allocation_probe.sh world /tmp/world-allocation
```

Commerce generation, full energy/grid qualification, applying specialist
upgrades, psych-driven drones, credit/research accumulation and persistent
upkeep/turn scheduling remain separate stages. A caller can now add the actual
fresh `allocation.economy` to a current credit ledger before facility upkeep;
using raw surplus as credits remains incorrect.
