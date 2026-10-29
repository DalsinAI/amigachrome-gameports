/* SPDX-License-Identifier: GPL-3.0-or-later. See air_port_combat.h. */
#include "air_port_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool percent(int32_t value,int32_t p,bool penalty,int32_t&out){const int64_t factor=100+(penalty?-(int64_t)p:(int64_t)p);if(factor<(-2147483647LL-1)||factor>2147483647LL)return false;const int64_t n=value*factor;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)(n/100);return true;}
}
LogicError air_port_combat(const AirPortContext&c,AirPortStrengths&out){
 if(c.attacker_triad>2||c.defender_triad>2||c.defender_weapon<-128||c.defender_weapon>127||c.defender_armor<-128||c.defender_armor>127||c.attacker_faction<0||c.attacker_faction>7||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 AirPortStrengths result{c.offense,c.defense,0};
 if(c.attacker_triad==2&&c.attacker_superiority){
  if(c.defender_triad==2){if(!c.defender_superiority&&c.air_superiority_percent){if(!percent(result.offense,c.air_superiority_percent,false,result.offense))return LogicError::overflow;result.applied|=2;}}
  else if(c.ground_strike_penalty){if(!percent(result.offense,c.ground_strike_penalty,true,result.offense))return LogicError::overflow;result.applied|=1;}
 }
 if(c.attacker_triad==2&&c.defender_triad==2&&c.defender_superiority&&!c.attacker_superiority&&!c.attacker_missile&&!c.defender_missile&&c.air_superiority_percent){if(!percent(result.defense,c.air_superiority_percent,false,result.defense))return LogicError::overflow;result.applied|=4;}
 if(c.defender_weapon==0&&c.defender_armor==1&&(c.attacker_faction||!c.defender_base)&&c.noncombat_penalty){if(!percent(result.defense,c.noncombat_penalty,true,result.defense))return LogicError::overflow;result.applied|=8;}
 if(c.attacker_triad!=1&&c.defender_triad==1&&c.defender_base&&c.port_percent){if(!percent(result.offense,c.port_percent,false,result.offense))return LogicError::overflow;result.applied|=16;}
 out=result;return LogicError::ok;
}
}
