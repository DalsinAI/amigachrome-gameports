/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_human_allocation.h"
#include "world_fresh_tile_yields.h"
#include "world_terraform_ecology.h"
#include "technology_effects_ranges.h"
#include "movement.h"
#include <stdlib.h>
namespace alpha { namespace {
const int32_t dx[21]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
const int32_t dy[21]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const UnitDefinitions&u,const CitizenDefinitions&c,const StableIntakeStage&s,const void*out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w)||effects_range_overlap(out,n,&t,sizeof t)||effects_range_overlap(out,n,&b,sizeof b)||effects_range_overlap(out,n,&r,sizeof r)||effects_range_overlap(out,n,&u,sizeof u)||effects_range_overlap(out,n,&c,sizeof c)||effects_range_overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return effects_range_overlap(out,n,w.original_bytes(),w.original_size())||effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
StableIntakeError logic(LogicError e){return e==LogicError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;}
StableIntakeError leaf(TileYieldError e){return e==TileYieldError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;}
unsigned bits(uint32_t n){unsigned count=0;while(n){n&=n-1;++count;}return count;}
bool integer(int64_t value,int32_t&out){if(value<(-2147483647LL-1)||value>2147483647LL)return false;out=(int32_t)value;return true;}
struct Work {WorldHumanAllocationResult next;TileYieldContext contexts[21];};
}
StableIntakeError world_human_allocation(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const UnitDefinitions&u,const CitizenDefinitions&c,size_t id,const StableIntakeStage&s,WorldHumanAllocationResult&output){
 if(aliases(w,t,b,r,u,c,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded()||id>2147483647u||(s.variant!=Variant::base&&s.variant!=Variant::alien_crossfire)||(s.variant==Variant::base&&s.expansion_enabled)||s.variant!=(w.game().data_version<11?Variant::base:Variant::alien_crossfire))return StableIntakeError::bad_state;
 if(s.worker.upkeep_state!=0||s.worker.editor_active)return StableIntakeError::unsupported_stage;
 const auto*base=w.base(id);if(!base||base->faction_id<1||base->faction_id>7||base->pop_size<0)return StableIntakeError::bad_state;
 const int32_t faction=base->faction_id;
 if(!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction))||(((uint32_t)base->governor_flags&0x80000040u)==0x80000040u)||base->specialist_adjust)return StableIntakeError::unsupported_stage;
 WorkerRadiusResult radius{};auto re=world_worker_radius(w,id,s.worker,radius);
 if(re==WorkerRadiusError::bad_stack)return StableIntakeError::bad_stack;
 if(re==WorkerRadiusError::out_of_memory)return StableIntakeError::out_of_memory;
 if(re!=WorkerRadiusError::ok)return StableIntakeError::bad_state;
 // A coherent retained arrangement exits before original governor scoring.
 // Preserve real flags; admit bit31 only when no priority values are consumed.
 if((uint32_t)base->governor_flags&0x80000000u){const uint32_t mask=(uint32_t)base->worked_tiles;
  if(base->specialist_total<0||base->specialist_total>base->pop_size||!(mask&1u)||(mask&~0x1fffffu)||(mask&radius.unavailable&~1u)||bits(mask)+(unsigned)base->specialist_total!=(unsigned)base->pop_size+1)return StableIntakeError::unsupported_stage;
 }
 UnitSupportTotals support{};auto le=world_unit_support(w,u,t,id,support);if(le!=LogicError::ok)return logic(le);
 WorldCitizenEnergy citizens{};le=world_citizen_energy(w,t,c,faction,citizens);if(le!=LogicError::ok)return logic(le);
 bool full=(base->facilities_built[29/8]&(1u<<(29%8)))!=0;
 if(!full&&world_has_project(w,27,faction,full)!=LogicError::ok)return StableIntakeError::bad_state;
 if(!full&&s.variant==Variant::alien_crossfire&&world_has_project(w,35,faction,full)!=LogicError::ok)return StableIntakeError::bad_state;
 const auto*owner=w.faction((size_t)faction);if(!owner)return StableIntakeError::bad_state;
 Work*work=static_cast<Work*>(malloc(sizeof(Work)));if(!work)return StableIntakeError::out_of_memory;
 *work=Work{};auto&next=work->next;auto&facts=next.facts;
 auto calculate=[&]()->StableIntakeError {
  facts.variant=s.variant;facts.profile=HumanAllocationProfile::directed_no_governor_priorities;facts.population=base->pop_size;facts.requirement=b.nutrient_req_citizen;facts.maintenance=support.mineral_cost;facts.current_item=base->queue_items[0];facts.specialists=base->specialist_total;facts.worked=(uint32_t)base->worked_tiles;facts.unavailable=radius.unavailable;facts.minimum_specialist_size=b.min_base_size_specialists;
  facts.specialist_types[0]=(uint32_t)base->specialist_types[0];facts.specialist_types[1]=(uint32_t)base->specialist_types[1];
  for(unsigned i=0;i<7;++i){const auto&v=citizens.citizens[i];facts.citizens[i]={v.prerequisite_available?1u:0u,v.economy,v.psych,v.labs};}
  const int32_t counts[]={owner->satellites_nutrient,owner->satellites_mineral,owner->satellites_energy};
  for(unsigned k=0;k<3;++k){if(counts[k]<0)return StableIntakeError::bad_state;if(!full&&counts[k]==2147483647)return StableIntakeError::overflow;const int32_t v=full?counts[k]:(counts[k]+1)/2;facts.satellites[k]=v<base->pop_size?v:base->pop_size;next.intake.satellites[k]=facts.satellites[k];}
  const auto&g=w.game();const MapGeometry geometry{(int32_t)g.map.info.width,(int32_t)g.map.info.height,g.map.info.flat};
  for(unsigned i=0;i<21;++i){if(i&&(radius.unavailable&(1u<<i)))continue;
   int32_t x=base->x+dx[i],y=base->y+dy[i];if(!geometry.flat){if(x<0)x+=geometry.width;else if(x>=geometry.width)x-=geometry.width;}
   WorldTileYieldStage stage{s.variant,s.expansion_enabled,false,s.control_upkeep,(int32_t)id,0,0};
   auto error=world_fresh_tile_yield_context(w,t,b,r,id,faction,x,y,stage,work->contexts[i]);if(error!=TileYieldError::ok)return leaf(error);
   if(!i&&(!(work->contexts[i].tile.items&1u)||(work->contexts[i].tile.val2&15u)!=(unsigned)faction))return StableIntakeError::unsupported_stage;
   TileYieldResult value{};error=calculate_tile_yields(work->contexts[i],value);if(error!=TileYieldError::ok)return leaf(error);
   facts.yields[i][0]=value.nutrient;facts.yields[i][1]=value.mineral;facts.yields[i][2]=value.energy;
  }
  const auto allocated=human_worker_allocation_traced(facts,next.allocation,next.trace);
  if(allocated==WorkerAllocationError::overflow)return StableIntakeError::overflow;
  if(allocated!=WorkerAllocationError::ok)return StableIntakeError::needs_allocation;
  auto&intake=next.intake;intake.worked_tiles=next.allocation.worked;intake.state_flags=(uint32_t)base->state_flags&~0x8100u;intake.terraform_reduction_after=s.terraform_reduction_before;intake.base_energy_after=s.base_energy_before;
  int32_t totals[3]={facts.satellites[0],facts.satellites[1],facts.satellites[2]};
  for(unsigned j=0;j<next.trace.count;++j){const auto&visit=next.trace.visits[j];auto context=work->contexts[visit.tile];context.terraform_reduction_before=intake.terraform_reduction_after;context.base_energy_before=intake.base_energy_after;
   TileYieldResult value{};auto error=calculate_tile_yields(context,value);if(error!=TileYieldError::ok)return leaf(error);
   const int32_t yields[]={value.nutrient,value.mineral,value.energy};for(unsigned k=0;k<3;++k)if(yields[k]!=facts.yields[visit.tile][k])return StableIntakeError::unsupported_stage;
   intake.terraform_reduction_after=value.terraform_reduction;intake.base_energy_after=value.base_energy;
   if(visit.kind!=AllocationVisitKind::candidate){
    if((context.tile.climate&0xe0u)<0x60u)intake.state_flags|=0x8000u;
    for(unsigned k=0;k<3;++k)if(!integer((int64_t)totals[k]+(visit.kind==AllocationVisitKind::remove?-(int64_t)yields[k]:(int64_t)yields[k]),totals[k]))return StableIntakeError::overflow;
    intake.terraform_reduction_after=0;
   }
  }
  for(unsigned k=0;k<3;++k){if(totals[k]!=next.allocation.intake[k])return StableIntakeError::bad_state;intake.intake[k]=totals[k];if(!integer((int64_t)totals[k]-facts.satellites[k],intake.tiles[k]))return StableIntakeError::overflow;}
  for(unsigned i=0;i<21;++i)if(intake.worked_tiles&(1u<<i))for(unsigned k=0;k<3;++k)intake.tile_yields[i][k]=facts.yields[i][k];
  TerraformEcologyResult ecology{};if(world_terraform_ecology(w,id,ecology)!=TerraformEcologyError::ok)return StableIntakeError::bad_state;intake.raw_eco_damage=ecology.raw_damage;return StableIntakeError::ok;
 };
 const auto error=calculate();if(error==StableIntakeError::ok)output=next;free(work);return error;
}
}
