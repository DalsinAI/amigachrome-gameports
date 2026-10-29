/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in world_unit_support.h. */
#include "world_unit_support.h"
#include "movement.h"
namespace alpha {
LogicError world_unit_support(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t base_id,UnitSupportTotals& output){
 if(!w.loaded() || !((w.game().data_version<11 && d.weapon_count==21 && d.armor_count==10 && d.ability_count==24) ||
   (w.game().data_version>=11 && d.weapon_count==26 && d.armor_count==14 && d.ability_count==29)))return LogicError::bad_state;
 const auto* base=w.base(base_id);if(!base || base->pop_size<0)return LogicError::bad_state;
 const auto* faction=w.faction(base->faction_id);if(!faction)return LogicError::bad_state;
 const int32_t rating=faction->SE_support_pending;
 if(rating>2147483643 || w.game().vehicle_count>2147483647u)return LogicError::overflow;
 int32_t slot=rating+4;slot=slot<0?0:slot>7?7:slot;
 const int32_t free_units[]={0,0,1,1,2,3,4,4};
 UnitSupportTotals next{rating,slot==7 && base->pop_size>4?base->pop_size:free_units[slot],rating< -3?2:1,0,0,0,0,0};
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};
 for(size_t i=0;i<w.game().vehicle_count;++i){
  const auto* v=w.vehicle(i);if(!v)return LogicError::bad_state;
  if(v->faction_id!=(uint8_t)base->faction_id || v->home_base_id!=(int32_t)base_id)continue;
  const auto* p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
  // Original comparison is an unsigned plan BYTE, including automatic sentinels.
  if((uint8_t)p->plan>=10)continue;
  bool clean;auto error=world_has_clean_reactor(w,d,tech,(size_t)v->unit_id,clean);if(error!=LogicError::ok)return error;
  if(clean){++next.clean_exempt;continue;}
  const bool native=v->unit_id<64 && (d.weapons[p->weapon_id].offense<0 || (d.weapon_count==26 && v->unit_id==15));
  if(native){size_t index;if(movement_tile_index(geometry,v->x,v->y,index)!=LogicError::ok)return LogicError::bad_state;
   const auto* tile=w.tile(index);if(!tile)return LogicError::bad_state;
   if((tile->items&0x20u) && (tile->climate&0xe0u)>=0x40u){++next.native_fungus_exempt;continue;}
  }
  ++next.supported_units;
 }
 next.maintained_units=next.supported_units>next.free_units?next.supported_units-next.free_units:0;
 const int64_t cost=(int64_t)next.maintained_units*next.cost_per_unit;if(cost>2147483647)return LogicError::overflow;
 next.mineral_cost=(int32_t)cost;output=next;return LogicError::ok;
}
}
