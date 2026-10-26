# Connected facility construction restoration stage

This component advances actual owned base records through a narrow facility
construction stage. It implements a declared restoration policy adapted from
Thinker `base.cpp::mod_base_production`, pinned at
`dabe62f2d1abba03de40f3dd3048494843f6a9af` (MIT; see
`THINKER-LICENSE.txt`). Original executable parity is unproven. OpenSMACX's
complete queue/set-facility routines informed the boundary, but do not supply
a recovered complete original production routine.

`FacilityProductionStage` accepts only base-game Creche2 and Recycling Tanks3.
It retains the existing current-human, base-owner, actual technology/facility
availability and single-slot queue gates. Active governors, network-locked
bases, units, other facilities, satellites, projects and expansion profiles
reject atomically. It does not grant technologies or change a queue merely to
make a test pass.

The stage credits freshly supplied nonnegative mineral surplus unless riots
are active, checks the parsed facility cost times the supplied current cost
factor, pays construction, applies the pinned human overflow cap and updates
the mineral baseline. Completion sets the actual built bit and production-done
flag, clears the pinned high production flag, and updates the faction's
best-mineral-output statistic. Starvation skips construction but still applies
the pinned statistic/old-done-bit handling. Already-funded construction can
complete during riots; negative surplus credits no minerals and gives zero
human overflow. These details are independently covered by the arithmetic
oracle.

The completed item remains in slot0 pending a real production choice, as in
the source's no-further-queue path. Completion emits explicit recomputation
and production-choice flags in its receipt. Popup/audio/announcement and
monument delivery are left to their consumers. No governor, AI selection,
special facility effect, worker reassignment or subsequent upkeep is silently
substituted.

## Stage authority and fresh facts

This is an engine-internal operation, outside `LocalPlayer` and its UI. The
caller must own the world, stage ordering, freshly computed intake/surplus and
the already calculated current mineral cost factor. Saved displayed mineral
totals are never read as fresh facts. Intake must be nonnegative and surplus
must not exceed intake; negative surplus remains meaningful.

The stage borrows the owned world and matching qualified rules for their
entire lifetime. Do not unload or reload the world while the stage exists.
It records one successful application per base per stage object. This ledger
is not persisted and is **not** turn or replay authority: a newly constructed
stage can credit another phase to an unfinished base. A persistent scheduler
and the fresh support/resource pipeline must be connected before exposing
this operation through live player controls. There is no End Turn button,
turn-counter increment, unit refill or full-turn completion claim.

The only core-world API extension is a private friendship for this stage.
The world still exposes const records externally. All arithmetic, gate and
buffer checks precede the first world/receipt/ledger write. Invalid calls,
recognized world replacement and outputs overlapping the world, rules, stage
or fresh input leave the caller's state unchanged.

## Persistence

`encode_facility_production_snapshot` directly extends the existing live
movement/order serialization union. It does not reload `original_bytes()` and
discard live decoded changes. Prior tile occupancy, vehicle movement, orders,
queue counters and mineral orders are retained. Added fields are the two
supported built bits, the production-state mask, mineral baseline and a
nondecreasing best-mineral-output statistic. Removing an original built bit
or changing any other decoded field rejects before writing output. Legacy
baseline and faction offsets are handled separately.

The complete original container and opaque bytes are retained. Encoding is
not action authorization, persistent stage bookkeeping, an atomic filesystem
save, or a certification of original Windows save compatibility.

## Validation and observed boundaries

```sh
python3 ports/alpha-centauri/native/test_facility_production.py --out /new/host-results
CXX=/path/to/m68k-amigaos-g++ \
  sh ports/alpha-centauri/native/build_facility_production_probe.sh /new/native-probe
```

The ASan/UBSan host suite checks6200 independent arithmetic cases plus86
world/persistence controls. These cover construction, waiting, riots,
starvation, negative surplus, human overflow, invalid ownership/stage/queue
facts, live movement before construction, exact whole-container byte changes,
reload/reencode, aliases and rejected unrelated mutations.

The real 68040/FPU HUNK ran under the existing independent FS-UAE3.2.35/AROS
m68k laboratory and documented disposable libnix FPU40 startup patch. All6200
arithmetic cases returned0 and produced198400 oracle-identical bytes. A
controlled original-container fixture constructed Recycling Tanks for40
minerals from32 stored plus8 freshly supplied, set its real facility bit, and
round-tripped its full144890-byte container. A separate actual legal land move
followed by construction preserved both changes byte-for-byte. These are
controlled native engine tests, not screenshots of new gameplay on the
original scenario.

The authentic unmodified Borehole scenario already has Recycling Tanks in
all11bases. Both owned Hive bases also refuse a legally requested Creche
order under their actual technology/facility gates. The probe records that
refusal and writes no constructed scenario. It does not alter technologies,
remove an existing facility or misrepresent a controlled fixture as original
scenario progress. Exact evidence is in
`FACILITY-PRODUCTION-EVIDENCE-20261026.json`.
