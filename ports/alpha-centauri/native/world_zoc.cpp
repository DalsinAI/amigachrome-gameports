/* SPDX-License-Identifier: GPL-3.0-or-later. See world_zoc.h. */
#include "world_zoc.h"
#include "vehicle_stack.h"
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
LogicError world_zone_of_control(const OriginalWorld&w,int32_t x,int32_t y,int32_t faction,bool network,bool move,uint32_t&out){
 if(!w.loaded()||faction<0||faction>7||!disjoint(w,&out,sizeof out))return LogicError::bad_state;
 const auto&m=w.game().map.info;const MapGeometry g{(int32_t)m.width,(int32_t)m.height,m.flat};size_t ti;auto e=movement_tile_index(g,x,y,ti);if(e!=LogicError::ok)return e;
 // Adapter to the existing strictly read-only validation entry point. These
 // owned allocations are nonconst; validate_vehicle_stacks never writes them.
 const VehicleStackStorage storage{g.width,g.height,const_cast<OriginalMapTile*>(w.tile(0)),m.tile_count,const_cast<OriginalVehicle*>(w.vehicle(0)),w.game().vehicle_count};
 if(validate_vehicle_stacks(storage)!=StackError::ok)return LogicError::bad_state;
 const auto*tile=w.tile(ti);if(move&&(tile->items&1u)&&(tile->val2&15u)<8){out=0;return LogicError::ok;}
 const bool ocean=(tile->climate&0xe0u)<0x60u;const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};
 const auto*observer=w.faction((size_t)faction);if(!observer)return LogicError::bad_state;
 for(unsigned dir=0;dir<8;++dir){
  int32_t xx=x+dx[dir],yy=y+dy[dir];if(!m.flat){if(xx<0)xx+=g.width;else if(xx>=g.width)xx-=g.width;}
  if(xx<0||yy<0||xx>=g.width||yy>=g.height)continue;
  size_t index;e=movement_tile_index(g,xx,yy,index);if(e!=LogicError::ok)return e;const auto*t=w.tile(index);const uint32_t owner=t->val2&15u;
  if(!(t->items&2u)||owner>=8||owner==(uint32_t)faction||(((t->climate&0xe0u)<0x60u)!=ocean)||(observer->diplo_status[owner]&1u))continue;
  // Validation establishes exactly one same-coordinate linked stack. The
  // predicate is existential, so iterating these checked records is equivalent
  // to actual veh_at(root) then next links; direction ordering stays original.
  for(size_t id=0;id<w.game().vehicle_count;++id){const auto*v=w.vehicle(id);if(v->x==xx&&v->y==yy&&v->faction_id!=faction&&((v->visibility&(1u<<(uint32_t)faction))||(!network&&!(v->flags&0x400u)))){out=owner+1;return LogicError::ok;}}
 }
 out=0;return LogicError::ok;
}
}
