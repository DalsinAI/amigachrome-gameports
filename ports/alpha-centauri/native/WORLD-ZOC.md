# Current World zone of control

Read-only original zoc_sea/zoc_move uses the eight source directions in order,
one cylindrical X wrap, land/ocean equality, occupied tile's stored owner nibble,
observer-to-tileowner pact, and actual hostile actor visibility/invisibility.
MultiplayerActive is explicit runtime data. Returning tileowner+1 preserves the
original even when that differs from the actual actor faction. The move variant
alone exempts base-bit tiles with a stored owner nibble below8.

Strict whole-World stack validation precedes all outputs; corrupt, disconnected,
off-map or inconsistent occupancy data refuses the unresolved original veh_at
repair/UI path. The adapter casts the privately owned nonconst allocations only
for the existing read-only validator, which never writes them. Allocation failure
also rejects before writes. Whole-world validation is currently per query; no
cached topology/performance or interactive gameplay claim is made.

32synthetic Worlds24576queries352guards pass host ASan/UBSan and actual independent
Linux68040, with directed pacts, mixed/stale stored owners, network/visibility,
invisible units, land/ocean, wrapped boundaries and malformed stacks. Complete
unchanged original zoc_sea/zoc_move plus real veh_at return3300times across30valid
synthetic Worlds (1650each edition), matching both owned binaries. All851968mapped
global bytes stay unchanged. No patched helper/observer/callback/PE startup.

No authority to move or retreat, no supply-pod/fungus destination validation,
no stack relocation, events, RNG/session/turn or complete battle is provided.
Normal Amiga SDK builds only; native/frontend/remote final-head CI UNRUN.
