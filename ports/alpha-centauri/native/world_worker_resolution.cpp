/* SPDX-License-Identifier: GPL-3.0-or-later. Policy attribution in header. */
#include "world_worker_resolution.h"
#include "world_terraform_ecology.h"
#include "world_unit_stats.h"
#include "movement.h"
namespace alpha {namespace {
const int32_t dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,const WorkerResolutionStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||overlap(out,n,&c,sizeof c)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
unsigned bits(uint32_t mask){unsigned n=0;while(mask){mask&=mask-1;++n;}return n;}
void set_type(ProposedCitizenAssignment&a,unsigned slot,uint32_t type){if(slot>=16)return;const unsigned shift=4*(slot&7);a.specialist_types[slot/8]=(a.specialist_types[slot/8]&~(15u<<shift))|(type<<shift);}
StableIntakeError energy_error(EnergyError e){return e==EnergyError::overflow?StableIntakeError::overflow:e==EnergyError::bad_buffer?StableIntakeError::bad_buffer:StableIntakeError::bad_state;}
StableIntakeError psych_error(PsychError e){return e==PsychError::overflow?StableIntakeError::overflow:e==PsychError::out_of_memory?StableIntakeError::out_of_memory:e==PsychError::unsupported?StableIntakeError::unsupported_stage:e==PsychError::bad_buffer?StableIntakeError::bad_buffer:StableIntakeError::bad_state;}
StableIntakeError intake_for(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const WorkerResolutionStage&s,WorkerResolutionResult&next){
 const auto*base=w.base(id);const auto&candidate=next.candidate;const auto&a=next.assignment;
 if(!base||!(a.worked_tiles&1u)||(a.worked_tiles&~candidate.facts.available)||(int32_t)bits(a.worked_tiles)+a.specialist_count!=a.population+1)return StableIntakeError::bad_state;
 StableIntakeResult fresh{};for(unsigned k=0;k<3;++k)fresh.satellites[k]=candidate.intake.satellites[k];fresh.worked_tiles=a.worked_tiles;fresh.state_flags=(uint32_t)base->state_flags&~0x8100u;fresh.base_energy_after=candidate.convoys.base_energy_after;
 const auto&g=w.game();const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};int32_t reduction=candidate.convoys.terraform_reduction_after;
 for(unsigned i=0;i<21;++i){if(!(a.worked_tiles&(1u<<i)))continue;int32_t x=base->x+dx[i];const int32_t y=base->y+dy[i];if(!geometry.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}size_t tile;
  if(movement_tile_index(geometry,x,y,tile)!=LogicError::ok)return StableIntakeError::bad_state;
  if((w.tile(tile)->climate&0xe0u)<0x60u)fresh.state_flags|=0x8000u;
  WorldTileYieldStage ts{Variant::base,false,false,s.worker.control_upkeep,(int32_t)id,reduction,fresh.base_energy_after};TileYieldResult yield{};
  const auto error=(w.tile(tile)->items&0x20u)?world_fresh_tile_yields(w,t,b,r,id,base->faction_id,x,y,ts,yield):world_tile_yields(w,t,b,r,id,base->faction_id,x,y,ts,yield);
  if(error!=TileYieldError::ok)return error==TileYieldError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
  const int32_t values[]={yield.nutrient,yield.mineral,yield.energy};for(unsigned k=0;k<3;++k){fresh.tile_yields[i][k]=values[k];if(!put((int64_t)fresh.tiles[k]+values[k],fresh.tiles[k]))return StableIntakeError::overflow;}fresh.base_energy_after=yield.base_energy;reduction=0;
 }
 for(unsigned k=0;k<3;++k)if(!put((int64_t)fresh.satellites[k]+fresh.tiles[k],fresh.intake[k]))return StableIntakeError::overflow;
 fresh.terraform_reduction_after=reduction;TerraformEcologyResult ecology{};if(world_terraform_ecology_for_workers(w,id,a.worked_tiles,ecology)!=TerraformEcologyError::ok)return StableIntakeError::bad_state;fresh.raw_eco_damage=ecology.raw_damage;next.intake=fresh;return StableIntakeError::ok;
}
StableIntakeError evaluate(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,size_t id,const WorkerResolutionStage&s,WorkerResolutionResult&next){
 auto error=intake_for(w,t,b,r,id,s,next);if(error!=StableIntakeError::ok)return error;
 const auto*base=w.base(id);const auto*owner=w.faction(base->faction_id);const auto&convoy=next.candidate.convoys;
 const CurrentMineralInputs mi{Variant::base,next.intake.intake[1],convoy.to[1],convoy.from[1],next.candidate.support.mineral_cost,next.intake.raw_eco_damage};error=world_mineral_accounting(w,wr,id,mi,next.mineral);if(error!=StableIntakeError::ok)return error;
 const NutrientFacts nf{base->pop_size,owner->SE_growth_pending,next.intake.intake[0],convoy.to[0],convoy.from[0],b.nutrient_req_citizen,base->nutrients_accumulated,owner->nutrient_surplus_total,1u,(base->facilities_built[0]&4u?1u:0u)|((uint32_t)base->state_flags&4u?2u:0u)};
 const auto food=base_nutrients(nf,next.nutrient);if(food!=NutrientError::ok)return food==NutrientError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 const WorldEnergyStage es{next.intake.intake[2],convoy.to[2],s.commerce,s.energy_grid,convoy.from[2],1};auto energy=world_energy_accounting(w,id,es,next.energy);if(energy!=EnergyError::ok)return energy_error(energy);
 energy=world_energy_allocation_for_assignment(w,t,b,c,id,next.assignment,next.energy.accounting.surplus,s.commerce,next.allocation);if(energy!=EnergyError::ok)return energy_error(energy);
 next.assignment.specialist_types[0]=next.allocation.allocation.specialist_types[0];next.assignment.specialist_types[1]=next.allocation.allocation.specialist_types[1];
 const WorldPsychStage ps{next.assignment.specialist_count,next.allocation.allocation.psych};const auto psych=world_base_psych_policy(w,u,b,id,ps,next.psych);if(psych!=PsychError::ok)return psych_error(psych);
 next.requires_riot_event=next.psych.psych.will_riot;return StableIntakeError::ok;
}
}
StableIntakeError world_worker_resolution(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,size_t id,const WorkerResolutionStage&s,WorkerResolutionResult&output){
 if(aliases(w,t,u,b,wr,r,c,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(s.commerce<0||s.energy_grid<0||c.count!=7)return StableIntakeError::bad_state;
 WorkerResolutionResult next{};auto error=world_worker_upkeep_candidate(w,t,u,b,r,id,s.worker,next.candidate);if(error!=StableIntakeError::ok)return error;
 const auto*base=w.base(id);const auto*owner=w.faction(base->faction_id);if(!owner||owner->SE_alloc_labs<0||owner->SE_alloc_labs>10||owner->SE_alloc_psych<0||owner->SE_alloc_psych>10-owner->SE_alloc_labs)return StableIntakeError::bad_state;
 const auto&candidate=next.candidate;const auto&facts=candidate.facts;const int32_t econ=10-owner->SE_alloc_labs-owner->SE_alloc_psych;
 const bool need_labs=!((uint32_t)w.game().preferences.GameRules&0x4000000u)&&!(base->facilities_built[23/8]&(1u<<(23%8)));
 const int32_t econ_weight=4+(econ>owner->SE_alloc_labs),labs_weight=2+2*(need_labs&&owner->SE_alloc_labs>0)+(econ<=owner->SE_alloc_labs),psych_weight=2+2*facts.can_riot+4*(facts.can_riot&&owner->SE_police_pending<=-3&&candidate.support.pacifism_units>0);
 bool eligible[7]{};int best=-1;int32_t best_score=(-2147483647-1);
 for(unsigned i=0;i<7;++i){const auto&citizen=c.specialists[i];if(citizen.economy<0||citizen.psych<0||citizen.labs<0)return StableIntakeError::bad_state;bool known=false,obsolete=false;
  if(world_has_technology(w,t,citizen.prerequisite,base->faction_id,known)!=LogicError::ok||world_has_technology(w,t,citizen.obsolete,base->faction_id,obsolete)!=LogicError::ok)return StableIntakeError::bad_state;
  eligible[i]=known&&!obsolete&&(citizen.psych>=2||base->pop_size>=b.min_base_size_specialists);if(!eligible[i])continue;
  int32_t score;if(!put((int64_t)econ_weight*citizen.economy+(int64_t)labs_weight*citizen.labs+(int64_t)psych_weight*citizen.psych,score))return StableIntakeError::overflow;if(best<0||score>best_score){best_score=score;best=(int)i;}
 }
 if(best<0)return StableIntakeError::needs_allocation;
 auto&a=next.assignment;a.population=base->pop_size;a.specialist_count=candidate.allocation.specialists;a.worked_tiles=candidate.allocation.worked;a.specialist_types[0]=(uint32_t)base->specialist_types[0];a.specialist_types[1]=(uint32_t)base->specialist_types[1];
 for(unsigned i=0;i<16;++i){const unsigned type=(a.specialist_types[i/8]>>(4*(i&7)))&15u;if(!facts.worked||type>6||!eligible[type])set_type(a,i,(uint32_t)best);}
 // Revisit the greedy prefix at the exact specialist-break boundary. The
 // candidate's future choices do not affect scores before the first break.
 for(uint32_t j=0;j<candidate.allocation.added_count;++j)a.worked_tiles&=~(1u<<candidate.allocation.added[j]);
 int32_t remaining=(int32_t)candidate.allocation.added_count+candidate.allocation.unallocated,N=facts.nutrient_before,M=facts.mineral_before,E=facts.energy_before;
 for(unsigned i=0;i<21;++i)if(a.worked_tiles&(1u<<i)){if(!put((int64_t)N+facts.yields[i][0],N)||!put((int64_t)M+facts.yields[i][1],M)||!put((int64_t)E+facts.yields[i][2],E))return StableIntakeError::overflow;}
 uint32_t choices[20]{},choice_count=0;const auto&spc=c.specialists[best];const int Wenergy=1+(facts.efficiency>=6);int Wmineral=3+(((uint32_t)base->governor_flags&0x4000000u)!=0)+(((uint32_t)base->governor_flags&0x8000000u)!=0);Wmineral+=(Wmineral==5);
 for(uint32_t j=0;j<candidate.allocation.added_count;++j){const unsigned i=candidate.allocation.added[j];const auto&yield=facts.yields[i];const int Wnutrient=1+(facts.can_grow||N<0||M<2)+(a.population<b.min_base_size_specialists+2);
  int32_t specialist_value,tile_value;if(!put(2LL*((int64_t)spc.labs+spc.economy)+(facts.can_riot?2LL:1LL)*spc.psych,specialist_value)||!put((int64_t)Wnutrient*yield[0]+(int64_t)Wmineral*yield[1]+(int64_t)Wenergy*yield[2],tile_value))return StableIntakeError::overflow;
  if(specialist_value>tile_value&&N>=(facts.can_grow?facts.nutrient_threshold:0)&&M>=2&&((int64_t)M+candidate.support.mineral_cost>=((int64_t)a.population*Wmineral+5)/2||(Wmineral<5&&a.specialist_count+remaining<a.population/2&&(int64_t)yield[0]+yield[1]+yield[2]<=2)))break;
  a.worked_tiles|=1u<<i;choices[choice_count++]=i;--remaining;if(!put((int64_t)N+yield[0],N)||!put((int64_t)M+yield[1],M)||!put((int64_t)E+yield[2],E))return StableIntakeError::overflow;
 }
 a.specialist_count+=remaining;
 for(;;){error=evaluate(w,t,u,b,wr,r,c,id,s,next);if(error!=StableIntakeError::ok)return error;
  if(!facts.can_riot||!next.psych.psych.will_riot||!choice_count)break;
  const unsigned last=choices[choice_count-1];if((int64_t)next.mineral.minerals.surplus-facts.yields[last][1]<0)break;
  --choice_count;a.worked_tiles&=~(1u<<last);++a.specialist_count;++next.specialist_adjust;++next.trimmed_workers;
 }
 output=next;return StableIntakeError::ok;
}
}
