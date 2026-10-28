/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution and policy in corresponding header. */
#include "world_worker_upkeep.h"
#include "upkeep_world_energy.h"
#include "world_unit_stats.h"
#include "world_terraform_ecology.h"
#include "movement.h"
#include <stdlib.h>
namespace alpha {namespace {
const int32_t dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const ResourceInfo&r,const WorkerUpkeepStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool facility(const OriginalBase&b,unsigned i){return (b.facilities_built[i/8]&(1u<<(i%8)))!=0;}
struct States {uint32_t*p;~States(){free(p);}};
}
StableIntakeError world_worker_upkeep_candidate(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&basic,const ResourceInfo&resources,size_t id,const WorkerUpkeepStage&stage,WorkerUpkeepCandidate&output){
 if(aliases(w,t,u,basic,resources,stage,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded()||id>2147483647u)return StableIntakeError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<1||base->faction_id>7||base->pop_size<0||base->specialist_total<0||base->specialist_total>base->pop_size||basic.nutrient_req_citizen<0)return StableIntakeError::bad_state;
 if(stage.upkeep_state!=1||stage.editor_active||w.game().data_version>=11)return StableIntakeError::unsupported_stage;
 const auto&g=w.game();const int32_t faction=base->faction_id;
 if(!(g.preferences.FactionStatus[0]&(1u<<(unsigned)faction))||(((uint32_t)base->governor_flags&0x80000040u)==0x80000040u))return StableIntakeError::unsupported_stage;
 const auto*owner=w.faction(faction);const auto*definition=w.faction_definition(faction);if(!owner||!definition)return StableIntakeError::bad_state;
 WorkerUpkeepCandidate next{};WorkerRadiusStage radius_stage{0,false};
 const auto radius=world_worker_radius(w,id,radius_stage,next.radius);
 if(radius!=WorkerRadiusError::ok)return radius==WorkerRadiusError::bad_stack?StableIntakeError::bad_stack:radius==WorkerRadiusError::out_of_memory?StableIntakeError::out_of_memory:StableIntakeError::bad_state;
 const size_t count=g.vehicle_count;States states{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};if(count&&!states.p)return StableIntakeError::out_of_memory;
 const auto support=world_base_support_policy(w,u,id,next.support,states.p,count);
 if(support!=SupportError::ok)return support==SupportError::overflow?StableIntakeError::overflow:support==SupportError::out_of_memory?StableIntakeError::out_of_memory:support==SupportError::unsupported?StableIntakeError::unsupported_stage:StableIntakeError::bad_state;
 StableIntakeStage flow_stage{Variant::base,false,stage.control_upkeep,{0,false},0,0};
 const auto flow=world_convoy_flows(w,t,basic,resources,id,flow_stage,next.convoys);if(flow!=StableIntakeError::ok)return flow;
 const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};size_t centre;
 if(movement_tile_index(geometry,base->x,base->y,centre)!=LogicError::ok)return StableIntakeError::bad_state;
 const auto*home=w.tile(centre);if(!home||!(home->items&1u)||(home->val2&15u)!=(unsigned)faction)return StableIntakeError::unsupported_stage;
 bool full=facility(*base,29),ascetic=false,matrix=false;
 if(!full&&world_has_project(w,27,faction,full)!=LogicError::ok)return StableIntakeError::bad_state;
 if(world_has_project(w,13,faction,ascetic)!=LogicError::ok||world_has_project(w,30,faction,matrix)!=LogicError::ok)return StableIntakeError::bad_state;
 const int32_t counts[]={owner->satellites_nutrient,owner->satellites_mineral,owner->satellites_energy};
 for(unsigned k=0;k<3;++k){if(counts[k]<0)return StableIntakeError::bad_state;if(!full&&counts[k]==2147483647)return StableIntakeError::overflow;const int32_t value=full?counts[k]:(counts[k]+1)/2;next.intake.satellites[k]=value<base->pop_size?value:base->pop_size;}
 auto&facts=next.facts;facts.available=(~next.radius.unavailable&0x1fffffu)|1u;facts.worked=(uint32_t)base->worked_tiles;facts.population=base->pop_size;facts.specialists=base->specialist_total;
 if(!put((int64_t)next.intake.satellites[0]+next.convoys.to[0]-next.convoys.from[0]-(int64_t)base->pop_size*basic.nutrient_req_citizen,facts.nutrient_before)||!put((int64_t)next.intake.satellites[1]+next.convoys.to[1]-next.convoys.from[1]-next.support.mineral_cost,facts.mineral_before)||!put((int64_t)next.intake.satellites[2]+next.convoys.to[2]-next.convoys.from[2],facts.energy_before)||!put(2LL*basic.nutrient_req_citizen,facts.nutrient_threshold))return StableIntakeError::overflow;
 // Thinker clears the complete 96-byte intake/surplus region before this
 // threshold query; with positive food requirement pop_boom cannot fire here.
 const int64_t cap=facility(*base,25)?127:(facility(*base,24)?basic.pop_limit_sans_hab_dome:basic.pop_limit_sans_hab_complex)+(ascetic?2LL:0)-definition->rule_population;
 facts.can_grow=cap>base->pop_size;facts.can_riot=!base->nerve_staple_turns_left&&!matrix&&!facility(*base,23);
 facts.psych_allocation=owner->SE_alloc_psych;
 UpkeepWorldEnergyStage efficiency_stage{16,0,0,0,0,0};UpkeepWorldEnergyResult efficiency{};
 const auto energy=world_energy_accounting(w,id,efficiency_stage,efficiency);if(energy!=EnergyError::ok)return energy==EnergyError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 facts.efficiency=16-efficiency.accounting.inefficiency;
 for(unsigned i=0;i<21;++i){if(!(facts.available&(1u<<i)))continue;int32_t x=base->x+dx[i];const int32_t y=base->y+dy[i];if(!geometry.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}
  WorldTileYieldStage ts{Variant::base,false,false,stage.control_upkeep,(int32_t)id,0,0};TileYieldResult result{};
  size_t tile;if(movement_tile_index(geometry,x,y,tile)!=LogicError::ok)return StableIntakeError::bad_state;
  const auto error=(w.tile(tile)->items&0x20u)?world_upkeep_fresh_tile_yields(w,t,basic,resources,id,faction,x,y,ts,result):world_tile_yields(w,t,basic,resources,id,faction,x,y,ts,result);
  if(error!=TileYieldError::ok)return error==TileYieldError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
  facts.yields[i][0]=result.nutrient;facts.yields[i][1]=result.mineral;facts.yields[i][2]=result.energy;
 }
 const auto allocated=worker_allocation_candidate(facts,next.allocation);if(allocated!=WorkerAllocationError::ok)return allocated==WorkerAllocationError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 next.intake.worked_tiles=next.allocation.worked;next.intake.state_flags=(uint32_t)base->state_flags&~0x8100u;next.intake.base_energy_after=next.convoys.base_energy_after;
 int32_t reduction=next.convoys.terraform_reduction_after;
 for(unsigned i=0;i<21;++i){if(!(next.allocation.worked&(1u<<i)))continue;int32_t x=base->x+dx[i];const int32_t y=base->y+dy[i];if(!geometry.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}size_t tile;
  if(movement_tile_index(geometry,x,y,tile)!=LogicError::ok)return StableIntakeError::bad_state;
  if((w.tile(tile)->climate&0xe0u)<0x60u)next.intake.state_flags|=0x8000u;
  WorldTileYieldStage ts{Variant::base,false,false,stage.control_upkeep,(int32_t)id,reduction,next.intake.base_energy_after};TileYieldResult result{};
  const auto error=(w.tile(tile)->items&0x20u)?world_upkeep_fresh_tile_yields(w,t,basic,resources,id,faction,x,y,ts,result):world_tile_yields(w,t,basic,resources,id,faction,x,y,ts,result);if(error!=TileYieldError::ok)return error==TileYieldError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
  const int32_t values[]={result.nutrient,result.mineral,result.energy};for(unsigned k=0;k<3;++k){next.intake.tile_yields[i][k]=values[k];if(!put((int64_t)next.intake.tiles[k]+values[k],next.intake.tiles[k]))return StableIntakeError::overflow;}
  next.intake.base_energy_after=result.base_energy;reduction=0;
 }
 for(unsigned k=0;k<3;++k)if(!put((int64_t)next.intake.satellites[k]+next.intake.tiles[k],next.intake.intake[k]))return StableIntakeError::overflow;
 next.intake.terraform_reduction_after=reduction;TerraformEcologyResult ecology{};if(world_terraform_ecology_for_workers(w,id,next.allocation.worked,ecology)!=TerraformEcologyError::ok)return StableIntakeError::bad_state;next.intake.raw_eco_damage=ecology.raw_damage;
 output=next;return StableIntakeError::ok;
}
}
