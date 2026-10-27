# Empty Sea Movement

The local player explicitly loads the new surface-action capability. It can
use the existing clear-land action and the separate `WorldActions::clear_sea_step`
action. `load_local_land_actions` remains land-only; a command prepared through
one route cannot be committed through the other. Ordinary production-only
sessions do not gain movement permission. A failed import preserves the old
session and its capabilities.

## Source Inventory

The implementation composes already qualified original speed, terrain-price,
movement-budget and stack-relocation primitives. This is a bounded local
restoration action, not a recovered complete `order_veh` implementation.

* OpenSMACX, pin `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`,
  [`veh.cpp`](https://github.com/b-casey/OpenSMACX/blob/2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c/src/veh.cpp):
  complete original `hex_cost`, `speed_proto`/`speed`, `veh_lift` at
  `005BFFA0`, `veh_drop` at `005C0080`, `sleep` at `005C01A0`, and
  `veh_cargo` at `005C1760`. Copyright Brendan Casey 2013–2021, GPL3.
* Thinker, pin `dabe62f2d1abba03de40f3dd3048494843f6a9af`,
  [`veh.cpp::stack_veh/set_board_to`](https://github.com/induktio/thinker/blob/dabe62f2d1abba03de40f3dd3048494843f6a9af/src/veh.cpp),
  [`move.cpp::allow_civ_move`](https://github.com/induktio/thinker/blob/dabe62f2d1abba03de40f3dd3048494843f6a9af/src/move.cpp),
  and [`engine_veh.h`](https://github.com/induktio/thinker/blob/dabe62f2d1abba03de40f3dd3048494843f6a9af/src/engine_veh.h).
  MIT attribution is preserved in `THINKER-LICENSE.txt`.

`veh_cargo` returns capacity. It does **not** establish that a ship is empty.
Boarding uses order1 (`ORDER_SENTRY_BOARD`), the transport ID in
`waypoint_x[0]`, and state bit0 (`VSTATE_IN_TRANSPORT`). The source stack routine
also automatically loads eligible co-located land units at open sea. The new
action refuses those conditions, rather than moving a ship and abandoning its
passengers. An assigned boarder referring to this ship also refuses the action
when its coordinates are elsewhere; that case needs transport reconciliation.

## Qualified Subset

Base-game human units only, with the saved map player, vehicle owner and active
turn faction agreeing. Supported designs are ordinary sea combat plans0–3
without capacity, and plan7 naval transports using the transport weapon mode
with positive capacity. Abilities, PSI/native designs, faction FREEABIL grants,
standing orders, waypoints and wake/embark/alert states remain unsupported.

Only adjacent, fully visible ocean/ocean or ocean/owned coastal-base edges are
accepted. Both endpoint neighborhoods must already be visible, with no nearby
foreign actors or bases. Foreign territory, pods, fungus and monolith events
are deferred. A land tile must contain the actual owned base and its physical
base bit; plain shore and land-base canal chains are excluded.

An ordinary unembarked garrison may remain in an owned base while the ship
leaves or arrives. Any board order or embarked state at either endpoint is
conservatively deferred, including a board order assigned to another ship.
Mixed non-sea stacks at open sea are unsupported. This is a qualified subset,
not a claim that every rejected action is illegal in the original game.

The current design, damage, morale and projects determine movement speed.
Existing terrain pricing determines the full step price. Insufficient remaining
movement refuses the fractional attempt; exhausted movement never refills.
Successful moves apply recovered lift/drop topology, cost, facing and movement
state, preserving unrelated units and bytes. No movement RNG draw is invented.

The separate session route binds commands to the current session identity and
revision; movement invalidates stale plans. Suspension, failed replacement,
cross-session commands, capability downgrade and overlapping owned-world/output
buffers are checked before mutation. Checkpoint encoding remains `ACLOCAL1`
around the complete original payload. Resuming through the explicitly enabled
local player restores surface capability; the land-only session loader still
does not grant it.

## Validation

`test_surface_actions.py` uses complete original-format synthetic containers
and an independent wire oracle for lift/drop, expenditure, facing and checkpoint
checksums. It covers all eight directions, old land-only refusal, explicit
capability isolation, stale/cross-session/suspended commands, empty capacity
versus passengers, base garrisons, boarders, fog, encounters, ownership and
movement exhaustion. Source data and opaque payload bytes must remain intact.

The existing 145-check local-player controller and 46 clear-land boundary cases
remain regression gates. The native probe uses real AmigaOS HUNK code and the
same case corpus. Native execution receipts are separate evidence; an AROS run
does not qualify AmigaOS3.2.3 or a complete turn/combat/campaign engine.

The genuine Boreholes scenario contains an empty Transport Foil at vehicle19,
position `(18,36)`, capacity2, with no order or linked passengers. Its
`(19,35)` ocean neighbor is a qualified full-cost move. Vehicle2, initially
selected by the frontend, is an ordered Plasma Garrison, not that ship.
