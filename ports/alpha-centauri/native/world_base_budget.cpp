/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_budget.h"
#include "world_cost_factor.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);
}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){
 if(overlap(p,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 return overlap(p,n,w.original_bytes(),w.original_size())||
 overlap(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile))||
 overlap(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle))||
 overlap(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase))||
 overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||
 overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||
 overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
BudgetError energy_error(EnergyError e){return e==EnergyError::overflow?BudgetError::overflow:e==EnergyError::bad_buffer?BudgetError::bad_buffer:BudgetError::bad_state;}
BudgetError accounting_error(StableIntakeError e){switch(e){
 case StableIntakeError::bad_buffer:return BudgetError::bad_buffer;
 case StableIntakeError::unsupported_stage:return BudgetError::unsupported;
 case StableIntakeError::needs_allocation:return BudgetError::needs_allocation;
 case StableIntakeError::overflow:return BudgetError::overflow;
 case StableIntakeError::out_of_memory:return BudgetError::out_of_memory;
 default:return BudgetError::bad_state;
}}
struct Scratch {uint32_t*p;~Scratch(){free(p);}};
}
BudgetError world_base_budget(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,
 const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const UpkeepCitizenDefinitions&c,
 const FacilityDefinitions&f,size_t id,const BaseBudgetStage&s,BaseBudgetResult&output,
 uint32_t*states,size_t capacity){
 if(capacity>SIZE_MAX/sizeof(uint32_t))return BudgetError::bad_buffer;
 const size_t bytes=capacity*sizeof(uint32_t);
 const void* sources[]={&t,&u,&b,&wr,&r,&c,&f,&s};
 const size_t sizes[]={sizeof t,sizeof u,sizeof b,sizeof wr,sizeof r,sizeof c,sizeof f,sizeof s};
 if(world_alias(w,&output,sizeof output)||world_alias(w,states,bytes)||
 overlap(&output,sizeof output,states,bytes))return BudgetError::bad_buffer;
 for(unsigned i=0;i<8;++i)if(overlap(sources[i],sizes[i],&output,sizeof output)||
  overlap(sources[i],sizes[i],states,bytes))return BudgetError::bad_buffer;
 if(!w.loaded())return BudgetError::bad_state;
 const size_t count=w.game().vehicle_count;
 if(capacity<count||(!states&&count))return BudgetError::bad_buffer;
 if(!w.base(id)||s.credits_before<0||s.maintenance_before<0||
  w.game().preferences.CurrentTurn<0)return BudgetError::bad_state;
 if(w.game().data_version>=11||s.resources.energy_grid)return BudgetError::unsupported;
 Scratch scratch{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};
 if(count&&!scratch.p)return BudgetError::out_of_memory;
 BaseBudgetResult next{};
 auto resource=world_base_accounting(w,t,u,b,wr,r,id,s.resources,next.resources,scratch.p,count);
 if(resource!=StableIntakeError::ok)return accounting_error(resource);
 auto allocation=world_energy_allocation(w,t,b,c,id,next.resources.energy.accounting.surplus,
  s.resources.commerce,next.allocation);
 if(allocation!=EnergyError::ok)return energy_error(allocation);
 const int64_t credits=(int64_t)s.credits_before+next.allocation.allocation.economy;
 if(credits>INT32_MAX||credits<0)return BudgetError::overflow;
 next.credits_after_income=(int32_t)credits;
 auto cost=world_base_cost_factor(w,b,id,1,next.mineral_cost_factor);
 if(cost!=CostFactorError::ok)return cost==CostFactorError::overflow?BudgetError::overflow:
  cost==CostFactorError::unsupported?BudgetError::unsupported:BudgetError::bad_state;
 if(next.mineral_cost_factor<=0)return BudgetError::unsupported;
 if(w.game().preferences.CurrentTurn>1){
  const FacilityUpkeepStage stage{next.credits_after_income,s.maintenance_before,
   next.mineral_cost_factor,s.player_flags_before};
  auto upkeep=world_facility_upkeep(w,t,u,f,id,stage,next.maintenance);
  if(upkeep!=EnergyError::ok)return energy_error(upkeep);
  next.maintenance_applied=true;
 }else{
  next.maintenance.credits_after=next.credits_after_income;
  next.maintenance.maintenance_after=s.maintenance_before;
  next.maintenance.player_flags_after=s.player_flags_before;
  memcpy(next.maintenance.built_after,w.base(id)->facilities_built,sizeof next.maintenance.built_after);
 }
 if(count)memcpy(states,scratch.p,count*sizeof(uint32_t));
 output=next;return BudgetError::ok;
}
}
