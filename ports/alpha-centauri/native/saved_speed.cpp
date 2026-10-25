/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in saved_speed.h. */
#include "saved_speed.h"
namespace alpha {
static bool valid(const UnitDefinitions& d,const OriginalPrototype& p,size_t id,int32_t road){
 return id<512 && road>=1 && road<=100 &&
  ((d.weapon_count==21 && d.armor_count==10 && d.ability_count==24) || (d.weapon_count==26 && d.armor_count==14 && d.ability_count==29)) &&
  p.chassis_id<9 && p.weapon_id<d.weapon_count && p.armor_id<d.armor_count && p.reactor_id>=1 && p.reactor_id<=4 &&
  !(p.ability_flags&~((1u<<d.ability_count)-1u));
}
static int32_t clamp(int32_t n,int32_t low,int32_t high){return n<low?low:n>high?high:n;}
LogicError saved_prototype_speed(const UnitDefinitions& d,const OriginalPrototype& p,size_t id,int32_t road,bool cloudbase,uint32_t& output){
 if(!valid(d,p,id,road))return LogicError::bad_state;
 if(d.weapon_count==26 && id==19){output=0;return LogicError::ok;}
 int32_t speed=d.chassis[p.chassis_id].speed;const int32_t triad=d.chassis[p.chassis_id].triad;
 if(speed<0 || speed>99 || triad<0 || triad>2)return LogicError::bad_state;
 const bool transport=d.weapons[p.weapon_id].mode==7;
 if(triad==2)speed+=p.reactor_id*2;
 if(transport)--speed;
 if(p.ability_flags&0x800000u)--speed;
 if(p.ability_flags&0x400u)speed+=triad==2?p.reactor_id*2:1;
 if(triad==2){
  if(p.ability_flags&0x8000000u)speed+=2;
  if(cloudbase)speed+=2;
  if(p.ability_flags&0x20u)speed=(speed*4)/5;
  if(transport)speed/=2;
 }
 output=(uint32_t)(clamp(speed,1,99)*road);return LogicError::ok;
}
LogicError saved_vehicle_speed(const UnitDefinitions& d,const OriginalPrototype& p,const OriginalVehicle& v,int32_t road,const SavedSpeedContext& ctx,uint32_t& output){
 if(v.unit_id<0 || v.unit_id>=512 || v.faction_id>=8 || ctx.effective_morale<0 || ctx.effective_morale>6)return LogicError::bad_state;
 uint32_t base;auto error=saved_prototype_speed(d,p,(size_t)v.unit_id,road,ctx.cloudbase_academy,base);if(error!=LogicError::ok)return error;
 if(d.weapon_count==26 && v.unit_id==19){output=0;return LogicError::ok;}
 int32_t speed=(int32_t)base;const int32_t triad=d.chassis[p.chassis_id].triad;
 if(triad==1 && ctx.maritime_control_center)speed+=road*2;
 if(!ctx.skip_morale && ctx.effective_morale==6 && (v.unit_id>=64 || d.weapons[p.weapon_id].offense>=0))speed+=road;
 if(v.damage_taken && triad!=2){
  const int32_t moves=speed/road;
  const int32_t hp=p.plan==12?1:clamp(p.reactor_id,1,100)*10;
  const int32_t denominator=p.plan==12?1:clamp(hp,1,99);
  speed=(moves*clamp(hp-v.damage_taken,0,9999)+denominator-1)/denominator;
  speed=clamp(speed,triad==1?2:1,999)*road;
 }
 if(speed<0 || speed>255)return LogicError::overflow;
 output=(uint32_t)speed;return LogicError::ok;
}
}
