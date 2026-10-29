/* SPDX-License-Identifier: GPL-3.0-or-later. See defense_abilities.h. */
#include "defense_abilities.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool percent(int32_t value,int32_t p,int32_t&out){const int64_t factor=(int64_t)p+100;if(factor<(-2147483647LL-1)||factor>2147483647LL)return false;const int64_t n=value*factor;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)(n/100);return true;}
}
LogicError defense_abilities(const DefenseAbilitiesContext&c,DefenseAbilitiesStrength&out){
 if((c.variant!=Variant::base&&c.variant!=Variant::alien_crossfire)||c.attacker_triad>2||c.attacker_chassis_speed>255||c.defender_armor>=(c.variant==Variant::base?10u:14u)||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 DefenseAbilitiesStrength result{c.defense,0};const bool mobile=c.attacker_triad==0&&c.attacker_chassis_speed>1;
 if(c.variant==Variant::alien_crossfire&&(c.defender_armor==10||c.defender_armor==12)&&mobile){if(!percent(result.defense,25,result.defense))return LogicError::overflow;result.applied|=1;}
 if(c.variant==Variant::base||!c.attacker_dissociative){
  if(c.defender_jammer&&mobile&&c.jammer_percent){if(!percent(result.defense,c.jammer_percent,result.defense))return LogicError::overflow;result.applied|=2;}
  if(c.defender_tracking&&c.attacker_triad==2&&c.tracking_percent){if(!percent(result.defense,c.tracking_percent,result.defense))return LogicError::overflow;result.applied|=4;}
 }
 out=result;return LogicError::ok;
}
}
