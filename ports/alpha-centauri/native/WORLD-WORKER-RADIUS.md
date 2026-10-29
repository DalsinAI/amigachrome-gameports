# Original worker-radius availability

This read-only binding reconstructs the original `farm_compute` availability
phase, a dependency of fresh base yield calculation. It returns 25 flags and
the unavailable mask over slots 0 through 20. It does not choose workers,
calculate intake, mutate actor stacks or update saved base accounting.

## Stage and source contract

`WorkerRadiusStage` supplies the actual BaseUpkeepState and editor mode. Only
the normal inspection stage (upkeep 0, editor false) is qualified. Other stages
reject before output changes; they are not silently replaced by zero flags.
The original upkeep-1 branch can add AI goals and editor stack repair can mutate
actors. Those effects require a separate complete stage and are not fabricated
by this query. No local player authority is inferred from a save snapshot.

The queried base's owner supplies faction visibility and diplomacy. Original
25-slot radius order is preserved, including slots 21 through 24 flagged 1.
Slots outside the map or not visible are flagged 2; a base square is flagged 4;
convoys or visible hostile units flag 8. Reservations by other bases flag 16.
Visible foreign territory flags 32. Multiple flags can coexist. The source
centre is included in availability; later base_yield separately forces the
centre. This query does not apply that later override.

The source territory-intelligence check, normal stack traversal and reservation
conditions are retained, including reservations by the same faction or an
ocean tile worked by another faction, the original distance thresholds and
single horizontal wrap. Later Thinker territory/worker fixes are not imported.
Radius offsets use the pinned MIT Thinker path table; behavior is independently
checked against bounded original helper execution and instructions.

## Input and output ownership

The API requires a loaded owned world, valid bounded base/map geometry and
coherent actor stacks with correct tile-presence bits. It reuses the existing
qualified transactional stack validator without repairing input. The validator
allocates temporary storage and can return out-of-memory. Unsafe visible tile
owner IDs reject rather than entering an original out-of-bounds diplomacy read.
Unsupported stage, state, topology, allocation and alias failures leave the
result unchanged. All decoded arrays, original container, World object and
transient stage are protected against overlapping output ranges.

## Evidence and limits

Private controls execute complete unchanged original helpers in both editions:
SMAC farm_compute 0x4fc140 and SMAX 0x4e68b0, with real veh_at, stack_fix,
stack_check, whose_territory and original radius tables. Each edition passes
1,280 contexts, including 640 coherent two-actor stack contexts, across map
bounds, visibility, diplomacy, convoys and reservations. All 133,120 output
bytes match an independent Python model with SHA256
`5f87f2f869d31fcec7a899aa4ab7160ad934a044791de019c7f7bba86d1d5af0`.
Every context compares the whole original global region and current-page state;
only declared availability outputs change. No PE startup, imports, fake
callbacks or full game execution occurs, and all executable/data bytes stay
private.

The owned probe separately loads explicit LE synthetic containers across
versions 7 through 12, poisons/frees input buffers and compares results and
whole-world preservation. The Python decoder reads the wire independently.
Additional controls cover all faction IDs, malformed stacks, unsafe ownership,
unsupported stages and input/output aliases. Private owner-tutorial checks
retain the original files unchanged. Final host/independent-68040 results and
normal SDK/native status are recorded in the evidence file and private handoff.

This phase has no frontend, worker selection, fresh base/satellite/specialist
aggregation, convoy/support processing, production/growth, research discovery,
turn or AI implementation. Actual AmigaOS execution remains a separate gate.
