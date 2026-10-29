/* SPDX-License-Identifier: GPL-3.0-or-later. See repair_vehicle.h. */
#include "repair_vehicle.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError repair_vehicle(const RepairVehicleFacts& in,RepairVehicleResult& out){
 if(detail::effects_range_overlap(&in,sizeof in,&out,sizeof out) ||
    (in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) ||
    in.phase_faction>7 || in.local_faction>7 || in.vehicle_index>INT32_MAX)return LogicError::bad_state;
 RepairVehicleResult next{};next.vehicle=in.vehicle;
 if(in.vehicle.faction_id!=in.phase_faction){out=next;return LogicError::ok;}
 if(in.vehicle.unit_id<0 || in.vehicle.unit_id>=512 || in.triad<0 || in.triad>2 ||
    in.reactor<0 || in.reactor>255 || in.weapon_offense<-128 || in.weapon_offense>127)return LogicError::bad_state;
 auto& v=next.vehicle;next.processed=true;
 v.iter_count=0;v.moves_spent=0;v.state&=0xfbffdffdu;v.flags&=0xefffu;
 if(((in.turn+in.vehicle_index)&3u)==0){v.state&=~0x800u;v.flags&=(v.flags&2)?0xfffdu:0xfffeu;}
 if((v.order==1 || v.order==2) && v.waypoint_y[0]!=0){
  const uint32_t bits=((uint32_t)(int32_t)v.waypoint_y[0]-1u)&0xffffu;
  v.waypoint_y[0]=(int16_t)(bits<32768u?(int32_t)bits:(int32_t)bits-65536);
  if(v.waypoint_y[0]==0)v.order=0;
 }
 const bool physical_base=(in.tile_items&1u) && (in.tile_val2&15u)<8u;
 if(physical_base || (in.triad==0 && (in.tile_items&0x800u)) ||
    (in.triad==2 && (in.tile_items&0x40000u)))v.state&=~8u;
 const bool expansion=in.variant==Variant::alien_crossfire;
 const bool tower=expansion && v.unit_id==19;
 if(tower)v.faction_id=0;
 const bool ogre=expansion && v.unit_id>=16 && v.unit_id<=18;
 if(!ogre && (tower || !(v.state&4u)) && v.damage_taken){
  next.healing_attempted=true;
  const bool native=in.weapon_offense<0 && v.unit_id<64;
  const bool fungus=(in.tile_items&0x20u) && (in.tile_climate&0xe0u)>0x3fu;
  const bool land=in.triad==0;const uint32_t altitude=in.tile_climate&0xe0u;
  uint32_t factor=1;
  if(!tower){
   if(native && fungus)factor=2;
   if(in.friendly_territory)++factor;
   if((in.triad==2 && (in.tile_items&0x40000u)) || (land && (in.tile_items&0x800u)))++factor;
  }
  if(in.base_present && !in.base_riots){
   ++factor;
   if(native?in.native_breed_bonus:(in.repair_facility || (in.repair_project && (expansion || in.triad!=2))))factor=v.damage_taken;
  }
  const bool bay=land && in.cargo_capacity_nonzero && (v.order==1 || altitude<0x60u) && in.other_stack_repair_bay;
  if(bay)factor*=2;
  if(in.nano_factory)factor=v.damage_taken;
  uint32_t healing=(uint32_t)in.reactor*factor,floor=0;
  if(in.phase_faction!=0 && !bay && !in.base_present && !in.nano_factory){
   floor=(uint32_t)in.reactor*2;
   if(in.reactor && fungus){
    if(native)floor=0;
    if(in.xenoempathy_dome){floor=0;healing+=(uint32_t)in.reactor;}
   }
  }
  int32_t remaining=(int32_t)v.damage_taken-(int32_t)healing;
  if(remaining>999)remaining=999;
  if(remaining<(int32_t)floor)remaining=(int32_t)floor;
  if(remaining>v.damage_taken)remaining=v.damage_taken;
  next.healing=healing;next.damage_floor=floor;v.damage_taken=(uint8_t)remaining;
  if(v.damage_taken<=floor && in.human && v.order==1 && v.damage_taken!=in.vehicle.damage_taken && (!land || altitude>=0x60u))v.order=0;
 }
 next.draw_requested=v.faction_id==in.phase_faction && !physical_base &&
   (v.faction_id==in.local_faction || (v.visibility&(1u<<in.local_faction)));
 out=next;return LogicError::ok;
}
}
