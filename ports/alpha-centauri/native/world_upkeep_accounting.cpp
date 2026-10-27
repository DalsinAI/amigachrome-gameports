/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_upkeep_accounting.h"
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
 const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,
 const CitizenDefinitions&c,const WorkerResolutionStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||
  overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||
  overlap(out,n,&c,sizeof c)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||
  overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||
  overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
StableIntakeError support_error(SupportError e){switch(e){
 case SupportError::ok:return StableIntakeError::ok;
 case SupportError::bad_buffer:return StableIntakeError::bad_buffer;
 case SupportError::unsupported:return StableIntakeError::unsupported_stage;
 case SupportError::out_of_memory:return StableIntakeError::out_of_memory;
 case SupportError::overflow:return StableIntakeError::overflow;
 default:return StableIntakeError::bad_state;
}}
struct States {uint32_t*p;~States(){free(p);}};
}
StableIntakeError world_upkeep_accounting(const OriginalWorld&w,const Rules&t,
 const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,
 const CitizenDefinitions&c,size_t id,const WorkerResolutionStage&s,
 WorldUpkeepAccountingResult&output,uint32_t*states,size_t capacity){
 if(capacity>SIZE_MAX/sizeof(uint32_t))return StableIntakeError::bad_buffer;
 const size_t bytes=capacity*sizeof(uint32_t);
 if(aliases(w,t,u,b,wr,r,c,s,&output,sizeof output)||aliases(w,t,u,b,wr,r,c,s,states,bytes)||
  overlap(&output,sizeof output,states,bytes))return StableIntakeError::bad_buffer;
 if(!w.loaded())return StableIntakeError::bad_state;
 const size_t count=w.game().vehicle_count;
 if(capacity<count||(!states&&count))return StableIntakeError::bad_buffer;
 States scratch{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};
 if(count&&!scratch.p)return StableIntakeError::out_of_memory;
 WorldUpkeepAccountingResult next{};
 const auto resolved=world_worker_resolution(w,t,u,b,wr,r,c,id,s,next.resolved);
 if(resolved!=StableIntakeError::ok)return resolved;
 // The resolver owns its immutable world query. Recompute projected actor
 // flags against the same world and verify the complete support summary;
 // no cached support values enter either resource calculation or these flags.
 SupportResult support{};
 const auto projected=world_base_support_policy(w,u,id,support,scratch.p,count);
 if(projected!=SupportError::ok)return support_error(projected);
 if(memcmp(&support,&next.resolved.candidate.support,sizeof support))return StableIntakeError::bad_state;
 const auto energy=world_energy_shortfall_orders(w,id,next.resolved.energy.accounting.surplus,next.energy_shortfall);
 if(energy!=EnergyError::ok)return energy==EnergyError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;
 if(count)memcpy(states,scratch.p,count*sizeof(uint32_t));
 output=next;return StableIntakeError::ok;
}
}
