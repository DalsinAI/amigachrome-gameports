# Checked vehicle removal storage

`remove_vehicle_storage` applies the bounded storage effects of the original
`veh_kill`: lift, dense actor compaction, stack/reference renumbering, faction
active-unit/combat counters, and caller-owned cursor/lift scratch updates.
It requires disjoint caller-owned decoded arrays and exclusive access. The
storage descriptor's logical count decreases; the final physical actor slot
remains as in the original. It does not update an OriginalWorld/session count.

The supported source route is explicitly observed offline, with logging disabled
and BaseWin invisible. These facts must be supplied, not inferred from a save.
The source's real logging and window helpers naturally return in that route;
there are no invented callbacks or returned presentation outcomes here.

Every rejection, including allocation failure in inherited stack validation,
precedes writes. Current arrays must have valid reciprocal, same-position,
acyclic stacks and correct vehicle-present tile bits. Counts above2048 and
malformed references reject. Unsigned plan-byte comparison and byte/int32
counter wrapping preserve the original arithmetic, including INT32_MIN.
Order1 reference equality clears the order without clearing its waypoint.
The separate nonzero iteration cursor decrements on `>= victim`; four ordinary
cursor references instead become−1 on equality.

Private complete natural-return original observations cover432 contexts per
edition, half linked stacks, without patched instructions, observer jumps or
fake callbacks. All1114112 bytes in the comparison range match the independent
shadow. This is additional evidence for the leaf, not an original full battle.
Public synthetic controls exercise save versions7–12, final actor deletion,
2048-actor boundary stacks and failure/no-write paths. Probe output is explicit
little-endian fields; no native struct dump or filesystem/player-save protocol.

`kill` and `battle_kill` remain separate mandatory dependencies: transport,
artifact capture, units_lost, kill rewards, native rewards, interludes, active
window/log/network paths and caller tails have not been implemented by this
leaf. Legal attacks, owned combat/session authority, CRT initialization,
persistence, player input, turns and AI are held. SDK build success is not
AmigaOS execution, and no remote CI or native/frontend qualification is claimed.
