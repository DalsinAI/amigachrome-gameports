/* SPDX-License-Identifier: GPL-3.0-or-later. See header for original semantics. */
#include "world_unit_readiness.h"
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
}
LogicError world_unit_readiness(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,
 size_t id,int32_t human,int32_t road,UnitReadiness&out){
 if(!w.loaded() || tech.count!=89 || road<1 || road>100 ||
    !((w.game().data_version<11 && units.weapon_count==21 && units.armor_count==10 && units.ability_count==24) ||
      (w.game().data_version>=11 && units.weapon_count==26 && units.armor_count==14 && units.ability_count==29)) || human<1 || human>7 || !disjoint(w,&out,sizeof out) ||
    detail::effects_range_overlap(&out,sizeof out,&units,sizeof units) ||
    detail::effects_range_overlap(&out,sizeof out,&tech,sizeof tech))return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v)return LogicError::bad_state;
 UnitReadiness next{};
 const auto&map=w.game().map.info;
 if(v->x<0 || v->y<0 || (uint32_t)v->x>=map.width || (uint32_t)v->y>=map.height || ((v->x+v->y)&1))return LogicError::bad_state;
 if(v->faction_id!=human || (v->flags&0x440u)==0x440u){out=next;return LogicError::ok;}
 const auto*p=w.prototype((size_t)v->unit_id);
 if(!p || p->chassis_id>=9)return LogicError::bad_state;
 const int triad=units.chassis[p->chassis_id].triad;
 if(triad<0 || triad>2)return LogicError::bad_state;
 if(triad==0 && v->order==1 && v->waypoint_x[0]>=0){
  const auto*carrier=w.vehicle((size_t)v->waypoint_x[0]);if(!carrier)return LogicError::bad_state;
  const auto*design=w.prototype((size_t)carrier->unit_id);if(!design || design->chassis_id>=9)return LogicError::bad_state;
  const int carrier_triad=units.chassis[design->chassis_id].triad;if(carrier_triad<0 || carrier_triad>2)return LogicError::bad_state;
  if(carrier_triad==2){
   const auto*t=w.tile(((size_t)v->y*map.width+(uint32_t)v->x)/2);if(!t)return LogicError::bad_state;
   const unsigned physical=t->val2&15u;
   if(!((t->items&1u)&&physical>=1&&physical<=7) && !(t->items&0x40000u)){next.jailed=true;out=next;return LogicError::ok;}
  }
 }
 if(!(v->state&0x4200u) && v->order>=1 && v->order<=3){out=next;return LogicError::ok;}
 uint32_t speed=0;const auto error=world_vehicle_speed(w,units,tech,id,road,false,speed);if(error!=LogicError::ok)return error;
 next.speed_evaluated=true;next.speed=speed;next.ready=speed>v->moves_spent;out=next;return LogicError::ok;
}
}
