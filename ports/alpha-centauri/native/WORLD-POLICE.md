# Original double police and garrison snapshot

`world_double_police` answers exactly original double-police ability bit 22.
SMAC uses the raw prototype bit. SMAX additionally checks faction-granted
FREEABIL entries using the prototype design owner (id / 64), independently of
the actor's faction. Basic designs below 64 have no design-owner grants. A raw
bit returns before grant state is inspected. Reached FREEABIL entries are
processed in original order, including technology checks on other ability
slots. A successful double-police grant returns before later malformed slots.
Malformed reached slots or out-of-range grant counts reject; they are not
silently interpreted as absent. Deep Radar, Nethack and general abilities are
outside this API. The query does not require unrelated reactor/speed fields.

`world_police_garrison` requires an explicit `offline_validated` snapshot
profile, qualified same-profile unit/technology tables and a selected base.
This profile is a bounded read-only analysis context; it does not infer the
original network flag from a save, establish active-session authority or force
a runtime flag to zero. Network/editor stack repair is unsupported.

The selected tile's vehicle bit and complete same-coordinate stack must agree.
There must be one head, reciprocal links, no cycles, and all units on that tile
must be reachable. Damaged snapshots reject instead of invoking original repair
or logging callbacks. The result preserves actual head-to-tail vehicle order,
includes allies and carries the prototype ID, unsigned plan byte, weapon
strength, chassis triad and resolved double-police fact used by `base_psych`.
For noncombat plans >= 4, the double-police field is explicitly unused/zero;
the original psych routine does not call has_abil for those plans.

Output arrays support up to 2,048 units. Unused tail entries are zero. The caller
owns the fixed output object; a temporary allocation makes all failures
transactional and can report out-of-memory. No large automatic temporary is
required on the Amiga stack. Output must be disjoint from all tables, the World
object, raw container and every decoded allocation, including partial overlap.
Errors preserve complete output and inputs. Snapshot collection and ability
queries preserve all world records, original wire bytes, rules and runtime RNG.

This supplies the ordered police facts for subsequent complete energy/happiness
World composition. It does not run psych, repair or reorder units, commit
accounting, establish action authority, execute upkeep/turn/AI, persist a save,
or expose a player control. AmigaOS execution remains a separate gate.
