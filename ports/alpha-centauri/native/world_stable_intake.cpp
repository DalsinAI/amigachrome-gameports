/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in corresponding header. */
#include "world_stable_intake.h"
#include "world_terraform_ecology.h"
#include "world_unit_stats.h"
#include "movement.h"
#include "world_fresh_tile_yields.h"
namespace alpha {namespace {
const int32_t dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const StableIntakeStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size()) || overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)) || overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)) || overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase)) || overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype)) || overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState)) || overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool add(int32_t a,int32_t b,int32_t&out){const int64_t n=(int64_t)a+b;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
unsigned popcount(uint32_t n){unsigned count=0;while(n){n&=n-1;++count;}return count;}
}
StableIntakeError world_stable_intake(const OriginalWorld&w,const Rules&t,const BasicRules&basic,const ResourceInfo&resources,size_t id,const StableIntakeStage&stage,StableIntakeResult&output){
 if(aliases(w,t,basic,resources,stage,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded() || (stage.variant!=Variant::base&&stage.variant!=Variant::alien_crossfire) || (stage.variant==Variant::base&&stage.expansion_enabled))return StableIntakeError::bad_state;
 if(stage.worker.upkeep_state!=0||stage.worker.editor_active)return StableIntakeError::unsupported_stage;
 const auto*base=w.base(id);if(!base||id>2147483647u||base->faction_id<0||base->faction_id>7||base->pop_size<0)return StableIntakeError::bad_state;
 const auto&g=w.game();const int32_t faction=base->faction_id;
 if(!(g.preferences.FactionStatus[0]&(1u<<(unsigned)faction)) || (((uint32_t)base->governor_flags&0x80000040u)==0x80000040u))return StableIntakeError::unsupported_stage;
 const uint32_t worked=(uint32_t)base->worked_tiles;
 // Directed-human original path preserves a coherent existing specialist
 // arrangement. Nonzero adjustment requires separate prefix/state handling.
 const int32_t specialists=base->specialist_total;
 if(specialists<0 || specialists>base->pop_size || (specialists && base->specialist_adjust!=0) ||
    !(worked&1u) || (worked&~0x1fffffu) ||
    popcount(worked)+(unsigned)specialists!=(unsigned)base->pop_size+1)return StableIntakeError::needs_allocation;
 WorkerRadiusResult radius{};const auto availability=world_worker_radius(w,id,stage.worker,radius);
 if(availability==WorkerRadiusError::bad_stack)return StableIntakeError::bad_stack;
 if(availability==WorkerRadiusError::out_of_memory)return StableIntakeError::out_of_memory;
 if(availability!=WorkerRadiusError::ok)return StableIntakeError::bad_state;
 if(worked&radius.unavailable&~1u)return StableIntakeError::needs_allocation;
 const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};size_t centre;
 if(movement_tile_index(geometry,base->x,base->y,centre)!=LogicError::ok)return StableIntakeError::bad_state;
 const auto*home=w.tile(centre);if(!home || !(home->items&1u) || (home->val2&15u)!=(unsigned)faction)return StableIntakeError::unsupported_stage;
 const auto*owner=w.faction((size_t)faction);if(!owner)return StableIntakeError::bad_state;
 bool full=(base->facilities_built[29/8]&(1u<<(29%8)))!=0;
 if(!full && world_has_project(w,27,faction,full)!=LogicError::ok)return StableIntakeError::bad_state;
 if(!full && stage.variant==Variant::alien_crossfire && world_has_project(w,35,faction,full)!=LogicError::ok)return StableIntakeError::bad_state;
 StableIntakeResult next{};next.worked_tiles=worked;next.state_flags=(uint32_t)base->state_flags&~0x8100u;next.base_energy_after=stage.base_energy_before;
 const int32_t counts[]={owner->satellites_nutrient,owner->satellites_mineral,owner->satellites_energy};
 for(unsigned k=0;k<3;++k){if(counts[k]<0)return StableIntakeError::bad_state;if(!full&&counts[k]==2147483647)return StableIntakeError::overflow;const int32_t value=full?counts[k]:(counts[k]+1)/2;next.satellites[k]=value<base->pop_size?value:base->pop_size;next.intake[k]=next.satellites[k];}
 int32_t reduction=stage.terraform_reduction_before;
 for(unsigned i=0;i<21;++i){if(!(worked&(1u<<i)))continue;
  int32_t x=base->x+dx[i];const int32_t y=base->y+dy[i];if(!geometry.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}
  size_t square;if(movement_tile_index(geometry,x,y,square)!=LogicError::ok)return StableIntakeError::needs_allocation;const auto*tile=w.tile(square);if(!tile)return StableIntakeError::bad_state;
  if((tile->climate&0xe0u)<0x60u)next.state_flags|=0x8000u;
  WorldTileYieldStage tile_stage{stage.variant,stage.expansion_enabled,false,stage.control_upkeep,(int32_t)id,reduction,next.base_energy_after};TileYieldResult value{};
  const auto error=world_fresh_tile_yields(w,t,basic,resources,id,faction,x,y,tile_stage,value);
  if(error==TileYieldError::overflow)return StableIntakeError::overflow;
  if(error!=TileYieldError::ok)return StableIntakeError::bad_state;
  const int32_t yields[]={value.nutrient,value.mineral,value.energy};
  for(unsigned k=0;k<3;++k){next.tile_yields[i][k]=yields[k];if(!add(next.tiles[k],yields[k],next.tiles[k]) || !add(next.intake[k],yields[k],next.intake[k]))return StableIntakeError::overflow;}
  next.base_energy_after=value.base_energy;
  // Original resource_yield channel3 resets this scratch after each square.
  reduction=0;
 }
 TerraformEcologyResult ecology{};if(world_terraform_ecology(w,id,ecology)!=TerraformEcologyError::ok)return StableIntakeError::bad_state;
 next.raw_eco_damage=ecology.raw_damage;next.terraform_reduction_after=reduction;output=next;return StableIntakeError::ok;
}
}
