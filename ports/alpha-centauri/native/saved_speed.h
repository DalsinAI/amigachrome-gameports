/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from OpenSMACX veh.cpp::speed_proto/speed,
 * Brendan Casey (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_SAVED_SPEED_H
#define AC_ALPHA_SAVED_SPEED_H
#include "original_game.h"
#include "game_logic.h"
namespace alpha {
struct SavedSpeedContext {
 bool cloudbase_academy,maritime_control_center,skip_morale; // design-owner/vehicle-owner projects respectively
 int32_t effective_morale; // resolved morale_veh(veh,true,0), NOT simply its raw byte
};
/* Qualified matching base/expansion definition tables required. Original wire
 * IDs remain raw: caller must not mix old base weapon IDs with SMAX tables.
 * All512 original prototype slots are supported, including unnamed/flags0
 * faction-specific designs. No designer availability or automatic naming.
 * Projects and effective morale are explicit dependencies; no fake callbacks.
 */
LogicError saved_prototype_speed(const UnitDefinitions& definitions,const OriginalPrototype& prototype,
 size_t id,int32_t road_rate,bool cloudbase_academy,uint32_t& output);
/* Original damage/elite/project rules, without invoking unported morale logic.
 * Rejects speeds beyond byte-sized expenditure capacity rather than wrapping.
 * Does not grant movement/passability or modify a unit. Output unchanged on error.
 */
LogicError saved_vehicle_speed(const UnitDefinitions& definitions,const OriginalPrototype& prototype,
 const OriginalVehicle& vehicle,int32_t road_rate,const SavedSpeedContext& context,uint32_t& output);
}
#endif
