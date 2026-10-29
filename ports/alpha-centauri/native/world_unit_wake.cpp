/* SPDX-License-Identifier: GPL-3.0-or-later. See local_unit_wake.h. */
#include "local_unit_wake.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool disjoint(const OriginalWorld&w,const void*p,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();
 return !effects_range_overlap(p,n,&w,sizeof w)&&!w.overlaps_byte_storage(p,n)&&
 !effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))&&
 !effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))&&
 !effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))&&
 !effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))&&
 !effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))&&
 !effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
bool valid(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const void*p,size_t n){
 return w.loaded()&&t.count==89&&
 ((w.game().data_version<11&&d.weapon_count==21&&d.armor_count==10&&d.ability_count==24)||
 (w.game().data_version>=11&&d.weapon_count==26&&d.armor_count==14&&d.ability_count==29))&&
 disjoint(w,p,n)&&!detail::effects_range_overlap(p,n,&d,sizeof d)&&!detail::effects_range_overlap(p,n,&t,sizeof t);
}
}
LogicError world_terraform_contribution(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,size_t id,int32_t terraform,uint32_t&out){
 if(!valid(w,d,t,&out,sizeof out)||terraform<0||terraform>19)return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v||v->unit_id<0||v->faction_id>7)return LogicError::bad_state;
 bool super=false;auto e=world_has_super_former(w,d,t,(size_t)v->unit_id,super);if(e!=LogicError::ok)return e;
 bool project=false;e=world_has_project(w,(terraform==10||terraform==11)?8:2,v->faction_id,project);if(e!=LogicError::ok)return e;
 uint32_t amount=super?4u:2u;if(project)amount=(terraform==10||terraform==11)?amount*2u:amount*3u/2u;
 out=amount;return LogicError::ok;
}
LogicError world_unit_wake(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,size_t id,int32_t road,LocalWakePreview&out){
 if(!valid(w,d,t,&out,sizeof out)||road<1||road>100)return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v||v->faction_id>7)return LogicError::bad_state;
 LocalWakePreview n{};n.vehicle=id;n.state_before=v->state;n.state_after=v->state&0xf4ffbdffu;n.spent_before=n.spent_after=v->moves_spent;n.turns_before=n.turns_after=v->movement_turns;n.order_before=v->order;
 // The original signed-byte comparison excludes128..255 as well as24..127.
 if(v->order>=4&&v->order<24&&!(v->state&0x04000000u)){
  auto e=world_vehicle_speed(w,d,t,id,road,false,n.speed);if(e!=LogicError::ok)return e;
  if(n.speed>255)return LogicError::overflow;
  n.speed_evaluated=true;n.spent_after=(uint8_t)(n.speed-(uint32_t)road);
  if(n.turns_before){e=world_terraform_contribution(w,d,t,id,(int32_t)v->order-4,n.contribution);if(e!=LogicError::ok)return e;
   n.turns_after=n.turns_before>n.contribution?(uint8_t)(n.turns_before-n.contribution):0;}
 }
 if((v->state&0x200u)&&v->order_auto_type==11&&!(v->state&4u))n.spent_after=0;
 out=n;return LogicError::ok;
}
}
