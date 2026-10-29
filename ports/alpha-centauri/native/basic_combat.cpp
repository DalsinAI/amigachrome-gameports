/* SPDX-License-Identifier: GPL-3.0-or-later. See basic_combat.h. */
#include "basic_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
int32_t clamp(int32_t n,int32_t low,int32_t high){return n<low?low:n>high?high:n;}
bool word(int64_t n){return n>=(-2147483647LL-1)&&n<=2147483647LL;}
int32_t active(int32_t n,int32_t high){n=clamp(n,-4,high);return n<=-2?n+1:n;}
}
LogicError basic_combat_strengths(Variant v,const BasicCombatContext&c,BasicCombatStrengths&out){
 if((v!=Variant::base&&v!=Variant::alien_crossfire)||c.prototype<0||c.prototype>=512||c.weapon_offense< -128||c.weapon_offense>127||c.armor_defense< -128||c.armor_defense>127||c.plan<0||c.plan>255||c.opposing_plan<0||c.opposing_plan>255||c.initial_morale<0||c.initial_morale>6||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 const bool expansion=v==Variant::alien_crossfire;
 const bool brood=expansion&&c.brood&&c.prototype<64&&(c.weapon_offense<0||c.prototype==15);
 const bool gas=expansion&&!c.opponent_native&&(c.prototype>=64||(c.weapon_offense>=0&&c.prototype!=15))&&!c.dissociative&&c.soporific;
 int32_t attack=c.initial_morale,defense=attack;
 if(c.at_base){
  if(c.creche){attack=attack+1-active(c.active_se_morale,4);defense=defense+1-active(c.active_se_morale,0);if(brood){++attack;++defense;}}
  else if(brood){attack=attack+1-active(c.active_se_morale,4);defense=defense+1-active(c.active_se_morale,4);}
  const int32_t bonus=(c.pending_se_morale>=2&&c.pending_se_morale<=3?1:0)+(c.opponent_native?1:0);
  defense+=bonus;if(c.offense_mode){attack+=bonus;if(gas)attack-=2;}
 }
 if(c.offense_mode)attack=clamp(attack,1,6);
 if(gas)defense-=2;
 defense=clamp(defense,1,6);
 const bool fixed=c.plan==12||(c.plan==11&&c.armor_defense==1&&c.opposing_plan!=11);
 const int64_t first=(int64_t)c.factored_offense*(attack+6);if(!word(first)||!word(first*4))return LogicError::overflow;
 const int64_t second=fixed?1:(int64_t)c.factored_defense*(defense+6);if(!word(second))return LogicError::overflow;
 BasicCombatStrengths result{(int32_t)(first*4),(int32_t)second,attack,defense,fixed};out=result;return LogicError::ok;
}
}
