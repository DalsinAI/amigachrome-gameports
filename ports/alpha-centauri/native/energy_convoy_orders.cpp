/* SPDX-License-Identifier: GPL-3.0-or-later. See energy_convoy_orders.h. */
#include "energy_convoy_orders.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool aliases(const OriginalWorld& w,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return w.overlaps_byte_storage(out,n) ||
 effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError energy_convoy_orders(const OriginalWorld&w,Variant profile,size_t base,int32_t surplus,EnergyConvoyOrders&out){
 if(aliases(w,&out,sizeof out) || !w.loaded() ||
    (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    ((w.game().data_version>=11)!=(profile==Variant::alien_crossfire)))return LogicError::bad_state;
 const auto*b=w.base(base);const auto&g=w.game();
 if(!b || b->faction_id<0 || b->faction_id>7 || g.vehicle_count>32767)return LogicError::bad_state;
 EnergyConvoyOrders next{};
 if(surplus<0)for(size_t i=0;i<g.vehicle_count;++i){const auto*v=w.vehicle(i);++next.visited;
  if(v->faction_id!=(unsigned)b->faction_id)continue;
  const auto*p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
  if(p->plan!=10 || v->order!=3 || v->order_auto_type!=2)continue;
  if(v->x<0 || v->y<0 || (uint32_t)v->x>=g.map.info.width || (uint32_t)v->y>=g.map.info.height || ((v->x+v->y)&1))return LogicError::bad_state;
  const auto*t=w.tile((size_t)v->y*(g.map.info.width/2)+(size_t)v->x/2);if(!t)return LogicError::bad_state;
  if((t->items&1u) && (t->val2&15u)<8){next.clear_order[i/8]|=(uint8_t)(1u<<(i%8));++next.changed_orders;}
 }
 out=next;return LogicError::ok;
}
}
