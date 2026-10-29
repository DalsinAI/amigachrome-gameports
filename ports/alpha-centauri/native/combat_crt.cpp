/* SPDX-License-Identifier: GPL-3.0-or-later; qualification combat_crt.h. */
#include "combat_crt.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
}
uint16_t combat_crt_draw(CombatRandomState&rng){
 rng.state=rng.state*214013u+2531011u;return (uint16_t)((rng.state>>16)&32767u);
}
CombatError resolve_seeded_combat_rounds(const CombatDuel&in,CombatRandomState&rng,CombatResult&out){
 if(overlap(&in,sizeof in,&rng,sizeof rng)||overlap(&in,sizeof in,&out,sizeof out)||overlap(&rng,sizeof rng,&out,sizeof out))return CombatError::bad_buffer;
 // Supported reactors allow <=79 rounds, hence <=158 CRT draws. Generate a
 // private tape to reuse the already-qualified round resolver; discard unused
 // private draws and commit only the actual draw count's resulting seed.
 uint16_t draws[160];auto scratch=rng;for(auto&draw:draws)draw=combat_crt_draw(scratch);
 CombatRollStream stream{draws,160,0};CombatResult result{};
 const auto e=resolve_combat_rounds(in,stream,result);if(e!=CombatError::ok)return e;
 auto next=rng;for(uint32_t i=0;i<result.random_draws;++i)(void)combat_crt_draw(next);
 rng=next;out=result;return CombatError::ok;
}
}
