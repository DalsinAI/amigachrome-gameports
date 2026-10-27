/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in research_progress.h. */
#include "world_upkeep_research.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,const WorkerResolutionStage&s,const ResearchLedger&l,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&u,sizeof u)||overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||overlap(out,n,&c,sizeof c)||overlap(out,n,&s,sizeof s)||overlap(out,n,&l,sizeof l))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
ResearchError accounting_error(StableIntakeError e){switch(e){case StableIntakeError::ok:return ResearchError::ok;case StableIntakeError::bad_buffer:return ResearchError::bad_buffer;case StableIntakeError::unsupported_stage:return ResearchError::unsupported;case StableIntakeError::needs_allocation:return ResearchError::needs_allocation;case StableIntakeError::out_of_memory:return ResearchError::out_of_memory;case StableIntakeError::overflow:return ResearchError::overflow;default:return ResearchError::bad_state;}}
struct States{uint32_t*p;~States(){free(p);}};
}
ResearchError world_upkeep_research(const OriginalWorld&w,const Rules&t,const UnitDefinitions&u,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const CitizenDefinitions&c,size_t id,const WorkerResolutionStage&s,const ResearchLedger&before,WorldUpkeepResearchResult&out,uint32_t*states,size_t capacity){
 if(capacity>SIZE_MAX/sizeof(uint32_t))return ResearchError::bad_buffer;
 const size_t bytes=capacity*sizeof(uint32_t);
 if(aliases(w,t,u,b,wr,r,c,s,before,&out,sizeof out)||aliases(w,t,u,b,wr,r,c,s,before,states,bytes)||overlap(&out,sizeof out,states,bytes))return ResearchError::bad_buffer;
 if(!w.loaded())return ResearchError::bad_state;
 const size_t count=w.game().vehicle_count;if(capacity<count||(!states&&count))return ResearchError::bad_buffer;
 States scratch{count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr};if(count&&!scratch.p)return ResearchError::out_of_memory;
 WorldUpkeepResearchResult next{};const auto accounted=world_upkeep_accounting(w,t,u,b,wr,r,c,id,s,next.accounting,scratch.p,count);if(accounted!=StableIntakeError::ok)return accounting_error(accounted);
 if(next.accounting.resolved.requires_riot_event)return ResearchError::needs_riot_resolution;
 const WorldResearchStage stage{next.accounting.resolved.allocation.allocation.labs,before};const auto error=world_base_research(w,t,b,id,stage,next.research);if(error!=ResearchError::ok)return error;
 if(count)memcpy(states,scratch.p,count*sizeof(uint32_t));
 out=next;return ResearchError::ok;
}
}
