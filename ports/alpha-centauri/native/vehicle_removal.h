/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_VEHICLE_REMOVAL_H
#define AC_ALPHA_VEHICLE_REMOVAL_H
#include "vehicle_stack.h"
#include "original_faction.h"
namespace alpha {
enum class RemovalError {ok,bad_state,unsupported,out_of_memory};
enum class RemovalObservation {missing,supplied};
/* Caller-owned runtime facts and references, never inferred from saved fields.
 * This leaf supports the actual offline/log-disabled/invisible-BaseWin route.
 * These facts do not establish legal attack or active-session authority. */
struct RemovalRuntime {
 RemovalObservation observation;
 bool multiplayer_active,logging_enabled,base_window_visible;
 int32_t cursor[4],iteration_cursor,lift_vehicle,lift_x,lift_y;
};
struct VehicleRemovalStorage {
 VehicleStackStorage stacks;
 const OriginalPrototype* prototypes;size_t prototype_count;
 OriginalFactionState* factions;size_t faction_count;
 RemovalRuntime* runtime;
};
/* Atomic checked STORAGE primitive. All validation/allocation precedes writes.
 * Repairs links/order1 vehicle references, decrements faction counts with source
 * byte/int32 wrapping, compacts the dense array and lowers its logical count.
 * The final physical slot is retained exactly as in the original. Runtime lift
 * scratch and four cursor IDs/iteration cursor follow the original veh_kill.
 * Not kill()/battle_kill: no casualty awards, units_lost, capture, transport,
 * active-window/log/network lifecycle, combat CRT/session commit or persistence.
 * Prototype/faction arrays, runtime, storage object and stack arrays must be
 * pairwise disjoint. The caller owns every supplied mutable object exclusively.
 */
RemovalError remove_vehicle_storage(VehicleRemovalStorage&,size_t victim);
}
#endif
