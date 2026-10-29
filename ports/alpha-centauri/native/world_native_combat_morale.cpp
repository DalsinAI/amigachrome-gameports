/* SPDX-License-Identifier: GPL-3.0-or-later. See native_combat_morale.h. */
#include "native_combat_morale.h"
#include "movement.h"
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
LogicError world_native_combat_morale(const OriginalWorld&w,size_t actor,int32_t opponent,uint32_t&out){
 if(!w.loaded()||opponent< -1||opponent>7||!disjoint(w,&out,sizeof out))return LogicError::bad_state;
 const auto*v=w.vehicle(actor);if(!v||v->unit_id<0||v->unit_id>=512)return LogicError::bad_state;
 const bool expansion=w.game().data_version>=11;
 NativeCombatMoraleContext c{w.game().preferences.CurrentTurn,v->unit_id,opponent,0,0,v->state,v->flags,0};
 if(opponent>0){const auto*f=w.faction((size_t)opponent);if(!f)return LogicError::bad_state;c.major_atrocities=f->major_atrocities;
  if(expansion){const auto span=w.game().tectonic;if(!span.data||span.size!=32)return LogicError::bad_state;const auto*p=span.data+4*opponent;const uint32_t raw=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);c.tectonic_detonations=raw!=0?1:0;}
 }
 if(expansion&&v->unit_id==19){const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};const int32_t dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};
  for(unsigned i=0;i<8;++i){int32_t x;if(movement_wrap_x(g,v->x+dx[i],x)!=LogicError::ok)return LogicError::bad_state;size_t at;if(movement_tile_index(g,x,v->y+dy[i],at)!=LogicError::ok)continue;const auto*t=w.tile(at);if(!t)return LogicError::bad_state;if((t->items&0x20u)&&(t->climate&0xe0u)>=0x40u)++c.fungus_neighbors;}
 }
 return native_combat_morale(expansion?Variant::alien_crossfire:Variant::base,c,out);
}
}
