/* SPDX-License-Identifier: GPL-3.0-or-later
 * OpenSMACX veh.cpp::hex_cost arithmetic with explicit validated design state.
 * Brendan Casey (C)2013-2021; pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_TERRAIN_PRICE_H
#define AC_ALPHA_TERRAIN_PRICE_H
#include "movement.h"
namespace alpha {
struct MovementDesign {
 uint32_t id,abilities,speed_points;
 int32_t chassis,triad,offense,plan;
 bool sea_fungus_native,land_fungus_native;
};
/* Caller validates the adjacent edge; diagonal means x/y distances both1.
 * Native exception flags are resolved from original ID AND base/expansion
 * profile by the caller. All original two-bit rock encodings are priced, while
 * the older predefined-only wrapper retains its narrower validation contract.
 * Unknown terrain item bits remain inert here; this grants NO movement/event
 * permission, ownership access, occupancy/ZOC, combat or expenditure.
 */
inline LogicError terrain_step_price(const MovementTile& from,const MovementTile& to,bool diagonal,
 const MovementDesign& p,const MovementContext& ctx,uint32_t& output){
 if(ctx.faction<0 || ctx.faction>7 || ctx.road_rate<1 || ctx.road_rate>100 || p.id>=512 ||
    p.chassis<0 || p.chassis>=9 || p.triad<0 || p.triad>2 || p.offense< -128 || p.offense>127 ||
    p.plan< -128 || p.plan>255 || p.speed_points>(uint32_t)(99*ctx.road_rate) ||
    (from.altitude&31u) || (to.altitude&31u) || (from.rockiness&63u) || (to.rockiness&63u))return LogicError::bad_state;
 const uint32_t road=(uint32_t)ctx.road_rate;uint32_t value;
 if(to.altitude<0x60){
  value=((to.items&0x20u) && to.altitude==0x40 && p.triad==1 && !p.sea_fungus_native && !ctx.xenoempathy_dome)?road*3:road;
 }else if(from.altitude<0x60 || p.triad!=0)value=road;
 else if((from.items&(0x8u|0x1u)) && (to.items&(0x8u|0x1u)) && ctx.faction)value=0;
 else if(((from.items&(0x4u|0x1u)) || ((from.items&0x20u) && ctx.faction>0 && ctx.xenoempathy_dome)) &&
         (to.items&(0x4u|0x1u)) && ctx.faction)value=1;
 else if((ctx.xenoempathy_dome || !ctx.faction || p.land_fungus_native) && (to.items&0x20u))value=1;
 else if((from.items&0x80u) && (to.items&0x80u) && diagonal && ctx.faction)value=1;
 else if(p.chassis==2 || (p.abilities&0x400u))value=road;
 else{
  value=road;
  if(to.rockiness>0x40 && !ctx.ignore_rough)value+=road;
  if((to.items&0x200000u) && !ctx.ignore_rough)value+=road;
  // Saved faction-specific PSI designs are not the predefined native unit IDs.
  if(ctx.faction && (to.items&0x20u) && (p.id>=64 || p.offense>=0)){
   if(p.plan!=9 && p.plan!=12 && ctx.planet<=0)value+=road*2;
   else if(value<=p.speed_points)value=p.speed_points;
  }
 }
 output=value;return LogicError::ok;
}
}
#endif
