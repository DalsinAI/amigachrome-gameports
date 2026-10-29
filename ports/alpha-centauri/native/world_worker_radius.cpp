/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in world_worker_radius.h. */
#include "world_worker_radius.h"
#include "movement.h"
#include "vehicle_stack.h"
namespace alpha {namespace {
const int32_t dx[25]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0};
const int32_t dy[25]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const WorkerRadiusStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return w.overlaps_byte_storage(out,n) || overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)) || overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)) || overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase)) || overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype)) || overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState)) || overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
int32_t wrap(const MapGeometry&g,int32_t x){if(!g.flat){if(x<0)x+=g.width;else if(x>=g.width)x-=g.width;}return x;}
int32_t distance(const MapGeometry&g,int32_t ax,int32_t ay,int32_t bx,int32_t by){
 int32_t x=ax>bx?ax-bx:bx-ax,y=ay>by?ay-by:by-ay;if(!g.flat&&x>g.width/2)x=g.width-x;
 const int32_t hi=x>y?x:y,lo=x<y?x:y;return hi-(((x+y)/2-lo+1)/2);
}
}
WorkerRadiusError world_worker_radius(const OriginalWorld&w,size_t id,const WorkerRadiusStage&stage,WorkerRadiusResult&output){
 if(aliases(w,stage,&output,sizeof output))return WorkerRadiusError::bad_buffer;
 if(!w.loaded())return WorkerRadiusError::bad_state;
 if(stage.upkeep_state!=0||stage.editor_active)return WorkerRadiusError::unsupported_stage;
 const auto&g=w.game();const auto&m=g.map.info;
 if(m.width<2||m.width>4096||(m.width&1u)||!m.height||m.height>4096||m.tile_count!=(m.width/2)*m.height||g.base_count>512||g.vehicle_count>32767 || (uint64_t)m.tile_count*sizeof(OriginalMapTile)>w.memory_bytes() || (uint64_t)g.vehicle_count*sizeof(OriginalVehicle)>w.memory_bytes() || (uint64_t)g.base_count*sizeof(OriginalBase)>w.memory_bytes())return WorkerRadiusError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<0||base->faction_id>7)return WorkerRadiusError::bad_state;
 const auto*faction=w.faction(base->faction_id);if(!faction)return WorkerRadiusError::bad_state;
 const MapGeometry geometry{(int32_t)m.width,(int32_t)m.height,m.flat};size_t centre;
 if(movement_tile_index(geometry,base->x,base->y,centre)!=LogicError::ok)return WorkerRadiusError::bad_state;
 VehicleStackStorage storage{geometry.width,geometry.height,const_cast<OriginalMapTile*>(w.tile(0)),m.tile_count,const_cast<OriginalVehicle*>(w.vehicle(0)),g.vehicle_count};
 const auto topology=validate_vehicle_stacks(storage);if(topology==StackError::out_of_memory)return WorkerRadiusError::out_of_memory;if(topology!=StackError::ok)return WorkerRadiusError::bad_stack;
 WorkerRadiusResult next{};const uint32_t bit=1u<<(unsigned)base->faction_id;
 for(unsigned i=0;i<25;++i){
  if(i>=21){next.flags[i]=1;continue;}
  const int32_t x=wrap(geometry,base->x+dx[i]),y=base->y+dy[i];size_t tile_id;
  if(movement_tile_index(geometry,x,y,tile_id)!=LogicError::ok){next.flags[i]=2;continue;}
  const auto*tile=w.tile(tile_id);
  if(!(faction->player_flags&0x200u)&&!(tile->visibility&bit)){next.flags[i]=2;continue;}
  if((tile->items&1u)&&(tile->val2&15u)<8)next.flags[i]=4;
  for(size_t v=0;v<g.vehicle_count;++v){const auto*actor=w.vehicle(v);if(actor->x!=x||actor->y!=y)continue;
   if(actor->order==3 || (actor->faction_id!=base->faction_id && (actor->visibility&bit) && !(faction->diplo_status[actor->faction_id]&3u)))next.flags[i]|=8;
  }
  if(tile->owner>7)return WorkerRadiusError::bad_state;
  int32_t owner=tile->owner;
  if(owner<1 || (owner!=base->faction_id && !(g.preferences.GameState&0x80u) && (owner>=8 || (faction->diplo_status[owner]&0x8000008u)!=0x8000008u)))owner=-1;
  // Original stack traversal ends with index -1 before this predicate. Preserve
  // its foreign-territory result; do not import later territory fixes.
  if(owner>=1 && owner!=base->faction_id)next.flags[i]|=32;
 }
 for(size_t b=0;b<g.base_count;++b){if(b==id)continue;const auto*other=w.base(b);size_t square;
  if(other->faction_id<0||other->faction_id>7||movement_tile_index(geometry,other->x,other->y,square)!=LogicError::ok)return WorkerRadiusError::bad_state;
  if(distance(geometry,base->x,base->y,other->x,other->y)>=6)continue;
  for(unsigned i=0;i<21;++i){if(i && !((uint32_t)other->worked_tiles&(1u<<i)))continue;
   const int32_t x=wrap(geometry,other->x+dx[i]),y=other->y+dy[i];size_t tile_id;
   if(movement_tile_index(geometry,x,y,tile_id)!=LogicError::ok || (other->faction_id!=base->faction_id && (w.tile(tile_id)->climate&0xe0u)>=0x60u) || distance(geometry,base->x,base->y,x,y)>=3)continue;
   for(unsigned j=0;j<21;++j)if(wrap(geometry,base->x+dx[j])==x && base->y+dy[j]==y)next.flags[j]|=16;
  }
 }
 for(unsigned i=0;i<21;++i)if(next.flags[i])next.unavailable|=1u<<i;
 output=next;return WorkerRadiusError::ok;
}
}
