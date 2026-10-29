/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2 and
 * mon_enemy_destroyed instruction behavior; no mod-enhancement semantics. */
#include "battle_bookkeeping.h"
#include <stddef.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){const auto x=(uintptr_t)a,y=(uintptr_t)b;return n&&m&&(x<=y?y-x<n:x-y<m);}
}
BookkeepingError preview_battle_bookkeeping(const BattleBookkeepingContext&c,const BattleBookkeepingState&s,BattleBookkeepingResult&out){
 using E=BookkeepingError;
 if(overlap(&c,sizeof c,&s,sizeof s)||overlap(&out,sizeof out,&c,sizeof c)||overlap(&out,sizeof out,&s,sizeof s))return E::invalid;
 if((uint32_t)c.edition>1||(uint32_t)c.outcome>1||c.attacker<0||c.attacker>7||c.defender<0||c.defender>7||c.attacker==c.defender||c.current_player< -1||c.current_player>7||c.multiplayer_active>1||c.pbem_active>1||(uint32_t)c.observation>1)return E::invalid;
 if(c.observation!=BookkeepingObservation::supplied)return E::missing_runtime;
 if(c.multiplayer_active||c.pbem_active)return E::network_notification;
 const bool lost=c.outcome==BookkeepingOutcome::attacker_lost;
 const uint32_t winner=lost?(uint32_t)c.defender:(uint32_t)c.attacker;
 const uint32_t loser=lost?(uint32_t)c.attacker:(uint32_t)c.defender;
 const uint32_t year=c.turn+c.start_year;
 const bool award=winner&&loser&&!s.enemy_destroyed[winner].awarded&&!(c.edition==BookkeepingEdition::smax&&(c.rule_flags[winner]&128u));
 if(award&&(int32_t)winner==c.current_player)return E::monument_ui;
 BattleBookkeepingResult next{};next.before_cleanup=s;
 ++next.before_cleanup.wins[winner];++next.before_cleanup.losses[loser];
 if(!lost){++next.before_cleanup.combat_total[winner][loser];++next.before_cleanup.combat_count[winner][loser];}
 next.after_cleanup=next.before_cleanup;
 if(lost){++next.after_cleanup.combat_total[winner][loser];++next.after_cleanup.combat_count[winner][loser];}
 if(award){
  auto&r=next.after_cleanup.enemy_destroyed[winner];bool others=false;
  for(uint32_t f=0;f<8;++f)if(f!=winner&&s.enemy_destroyed[f].awarded)others=true;
  uint32_t leader=others?loser:winner;
  if(others)for(uint32_t f=0;f<8;++f)if(f!=winner&&s.enemy_destroyed[f].awarded&&s.enemy_destroyed[f].rank)leader=f;
  r.year=year;r.opponent=loser;r.leader=leader;r.awarded=1;r.rank=others?0u:1u;r.focus=6;
 }
 next.winner=winner;next.loser=loser;next.monument_changed=award?1u:0u;out=next;return E::ok;
}
}
