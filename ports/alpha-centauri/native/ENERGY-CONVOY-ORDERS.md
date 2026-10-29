# Original deficit-energy convoy orders

`energy_convoy_orders` restores the complete callback-free behavior of original
`base_energy_costs` as an immutable order-change proposal. The original caller
runs it in stage 2 after drone/doctor processing, when the turn is greater than 1.
That caller's lifecycle and phase authority are not established by this query.

The current signed energy surplus is explicit. A nonnegative value leaves all
orders alone. A negative value inspects every actor in the world and proposes
order 0 when all of these original conditions hold:

* Actor faction equals the current base faction.
* The actor's actual saved design has supply plan 10.
* Current order is convoy 3 and automatic resource type is energy 2.
* Its tile has the physical base bit and a faction nibble below 8.

The tile faction may be different from the actor/current base faction. The actor's
home base, stack links, territory owner and current base location are not filters.
There is no original callback, random draw or facility-maintenance debit in this
routine. Only the order byte changes; the result is a bounded actor bitmap.

Source reference: OpenSMACX `base.cpp::base_energy_costs`, Brendan Casey
(C) 2013–2021, GPL-3.0-or-later pin
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`; original SMAC `0x50a030`
and SMAX `0x4f4dc0` instruction streams were inspected privately. No original
executable bytes, private assets or original observations are published here.

The tests use an independent Python selector, all supported saved-world versions,
signed energy boundaries, empty/nonempty actor sets, foreign physical base tiles,
plan/order/resource failures, home-base differences, immutable World/RNG and
whole-object alias/output-preservation guards. Bounded private executions of both
complete original routines return normally with every actor byte matching the
independent model and other mapped global/map bytes unchanged.

This does not mutate a World or commit an order, refill movement, calculate fresh
accounting, run a whole base upkeep or turn, or expose a frontend/player action.
The engine must obtain the current energy result from the correct fresh stage
and apply the proposal through its own phase transaction. Saved energy values
must not be silently recycled as fresh accounting.

## Local qualification

Host ASan/UBSan and an independent static big-endian 68040 build passed the same
3,690 queries over 147 worlds, including three private owner tutorials: 2,220
accepted and 1,470 rejected. The output hashes agree. Public synthetic controls
cover 144 worlds / 3,600 queries. Aliased outputs and unloaded/wrong-profile
worlds preserve state; the immutable World, original bytes and RNG remain intact.

Both changed owned executables also match all 480 complete original routine
observations in each edition, comparing full actor-record projections. Original
helpers return normally without patched instructions, imported callbacks or fake
turns. These are controlled helper executions, not a whole base upkeep or game.

Two fresh normal AmigaOS SDK builds are byte-identical, but native execution and
remote CI are unrun. All parent source files and workflow commands remain intact.
No frontend, input control, deployment or watch preview was changed.
