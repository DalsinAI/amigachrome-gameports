/* SPDX-License-Identifier: GPL-3.0-or-later. See combat_round.h. */
#include "combat_round.h"
#include "technology_effects_ranges.h"
namespace alpha {
CrtRandomDraw original_crt_random_step(uint32_t state){state=state*214013u+2531011u;return{state,(state>>16)&32767u};}
LogicError combat_health(const CombatHealthContext&c,CombatHealth&out){
 if(c.plan>255||c.reactor>255||c.damage>255||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 const uint32_t reactor=c.reactor<1?1:c.reactor>100?100:c.reactor;const uint32_t power=c.plan==12?1:reactor*10;const CombatHealth r{power,c.damage>=power?0:power-c.damage};out=r;return LogicError::ok;
}
LogicError combat_round_preview(const CombatRoundContext&c,CombatRoundResult&out){
 if(c.offense<0||c.defense<0||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 CombatHealth a,b;auto e=combat_health(c.attacker,a);if(e!=LogicError::ok)return e;e=combat_health(c.defender,b);if(e!=LogicError::ok)return e;
 CombatRoundResult r{c.crt_state,0,0,0,c.attacker.damage,c.defender.damage,a.remaining,b.remaining,0};
 if(a.remaining&&b.remaining){
  if(c.offense>1){const auto d=original_crt_random_step(r.next_crt_state);r.next_crt_state=d.next_state;r.attacker_roll=d.value%(uint32_t)c.offense;++r.draw_count;}
  if(c.defense>1){const auto d=original_crt_random_step(r.next_crt_state);r.next_crt_state=d.next_state;r.defender_roll=d.value%(uint32_t)c.defense;++r.draw_count;}
  if(r.attacker_roll<=r.defender_roll){r.hit_side=1;r.attacker_damage=(c.attacker.damage+(c.psi?c.attacker.reactor:1))&255u;}
  else{r.hit_side=2;r.defender_damage=(c.defender.damage+(c.psi?c.defender.reactor:1))&255u;}
  const CombatHealthContext aa{c.attacker.plan,c.attacker.reactor,r.attacker_damage},bb{c.defender.plan,c.defender.reactor,r.defender_damage};e=combat_health(aa,a);if(e!=LogicError::ok)return e;e=combat_health(bb,b);if(e!=LogicError::ok)return e;r.attacker_health=a.remaining;r.defender_health=b.remaining;
 }
 out=r;return LogicError::ok;
}
}
