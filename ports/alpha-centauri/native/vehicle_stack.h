/* SPDX-License-Identifier: GPL-3.0-or-later
 * Stack relocation adapted from OpenSMACX veh.cpp::veh_lift/veh_drop/veh_top,
 * Brendan Casey (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_VEHICLE_STACK_H
#define AC_ALPHA_VEHICLE_STACK_H
#include "original_game.h"
namespace alpha {
enum class StackError { ok,bad_geometry,bad_reference,bad_topology,bad_tile_bits,out_of_memory };
/* Borrowed decoded storage, never original wire bytes. Caller provides disjoint
 * arrays and exclusive access. Suspended/off-map vehicles are unsupported.
 */
struct VehicleStackStorage {
 int32_t width,height;
 OriginalMapTile* tiles;size_t tile_count;
 OriginalVehicle* vehicles;size_t vehicle_count;
};
/* Validate reciprocal, same-coordinate, acyclic, single-root stacks and the
 * original vehicle-present tile bits. Does not silently repair corrupt saves.
 */
StackError validate_vehicle_stacks(const VehicleStackStorage& storage);
/* STORAGE PRIMITIVE ONLY: caller must separately establish movement legality,
 * faction ownership, passability, ZOC/events, cost and action transaction.
 * Not a player command, teleport permission, turn or save writer. triad is the
 * resolved chassis triad (land=0,sea=1,air=2). Reproduces lift then drop,
 * including same-tile promotion, base ownership preservation, supply marking
 * and source ownership remaining unchanged. No RNG/cost/order fields change.
 * Every failure precedes all writes, including allocation failure.
 */
StackError relocate_vehicle_storage(const VehicleStackStorage& storage,size_t vehicle,
                                   int32_t x,int32_t y,int32_t triad);
const char* stack_error_name(StackError error);
}
#endif
