/* SPDX-License-Identifier: GPL-3.0-or-later. See node_artifacts.h. */
#include "node_artifacts.h"
#include "technology_effects_ranges.h"
#include "vehicle_stack.h"
namespace alpha {namespace {
bool aliases(const OriginalWorld& w,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return effects_range_overlap(out,n,w.original_bytes(),w.original_size()) ||
 effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError node_artifacts(const OriginalWorld& w,Variant profile,NodeRuntime runtime,size_t base,NodeArtifacts& output){
 if(aliases(w,&output,sizeof output) || !w.loaded() || runtime!=NodeRuntime::owned_offline ||
 (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
 ((w.game().data_version>=11)!=(profile==Variant::alien_crossfire)))return LogicError::bad_state;
 const auto* b=w.base(base);if(!b || !(b->facilities_built[1]&1u))return LogicError::bad_state;
 const auto& g=w.game();if(g.vehicle_count>32767)return LogicError::bad_state;
 VehicleStackStorage storage{(int32_t)g.map.info.width,(int32_t)g.map.info.height,
 const_cast<OriginalMapTile*>(w.tile(0)),g.map.info.tile_count,
 const_cast<OriginalVehicle*>(w.vehicle(0)),g.vehicle_count};
 if(validate_vehicle_stacks(storage)!=StackError::ok)return LogicError::bad_state;
 NodeArtifacts next{};next.state_flags=(uint32_t)b->state_flags&~0xc00u;
 int32_t at=-1;
 for(size_t i=0;i<g.vehicle_count;++i){const auto* v=w.vehicle(i);if(v->x==b->x && v->y==b->y){at=(int32_t)i;break;}}
 if(at>=0){while(w.vehicle((size_t)at)->prev_veh_id_stack>=0)at=w.vehicle((size_t)at)->prev_veh_id_stack;
  while(at>=0){const auto* v=w.vehicle((size_t)at);++next.visited;
   const auto* p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
   if(p->plan==12){next.clear_order[(unsigned)at/8]|=(uint8_t)(1u<<((unsigned)at%8));++next.artifacts;if(v->order)++next.changed_orders;}
   at=v->next_veh_id_stack;
  }
 }
 output=next;return LogicError::ok;
}
}
