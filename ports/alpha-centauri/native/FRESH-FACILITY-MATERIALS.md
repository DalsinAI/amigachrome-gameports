# Fresh material inputs for staged construction

`FacilityProductionStage::apply_fresh_materials` computes the construction
budget directly from its borrowed current owned world. It derives actual
home-unit support, recomputes convoy/worked-tile mineral output, obtains the
world's current mineral cost factor, then immediately invokes the existing
bounded Creche/Recycling construction policy. There are no supplied mineral
intake, surplus or cost-factor scalars. The original saved resource totals
are not reused. Earlier live actor/order changes are retained; the source
world is never reloaded from its older original wire bytes.

The qualified matching unit definitions, world/resource rules and explicit
stable inspection context are still engine inputs. All worker, technology,
ownership, human, queue, facility and supported-stage gates remain. A valid
original cost factor of0 is explicitly unsupported for this construction
policy; another factor is never substituted. Projected support flags are
discarded rather than applied as a side effect.

This closes **material freshness only**. It does not establish current
starvation/riot flags, actual upkeep scheduling, complete turns or persistent
phase/replay authority. The per-object ledger and existing restoration-policy
limits remain. The method stays outside live player controls and End Turn.

Outputs cannot overlap any old/new input or the world/stage storage. New
definition/context inputs also cannot alias mutable world/stage storage.
Errors leave the world, receipt and ledger unchanged. A processed base rejects
before recalculating its now-completed facility. Failed worker or late
cost-factor checks do not consume the ledger entry.

## Validation

```sh
python3 ports/alpha-centauri/native/test_fresh_facility_materials.py \
  --out /new/host-proof
CXX=/path/to/m68k-amigaos-g++ \
  sh ports/alpha-centauri/native/build_fresh_facility_materials_probe.sh /new/native-probe
```

Independent Python wire/support/mineral/cost arithmetic models verify72
complete containers under host ASan/UBSan, including controlled valid worker
allocations, queued technologies/facilities, paid versus clean support,
industry changes, poisoned saved mineral totals, missing technology, worker
allocation rejection, valid factor0, aliases and retry/one-shot behavior.
The pre-existing6200 arithmetic plus86 world construction controls also pass
with the expanded source dependencies.

A particularly useful controlled case has six units homed to the base: fresh
upkeep4 and intake3 leave net-1, so30 stored minerals cannot fund its32-cost
Recycling Tanks. Prior changes in the **live decoded records** re-home four
units to another valid own base while the original wire stays old. The new
method then derives upkeep0, net+3 and factor8; construction completes and
leaves1 mineral. The complete encoded container preserves both the earlier
re-homing and the constructed facility. This is a controlled engine-state
test, not a newly implemented public re-home command or a gameplay screenshot.

The actual 68040/FPU HUNK ran all72 cases in the independent FS-UAE3.2.35/AROS
m68k laboratory, using the documented disposable libnix FPU40 compatibility
patch. Every case returned0; all diagnostic logs and every byte of all72
result containers match the independent host/oracle results. Reload/reencode
also passes. Input files stay unchanged.

The authentic Borehole base9 query still derives upkeep2, mineral intake6,
net4 and factor8. Its existing unit queue correctly prevents this narrow
facility operation; its result container is unchanged. No technology or
facility gate was loosened to manufacture scenario progress. Exact hashes and
scope are in `FRESH-FACILITY-MATERIALS-EVIDENCE-20261026.json`.
