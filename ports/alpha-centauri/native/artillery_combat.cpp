/* SPDX-License-Identifier: GPL-3.0-or-later. See artillery_combat.h. */
#include "artillery_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool percent(int32_t value,int64_t p,int32_t&out){if(p<(-2147483647LL-1)||p>2147483647LL)return false;const int64_t factor=100+p;if(factor<(-2147483647LL-1)||factor>2147483647LL)return false;const int64_t n=value*factor;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)(n/100);return true;}
}
LogicError artillery_combat(const ArtilleryContext&c,ArtilleryStrengths&out){
 if(c.attacker_triad>2||c.defender_triad>2||c.attacker_altitude_byte>255||c.defender_altitude_byte>255||!(c.battle_flags&2u)||c.battle_flags>31||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 ArtilleryStrengths result{c.offense,c.defense,0};
 if(!(c.battle_flags&0x18u)){
  if(c.attacker_triad==0&&c.defender_triad==1){if(c.land_gun_percent){if(!percent(result.offense,c.land_gun_percent,result.offense))return LogicError::overflow;result.applied=1;}}
  else if(c.attacker_triad==1&&c.defender_triad==0){if(c.land_gun_percent){if(!percent(result.defense,c.land_gun_percent,result.defense))return LogicError::overflow;result.applied=2;}}
  else if(c.altitude_percent){const int32_t a=(int32_t)(c.attacker_altitude_byte>>5),d=(int32_t)(c.defender_altitude_byte>>5);
   if(a>d){if(!percent(result.offense,(int64_t)(a-d)*c.altitude_percent,result.offense))return LogicError::overflow;result.applied=4;}
   else if(d>a){if(!percent(result.defense,(int64_t)(d-a)*c.altitude_percent,result.defense))return LogicError::overflow;result.applied=8;}
  }
 }
 out=result;return LogicError::ok;
}
}
