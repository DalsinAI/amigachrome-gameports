/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_accounting.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool aliases(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,
 const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const WorldAccountingStage&s,
 const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||
  overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||
  overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||
  overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
StableIntakeError support_error(SupportError e){switch(e){
 case SupportError::ok:return StableIntakeError::ok;
 case SupportError::unsupported:return StableIntakeError::unsupported_stage;
 case SupportError::bad_buffer:return StableIntakeError::bad_buffer;
 case SupportError::overflow:return StableIntakeError::overflow;
 case SupportError::out_of_memory:return StableIntakeError::out_of_memory;
 default:return StableIntakeError::bad_state;
}}
struct States {uint32_t*p;~States(){free(p);}};
}
StableIntakeError world_base_accounting(const OriginalWorld&w,const Rules&t,
 const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,
 size_t id,const WorldAccountingStage&s,WorldAccountingResult&output,uint32_t*states,size_t capacity){
 if(capacity>SIZE_MAX/sizeof(uint32_t))return StableIntakeError::bad_buffer;
 const size_t bytes=capacity*sizeof(uint32_t);
 if(aliases(w,t,u,b,wr,r,s,&output,sizeof output)||aliases(w,t,u,b,wr,r,s,states,bytes)||
  overlap(&output,sizeof output,states,bytes))return StableIntakeError::bad_buffer;
 if(!w.loaded())return StableIntakeError::bad_state;
 const size_t count=w.game().vehicle_count;
 if(capacity<count||(!states&&count))return StableIntakeError::bad_buffer;
 States scratch{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};
 if(count&&!scratch.p)return StableIntakeError::out_of_memory;
 WorldAccountingResult next{};
 const auto support=world_base_support_policy(w,u,id,next.support,scratch.p,count);
 if(support!=SupportError::ok)return support_error(support);
 const UpkeepWorldMineralStage ms{s.intake,next.support.mineral_cost};
 const auto mineral=world_base_minerals(w,t,b,wr,r,id,ms,next.mineral);
 if(mineral!=StableIntakeError::ok)return mineral;
 const auto*base=w.base(id);const auto*faction=w.faction((size_t)base->faction_id);
 const auto&fresh=next.mineral.fresh;const auto&convoy=next.mineral.convoys;
 const NutrientFacts nf{base->pop_size,faction->SE_growth_pending,fresh.intake[0],convoy.to[0],convoy.from[0],
  b.nutrient_req_citizen,base->nutrients_accumulated,faction->nutrient_surplus_total,
  (uint32_t)s.intake.worker.upkeep_state,(base->facilities_built[0]&4u?1u:0u)|((uint32_t)base->state_flags&4u?2u:0u)};
 const auto nutrient=base_nutrients(nf,next.nutrient);
 if(nutrient!=NutrientError::ok)return nutrient==NutrientError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 const UpkeepWorldEnergyStage es{fresh.intake[2],convoy.to[2],s.commerce,s.energy_grid,convoy.from[2],s.intake.worker.upkeep_state};
 auto energy=world_energy_accounting(w,id,es,next.energy);
 if(energy!=EnergyError::ok)return energy==EnergyError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 energy=world_energy_shortfall_orders(w,id,next.energy.accounting.surplus,next.energy_shortfall);
 if(energy!=EnergyError::ok)return energy==EnergyError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 if(count)memcpy(states,scratch.p,count*sizeof(uint32_t));
 output=next;return StableIntakeError::ok;
}
}
