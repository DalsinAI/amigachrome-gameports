/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "attack_modifier_ledger.h"
#include "technology_effects_ranges.h"
namespace alpha {
BattleEntryError attack_modifier_ledger(const AttackModifierFacts&f,const AttackModifierLedger&prior,AttackModifierResult&out){
 using detail::effects_range_overlap;using E=BattleEntryError;
 if(effects_range_overlap(&f,sizeof f,&out,sizeof out)||effects_range_overlap(&prior,sizeof prior,&out,sizeof out)||effects_range_overlap(&f,sizeof f,&prior,sizeof prior))return E::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if((uint32_t)f.edition>1||f.fungus_applied>1||f.fanatic_flag>1||f.offense_factor_count>8||f.ground_applied>31)return E::bad_state;
 if(((f.ground_applied&1u)&&!f.mobile_percent)||((f.ground_applied&16u)&&!f.downhill_percent)||((bool)(f.ground_applied&4u)!=(f.road_percent!=0)))return E::bad_state;
 AttackModifierResult next{};next.ledger=prior;next.ledger.count=0;
 auto event=[&](uint32_t label,int32_t percent){auto&e=next.lookups[next.lookup_count++];e={label,percent,0};if(percent&&next.ledger.count<4){next.ledger.slots[next.ledger.count++]={label,percent};e.appended=1;}};
 if(f.fungus_applied)event(338,50);
 if(f.fanatic_flag&&f.fanatic_percent)event(528,f.fanatic_percent);
 if(f.edition==Variant::alien_crossfire)for(uint32_t i=0;i<f.offense_factor_count;++i){const uint32_t n=(uint32_t)f.offense_factors[i]-100u;event(1108,n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n);}
 if(f.ground_applied&1u)event(611,f.mobile_percent);
 if(f.ground_applied&4u)event(606,f.road_percent);
 if(f.ground_applied&16u)event(330,f.downhill_percent);
 out=next;return E::ok;
}
}
