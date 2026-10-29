/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_nutrients.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool alias(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const FreshNutrientStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
StableIntakeError world_fresh_nutrients(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const FreshNutrientStage&s,FreshNutrientResult&output){
 if(alias(w,t,b,r,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 FreshNutrientResult next{};const auto e=world_stable_intake(w,t,b,r,id,s.intake,next.intake);if(e!=StableIntakeError::ok)return e;
 const auto*base=w.base(id);const auto*owner=w.faction((size_t)base->faction_id);
 NutrientFacts f{base->pop_size,owner->SE_growth_pending,next.intake.intake[0],s.convoy_to,s.convoy_from,b.nutrient_req_citizen,base->nutrients_accumulated,owner->nutrient_surplus_total,(uint32_t)s.intake.worker.upkeep_state,((base->facilities_built[0]&4u)?1u:0u)|(((uint32_t)base->state_flags&4u)?2u:0u)};
 const auto n=base_nutrients(f,next.nutrients);if(n==NutrientError::overflow)return StableIntakeError::overflow;if(n!=NutrientError::ok)return StableIntakeError::bad_state;
 output=next;return StableIntakeError::ok;
}
}
