# Original mineral accounting foundation

`base_minerals` implements the explicit-fact accounting leaf from Brendan
Casey's complete OpenSMACX `base.cpp::base_minerals`, pinned at
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`, original offset `004E9CB0`.
It is deliberately separate from a turn executor. No new End Turn button,
movement refill, saved-state mutation, production completion or AI is added.

The input must contain freshly computed intake, real support maintenance and
convoy amounts. Saved `mineral_intake_2` and saved surplus are not a substitute
for fresh computation. Facility booleans mean physical built facilities;
project booleans mean current project effects. A production queue does not
grant a facility bonus. Population, ecology, satellite, difficulty, research,
weather and sea-level facts refer to the same engine stage.

This preserves three original behaviors which later Thinker versions change
or explicitly flag:

- Incoming convoy minerals are added **before** the production multiplier.
- The ecology reduction tests Nanoreplicator or Singularity Inductor, even
  though Quantum Converter is the mineral multiplier's corresponding facility.
- Space Elevator orbital doubling happens after ecology and updates current
  surplus while leaving `surplus_final` at its undoubled value.

The mineral/support/atrocity domains are nonnegative; population is 0..127,
difficulties 0..5 and native-life selection 0..2. Other signed ecology/sea-level
inputs retain the source's division and clamp order. Every intermediate is
bounded conservatively to signed 32 bits, including source expressions whose
mixed signed/unsigned C++ arithmetic could otherwise wrap. Inputs which require
that larger/wrapping domain reject. Truncation of negative division is toward
zero. Failure and partial aliases leave the complete output untouched.

## Validation

```sh
python3 ports/alpha-centauri/native/test_upkeep_base_minerals.py --out /new/output
```

The independent Python integer model covers all 512 physical-facility/project
combinations across human/AI and orbital branches, randomized stage contexts,
signed boundaries and overflowing arithmetic. A probe checks every aligned
partial input/output overlap before processing the controls. The host run under
ASan/UBSan passed 10,300 controls: 6,247 accepted, 3,292 invalid-domain and 761
overflow rejections. Output SHA256:
`a8990edeb9da709c84222436a229e093513174e4f36fe15b12a1741eeda7bac9`.
The source also compiled and linked with strict GCC16-open3 68040/FPU flags.
The real Amiga HUNK probe then ran all 10,300 binary controls under FS-UAE
3.2.35 with the matching 23 October 2026 AROS m68k ROM pair. It returned zero
and produced 247,200 result bytes, identical to the complete independent
Python oracle and its hash above. This used the existing documented disposable
libnix FPU40 compatibility patch; no game or guest API was substituted.
Exact probe/evidence hashes are in `BASE-MINERALS-EVIDENCE-20261026.json`.

No unchanged original-executable comparison, independent Linux m68k execution,
AmigaOS 3.2 run or world/session integration has been performed for this leaf.
Those remain qualification work; this native AROS execution is scoped to the
pure explicit-fact accounting leaf.

## What a real imported-world turn still needs

The existing imported-world helpers are mostly read-only proposals. A turn
cannot safely be assembled by reusing displayed saved totals or calling the
small hand-authored colony scenario's movement refill.

1. Bind authoritative current faction/base ordering and transactional runtime
   stage state. Imported bytes alone do not establish those transient facts.
2. Finish real support/pacifism and maintenance calculation from all actors.
   Existing convoy aggregation supplies only its bounded resource contributions.
3. Allow actual upkeep-stage worker allocation, specialist effects and fresh
   technology caches. `world_stable_intake` and `world_worker_radius` currently
   permit inspection stage zero and reject upkeep stage one.
4. Bind mineral and energy accounting to fresh results. This leaf is one
   mineral component; it does not supply maintenance, commerce or energy loss.
5. Complete production, facility/unit creation and queues before growth in the
   correct original order. Existing facility selection/retooling is an order,
   not completed construction.
6. Bind the existing split `begin_population_growth` / recompute /
   `finish_population_growth` path to real reassignment and removal effects.
   Reusing pre-growth surplus after population changes is incorrect.
7. Add support checks, drones/riots/revolt, energy costs, research, ecology,
   faction aggregate accounting and event effects. Their ordering changes state.
8. Complete faction/AI/unit action scheduling and repair before a final turn
   commit; extend persistence to every mutated field and prove restart/reload.

The pinned Thinker `mod_base_upkeep` provides an additional public ordering
reference, but it includes deliberate mod fixes. Those differences must be
identified rather than silently presented as the original game.
