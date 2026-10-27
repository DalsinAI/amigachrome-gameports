/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_traded_budget.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);
}
bool aliases(const OriginalWorld&w,const void*p,size_t n){
 if(overlap(&w,sizeof w,p,n))return true;
 if(!w.loaded())return false;
 return overlap(w.original_bytes(),w.original_size(),p,n)||
 overlap(w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile),p,n)||
 overlap(w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle),p,n)||
 overlap(w.base(0),w.game().base_count*sizeof(OriginalBase),p,n)||
 overlap(w.prototype(0),512*sizeof(OriginalPrototype),p,n)||
 overlap(w.faction(0),8*sizeof(OriginalFactionState),p,n)||
 overlap(w.faction_definition(0),8*sizeof(OriginalFactionDefinition),p,n);
}
BudgetError resource_error(StableIntakeError e){switch(e){
 case StableIntakeError::bad_buffer:return BudgetError::bad_buffer;
 case StableIntakeError::unsupported_stage:return BudgetError::unsupported;
 case StableIntakeError::needs_allocation:return BudgetError::needs_allocation;
 case StableIntakeError::overflow:return BudgetError::overflow;
 case StableIntakeError::out_of_memory:return BudgetError::out_of_memory;
 default:return BudgetError::bad_state;
}}
struct Scratch {uint32_t*p;~Scratch(){free(p);}};
}
BudgetError world_traded_budget(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,
 const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,
 const FacilityDefinitions&f,size_t id,const int32_t*energies,size_t count,
 const BaseBudgetStage&s,TradedBudgetResult&output,uint32_t*states,size_t capacity){
 if(count>SIZE_MAX/sizeof(int32_t)||capacity>SIZE_MAX/sizeof(uint32_t))return BudgetError::bad_buffer;
 const size_t en=count*sizeof(int32_t),sn=capacity*sizeof(uint32_t);
 if(aliases(w,&output,sizeof output)||aliases(w,states,sn)||aliases(w,energies,en)||
 overlap(&output,sizeof output,states,sn)||overlap(&output,sizeof output,energies,en)||
 overlap(states,sn,energies,en))return BudgetError::bad_buffer;
 const void*sources[]={&t,&u,&b,&wr,&r,&c,&f,&s};
 const size_t sizes[]={sizeof t,sizeof u,sizeof b,sizeof wr,sizeof r,sizeof c,sizeof f,sizeof s};
 for(unsigned i=0;i<8;++i)if(overlap(sources[i],sizes[i],&output,sizeof output)||
  overlap(sources[i],sizes[i],states,sn))return BudgetError::bad_buffer;
 if(!w.loaded())return BudgetError::bad_state;
 const size_t actors=w.game().vehicle_count;
 if(capacity<actors||(!states&&actors))return BudgetError::bad_buffer;
 if(!energies||count!=w.game().base_count||id>=count||s.resources.commerce||s.resources.energy_grid)
  return BudgetError::bad_state;
 Scratch scratch{actors?(uint32_t*)malloc(actors*sizeof(uint32_t)):nullptr};
 if(actors&&!scratch.p)return BudgetError::out_of_memory;
 WorldAccountingResult resources{};
 const auto fresh=world_base_accounting(w,t,u,b,wr,r,id,s.resources,resources,scratch.p,actors);
 if(fresh!=StableIntakeError::ok)return resource_error(fresh);
 if(energies[id]!=resources.mineral.fresh.intake[2])return BudgetError::bad_state;
 TradedBudgetResult next{};
 const auto trade=world_base_commerce(w,t,id,energies,count,next.trade);
 if(trade!=CommerceError::ok)return trade==CommerceError::overflow?BudgetError::overflow:
  trade==CommerceError::bad_buffer?BudgetError::bad_buffer:
  trade==CommerceError::unsupported?BudgetError::unsupported:BudgetError::bad_state;
 BaseBudgetStage stage=s;stage.resources.commerce=next.trade.total;
 const auto budget=world_base_budget(w,t,u,b,wr,r,c,f,id,stage,next.budget,scratch.p,actors);
 if(budget!=BudgetError::ok)return budget;
 if(actors)memcpy(states,scratch.p,actors*sizeof(uint32_t));
 output=next;return BudgetError::ok;
}
}
