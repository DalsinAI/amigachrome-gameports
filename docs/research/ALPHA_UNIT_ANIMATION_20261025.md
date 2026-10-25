# Original map unit animation recovery

Host-only research, 25 October 2026. No running instance or lab was accessed.
The palette integration remains frozen at adcbd7efadc5345b2abb020d27e4bb05543fb66f;
PR87 and PR90 heads are unchanged. No animation production change is made here.

## Recovered frame selection

Original SMAX chassis instructions were privately executed for ticks0..31 for
both supported chassis profiles:64 cases,320 append calls. Append adapters only
record receiver-bound calls; this is not execution of the full renderer/game.
Basic infantry selects body tick modulo15, with the same body-root translation
already qualified. It does not simply cycle all17 body frames. Basic speeder
passes the raw tick to its front/back wheel models; its other eight components
(including the separate reactor) stay frame0. The two wheel files each have17
frames. Original append methods independently reduce the requested frame by
the destination model's frame count, so those wheels wrap modulo17.

The counter at original address0x8e9f50 starts at1 in movement-start0x4bfe00.
Movement draw0x4c0f50 assembles the current pose, then increments that counter.
Normal draw0x4c12a0 assembles the current counter but does not increment it.
The normal vehicle-routing function0x5594f0 dispatches ordinary, turning and
movement draws using its0x10/0x20 flags. Selection alone supplies no independent
wall-clock frame advance in these paths. Consequently a blanket idle timer
cycling every unit would invent behaviour. This conclusion is limited to the
inspected supported infantry/basic-rover path; other chassis have their own
counter handling and are not generalised here. A direct-address census found
18 references to the counter in the executable code sections; indirect alias
writes are not excluded by that census.

## Timing and action separation

The movement controller0x55a150 uses timeGetTime and processes non-input events
while waiting. Instruction inspection (not full-controller execution) shows a
single-player minimum movement-step wait16ms on land,10ms for triad1 and5ms for
triad2. Multiplayer starts from10ms, scaled to6/3ms, and local-owner movement can
skip the wait. It chooses9/6/3 interpolation steps from the signed preference
at0x94581c; multiplayer chooses4. The identity of that preference has not been
recovered and is not assigned a friendly setting name. These are minimum waits,
not a promised frame rate: rendering cost and visible viewports affect delivery.
The counter advances per scoot draw; it is not an elapsed-millisecond counter.

The raw decompile aliases stack locals incorrectly in the large controller.
Timing constants, loop bound locals and timer comparison were checked against
actual disassembly; raw decompiled pseudo-arguments are not a portable API.
No game action, unit position or economic state was executed/modified here.

## Bounded implementation strategy

Keep the existing sixteen facing/zoom caches for explicitly static pose0.
Add an explicit action-pose input, separate from game RNG/turn time, only when a
source-backed move/turn controller requests it. Do not key an unbounded cache by
tick, unit or faction. A single scratch RGBA/depth frame for the active actor
(or a small fixed LRU) is sufficient; replace it when pose/facing/zoom/palette
changes. Preserve the existing global/faction palette generation invalidation.

For infantry use tick%15 only on its body and frame0 on the reactor. For this
basic rover use tick%17 only on the two wheels and frame0 elsewhere. Object
visibility, translation, matrix and lighting must all use the same selected
frame. Both bounds and final pixels must consume that frame; reusing frame0
bounds can clip walking poses. Unsupported profiles remain unsupported.

Before integration, qualify synthetic hidden/moving frames, wrap boundaries,
shared-depth ordering and palette-cache changes, then owner-data poses against
original append traces. Native execution and action-controller timing require
an allocated isolated lab after the user's watch session ends. This research
is not an implemented or native-qualified animation feature.
