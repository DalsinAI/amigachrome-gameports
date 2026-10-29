/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_crt.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool alias(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&d,sizeof d)||overlap(out,n,&t,sizeof t)||overlap(out,n,&r,sizeof r))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
CombatError world_resolve_seeded_field_combat(const OriginalWorld&w,const UnitDefinitions&d,
 const Rules&t,const BasicRules&r,size_t attacker,size_t defender,
 CombatRandomState&rng,FieldCombatFacts&facts,CombatResult&out){
 if(alias(w,d,t,r,&rng,sizeof rng)||alias(w,d,t,r,&facts,sizeof facts)||alias(w,d,t,r,&out,sizeof out)
 ||overlap(&rng,sizeof rng,&facts,sizeof facts)||overlap(&rng,sizeof rng,&out,sizeof out)||overlap(&facts,sizeof facts,&out,sizeof out))return CombatError::bad_buffer;
 FieldCombatFacts next{};auto e=world_field_combat(w,d,t,r,attacker,defender,next);if(e!=CombatError::ok)return e;
 auto random=rng;CombatResult result{};e=resolve_seeded_combat_rounds(next.duel,random,result);if(e!=CombatError::ok)return e;
 rng=random;facts=next;out=result;return CombatError::ok;
}
}
