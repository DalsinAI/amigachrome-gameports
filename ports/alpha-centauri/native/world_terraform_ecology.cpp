/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in corresponding header. */
#include "world_terraform_ecology.h"
#include "movement.h"
namespace alpha {namespace {
const int32_t dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size()) || overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)) || overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)) || overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase)) || overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype)) || overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState)) || overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
unsigned popcount(uint32_t n){unsigned count=0;while(n){n&=n-1;++count;}return count;}
}
TerraformEcologyError world_terraform_ecology(const OriginalWorld&w,size_t id,TerraformEcologyResult&output){
 if(aliases(w,&output,sizeof output))return TerraformEcologyError::bad_buffer;
 if(!w.loaded())return TerraformEcologyError::bad_state;
 const auto*base=w.base(id);if(!base)return TerraformEcologyError::bad_state;
 return world_terraform_ecology_for_workers(w,id,(uint32_t)base->worked_tiles&0x1fffffu,output);
}
TerraformEcologyError world_terraform_ecology_for_workers(const OriginalWorld&w,size_t id,uint32_t worked,TerraformEcologyResult&output){
 if(aliases(w,&output,sizeof output))return TerraformEcologyError::bad_buffer;
 if(!w.loaded()||(worked&~0x1fffffu))return TerraformEcologyError::bad_state;
 const auto&g=w.game();const auto&m=g.map.info;
 if(m.width<2||m.width>4096||(m.width&1u)||!m.height||m.height>4096||m.tile_count!=(m.width/2)*m.height||g.base_count>512 || (uint64_t)m.tile_count*sizeof(OriginalMapTile)>w.memory_bytes() || (uint64_t)g.base_count*sizeof(OriginalBase)>w.memory_bytes())return TerraformEcologyError::bad_state;
 const auto*base=w.base(id);if(!base)return TerraformEcologyError::bad_state;
 const MapGeometry geometry{(int32_t)m.width,(int32_t)m.height,m.flat};size_t centre;
 if(movement_tile_index(geometry,base->x,base->y,centre)!=LogicError::ok)return TerraformEcologyError::bad_state;
 const bool tree=(base->facilities_built[13/8]&(1u<<(13%8)))!=0,hybrid=(base->facilities_built[14/8]&(1u<<(14%8)))!=0;
 TerraformEcologyResult next{};
 for(unsigned i=0;i<21;++i){
  int32_t x=base->x+dx[i];const int32_t y=base->y+dy[i];
  if(!m.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}
  size_t tile_id;if(movement_tile_index(geometry,x,y,tile_id)!=LogicError::ok)continue;
  const auto*tile=w.tile(tile_id);if(!tile)return TerraformEcologyError::bad_state;
  next.valid_tiles|=1u<<i;const uint32_t items=tile->items;
  int32_t damage=(int32_t)popcount(items&0x1c8805cu);
  if(worked&(1u<<i))damage*=2;
  damage+=((items&0x1000000u)?8:0)+((items&0x800000u)?6:0)+((items&0x400000u)?4:0)-((items&0x200000u)?1:0);
  if(tree&&hybrid)damage=0;
  if(tree||hybrid)damage/=2;
  next.contributions[i]=damage;next.raw_damage+=damage;
 }
 // At most21 bounded tile contributions; signed32 cannot overflow here.
 output=next;return TerraformEcologyError::ok;
}
}
