/* SPDX-License-Identifier: GPL-3.0-or-later. See world_repair_vehicle.h. */
#include "world_repair_vehicle.h"
#include "world_unit_stats.h"
#include "movement.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool alias(const OriginalWorld&w,const UnitDefinitions&d,const Rules&r,const RepairVehicleResult&out){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 const void*p=&out;const size_t n=sizeof out;const auto&g=w.game();
 return effects_range_overlap(p,n,&w,sizeof w)||effects_range_overlap(p,n,&d,sizeof d)||effects_range_overlap(p,n,&r,sizeof r)||w.overlaps_byte_storage(p,n)||
 effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||
 effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||
 effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||
 effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||
 effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||
 effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
bool fac(const OriginalBase&b,unsigned id){return (b.facilities_built[id/8]&(1u<<(id%8)))!=0;}
LogicError repair_bay(const OriginalWorld&w,const UnitDefinitions&d,const Rules&r,int32_t id,bool&out){
 if(id<0 || id>=512)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)id);if(!p)return LogicError::bad_state;
 if(p->ability_flags&0x200000u){out=true;return LogicError::ok;}
 out=false;if(w.game().data_version<11 || id<64)return LogicError::ok;
 const auto*f=w.faction_definition((size_t)id/64);if(!f || f->faction_bonus_count<0 || f->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){
  const int32_t bit=f->faction_bonus_val1[i];if(bit<0 || (uint32_t)bit>=d.ability_count)return LogicError::bad_state;
  if(bit!=21)continue;
  bool known;const auto e=world_has_technology(w,r,d.abilities[21].prerequisite,id/64,known);if(e!=LogicError::ok)return e;
  if(known){out=true;return LogicError::ok;}
 }
 return LogicError::ok;
}
LogicError stack_bay(const OriginalWorld&w,const UnitDefinitions&d,const Rules&r,size_t id,bool&out){
 const auto*v=w.vehicle(id);if(!v)return LogicError::bad_state;
 const size_t count=w.game().vehicle_count;size_t top=id,steps=0;
 while(w.vehicle(top)->prev_veh_id_stack>=0){
  if(++steps>=count)return LogicError::bad_state;
  const int32_t prev=w.vehicle(top)->prev_veh_id_stack;const auto*p=w.vehicle((size_t)prev);
  if(!p || p->next_veh_id_stack!=(int32_t)top || p->x!=v->x || p->y!=v->y)return LogicError::bad_state;
  top=(size_t)prev;
 }
 if(w.vehicle(top)->prev_veh_id_stack!=-1)return LogicError::bad_state;
 steps=0;int32_t previous=-1;bool found=false,bay=false;
 for(int32_t current=(int32_t)top;current>=0;){
  if(++steps>count)return LogicError::bad_state;
  const auto*p=w.vehicle((size_t)current);
  if(!p || p->prev_veh_id_stack!=previous || p->x!=v->x || p->y!=v->y)return LogicError::bad_state;
  if((size_t)current==id)found=true;
  else {bool present;const auto e=repair_bay(w,d,r,p->unit_id,present);if(e!=LogicError::ok)return e;bay|=present;}
  previous=current;current=p->next_veh_id_stack;if(current<-1)return LogicError::bad_state;
 }
 size_t colocated=0;for(size_t i=0;i<count;++i){const auto*p=w.vehicle(i);if(p->x==v->x && p->y==v->y)++colocated;}
 if(!found || steps!=colocated)return LogicError::bad_state;
 out=bay;return LogicError::ok;
}
}
LogicError world_repair_vehicle(const OriginalWorld&w,const UnitDefinitions&d,const Rules&r,Variant profile,int32_t phase,int32_t local,size_t id,RepairVehicleResult&out){
 if(alias(w,d,r,out) || !w.loaded() || phase<0 || phase>7 || local<0 || local>7 ||
    (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    ((w.game().data_version<11)!=(profile==Variant::base)) || r.count!=89 ||
    d.weapon_count!=(profile==Variant::base?21u:26u) || d.ability_count!=(profile==Variant::base?24u:29u))return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v)return LogicError::bad_state;
 RepairVehicleFacts facts{};facts.variant=profile;facts.phase_faction=(uint32_t)phase;facts.local_faction=(uint32_t)local;
 facts.turn=(uint32_t)w.game().preferences.CurrentTurn;facts.vehicle_index=(uint32_t)id;facts.vehicle=*v;
 if(v->faction_id!=phase)return repair_vehicle(facts,out);
 if(v->unit_id<0 || v->unit_id>=512)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);if(!p || p->chassis_id>=9 || p->weapon_id>=d.weapon_count)return LogicError::bad_state;
 facts.triad=d.chassis[p->chassis_id].triad;facts.reactor=p->reactor_id;facts.weapon_offense=d.weapons[p->weapon_id].offense;
 if(facts.triad<0 || facts.triad>2 || facts.weapon_offense<-128 || facts.weapon_offense>127)return LogicError::bad_state;
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};
 size_t ti;if(movement_tile_index(geometry,v->x,v->y,ti)!=LogicError::ok)return LogicError::bad_state;
 const auto*t=w.tile(ti);if(!t)return LogicError::bad_state;
 facts.tile_items=t->items;facts.tile_climate=t->climate;facts.tile_val2=t->val2;
 const bool tower=profile==Variant::alien_crossfire && v->unit_id==19;
 const bool ogre=profile==Variant::alien_crossfire && v->unit_id>=16 && v->unit_id<=18;
 if(!ogre && (tower || !(v->state&4u)) && v->damage_taken){
  // whose_territory(phase,x,y,null,0) can return phase only for positive
  // owner==phase. Foreign treaties never turn a foreign owner into phase.
  facts.friendly_territory=phase>0 && t->owner==phase;
  const OriginalBase* base=nullptr;
  if(t->items&1u){for(size_t i=0;i<w.game().base_count;++i){const auto*b=w.base(i);if(b->x==v->x && b->y==v->y){base=b;break;}}
   if(!base)return LogicError::bad_state; // reject broken base bits, no fixup/UI
  }
  facts.base_present=base!=nullptr;
  auto project=[&](size_t index,int32_t faction,bool&present){return world_has_project(w,index,faction,present);};
  if(base){
   facts.base_riots=(base->state_flags&2u)!=0;
   if(!facts.base_riots){
    const bool native=facts.weapon_offense<0 && v->unit_id<64;
    if(native){
     facts.native_breed_bonus=fac(*base,31)||fac(*base,32)||fac(*base,9)||fac(*base,30);
     const size_t indices[]={8u,16u,31u,17u};for(size_t index:indices){bool present;const auto e=project(index,base->faction_id,present);if(e!=LogicError::ok)return e;facts.native_breed_bonus|=present;}
    }else{
     facts.repair_facility=fac(*base,(unsigned)(27+facts.triad));
     if(!facts.repair_facility && (facts.triad!=2 || profile==Variant::alien_crossfire)){const size_t index=facts.triad==0?1:facts.triad==1?10:35;const auto e=project(index,phase,facts.repair_project);if(e!=LogicError::ok)return e;}
    }
   }
  }
  // Original veh_cargo grows eligible native predefined capacities by morale.
  // Only nonzero matters here, so declared nonzero is exactly equivalent.
  facts.cargo_capacity_nonzero=p->carry_capacity!=0;
  uint8_t order=v->order;if((order==1 || order==2) && v->waypoint_y[0]==1)order=0;
  if(facts.triad==0 && facts.cargo_capacity_nonzero && (order==1 || (t->climate&0xe0u)<0x60u)){
   const auto e=stack_bay(w,d,r,id,facts.other_stack_repair_bay);if(e!=LogicError::ok)return e;
  }
  auto e=project(22,phase,facts.nano_factory);if(e!=LogicError::ok)return e;
  if(phase!=0 && !facts.other_stack_repair_bay && !facts.base_present && !facts.nano_factory && p->reactor_id && (t->items&0x20u) && (t->climate&0xe0u)>0x3fu){e=project(8,phase,facts.xenoempathy_dome);if(e!=LogicError::ok)return e;}
  facts.human=(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)phase))!=0;
 }
 return repair_vehicle(facts,out);
}
}
