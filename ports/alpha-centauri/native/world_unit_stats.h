/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-world bindings for recovered OpenSMACX morale/speed dependencies.
 * Brendan Casey (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_WORLD_UNIT_STATS_H
#define AC_ALPHA_WORLD_UNIT_STATS_H
#include "original_world.h"
#include "saved_speed.h"
namespace alpha {
/* Project argument is an original SP array INDEX, not facility ID70+index. */
LogicError world_has_project(const OriginalWorld& world,size_t project,int32_t faction,bool& output);
LogicError world_has_technology(const OriginalWorld& world,const Rules& technologies,int32_t tech,int32_t faction,bool& output);
LogicError world_effective_morale(const OriginalWorld& world,const UnitDefinitions& definitions,
 const Rules& technologies,size_t vehicle,bool check_drone_riot,uint32_t& output);
/* Resolve only the abilities queried by original speed_proto. Design owner is
 * prototype_id/64, not the vehicle faction. The original base-game component
 * profile has raw flags only; faction grants belong to the expansion engine. Already present bits and predefined
 * designs retain original short-circuiting; FREEABIL requires owner technology.
 * Output preserves all raw design bits and adds qualified speed-related grants.
 * Does not answer Deep Radar/Nethack or other unrelated ability questions. */
LogicError world_speed_ability_flags(const OriginalWorld& world,const UnitDefinitions& definitions,
 const Rules& technologies,size_t prototype,uint32_t& output);
LogicError world_vehicle_speed(const OriginalWorld& world,const UnitDefinitions& definitions,
 const Rules& technologies,size_t vehicle,int32_t road_rate,bool skip_morale,uint32_t& output);
}
#endif
