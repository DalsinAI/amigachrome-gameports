/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original repair_phase behavior, OpenSMACX/Thinker layout attribution in
 * original_game.h. Original SMAC/SMAX edition differences retained.
 */
#ifndef ALPHA_REPAIR_VEHICLE_H
#define ALPHA_REPAIR_VEHICLE_H
#include "game_logic.h"
#include "original_game.h"
namespace alpha {
struct RepairVehicleFacts {
 Variant variant;
 uint32_t phase_faction,local_faction,turn,vehicle_index;
 OriginalVehicle vehicle;
 int32_t triad,reactor,weapon_offense;
 uint32_t tile_items;
 uint8_t tile_climate,tile_val2;
 bool friendly_territory,base_present,base_riots,native_breed_bonus;
 bool repair_facility,repair_project,cargo_capacity_nonzero,other_stack_repair_bay;
 bool nano_factory,xenoempathy_dome,human;
};
struct RepairVehicleResult {
 OriginalVehicle vehicle;
 bool processed,healing_attempted,draw_requested;
 uint32_t healing,damage_floor;
};
/* Pure proposal for ONE original repair first-pass actor. Explicit facts must
 * come from real current definitions, territory/base/breed helpers and coherent
 * whole-stack inspection. cargo_capacity_nonzero means veh_cargo CAPACITY,
 * not transported occupancy. SMAC air repair projects do not exist. Native
 * breed uses the base OWNER; project ownership otherwise uses phase faction.
 * draw_requested describes the later second-loop predicate, not delivery.
 * No World mutation, faction ODP reset, rendering, phase admission, movement
 * refill authority, player action, end turn, persistence or whole-phase claim.
 * Invalid input/output overlap rejects before writes. Foreign actors preserve
 * their complete record without inspecting unused profile facts.
 */
LogicError repair_vehicle(const RepairVehicleFacts&,RepairVehicleResult&);
}
#endif
