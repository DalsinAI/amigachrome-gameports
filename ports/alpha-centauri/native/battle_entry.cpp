/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "battle_entry.h"
#include "technology_effects_ranges.h"
namespace alpha {
namespace {
int32_t signed_word(uint32_t n) {
 return n&0x80000000u ? -(int32_t)(~n)-1 : (int32_t)n;
}
int32_t remaining(uint32_t speed, uint32_t spent) {
 const int32_t n=signed_word(speed-spent);
 return n<0?0:n>999?999:n;
}
bool valid(Variant v) { return (uint32_t)v<=1; }
}
BattleEntryError inspect_battle_charge(const BattleChargeFacts&f, BattleChargeResult&out) {
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return BattleEntryError::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return BattleEntryError::missing_runtime;
 if(!valid(f.edition)||f.moves_spent>255)return BattleEntryError::bad_state;
 if(f.road<=0)return BattleEntryError::unsupported;
 const bool use=((uint32_t)f.mode&3u)&&f.mode<4;
 BattleChargeResult r{use?remaining(f.speed,f.moves_spent):f.road,(uint32_t)use};
 out=r;return BattleEntryError::ok;
}
BattleEntryError inspect_battle_hasty(const BattleHastyFacts&f, BattleHastyResult&out) {
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return BattleEntryError::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return BattleEntryError::missing_runtime;
 if(!valid(f.edition)||f.moves_spent>255||f.base_present>1||f.attacker_faction>7||f.attacker_design>=512||f.report_count<0)return BattleEntryError::bad_state;
 if(f.attack_flags!=2||f.road<=0)return BattleEntryError::unsupported;
 BattleHastyResult r{f.attacker_state,f.defender_state,f.moves_spent,0,f.offense,0,f.report_count,0,0};
 if(f.base_present)r.attacker_state&=~0x40000u;
 if(!(r.attacker_state&0x1000u)) {
  if(f.attacker_faction&&f.attacker_design!=8&&((uint32_t)f.edition==0||f.attacker_design!=15)) {
   int32_t n=remaining(f.speed,f.moves_spent);if(n>f.road)n=f.road;
   r.offense=signed_word((uint32_t)n*(uint32_t)f.offense)/f.road;
   r.penalty=signed_word(((uint32_t)n-(uint32_t)f.road)*100u)/f.road;
   r.scaled=1;r.string_lookup_required=1;
   if(r.penalty&&r.report_count<4){++r.report_count;r.report_append_required=1;}
  }
  if(f.mode!=1)r.moves_spent=(f.moves_spent+(uint32_t)f.charge)&255u;
 }
 r.attacker_state=(r.attacker_state&~0x1000u)|0x44u;
 r.defender_state|=4u;
 out=r;return BattleEntryError::ok;
}
}
