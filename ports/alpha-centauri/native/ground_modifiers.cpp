/* SPDX-License-Identifier: GPL-3.0-or-later. See ground_modifiers.h. */
#include "ground_modifiers.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool percent(int32_t x,int32_t p,int32_t&out){const int64_t factor=(int64_t)p+100;if(factor<(-2147483647LL-1)||factor>2147483647LL)return false;const int64_t n=factor*x;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)(n/100);return true;}
}
LogicError ground_combat_modifiers(const GroundModifierContext&c,GroundModifierStrengths&out){
 if(c.terrain<2||c.terrain>4||c.roughness>255||c.attacker_altitude>255||c.defender_altitude>255||c.attacker_speed<0||c.attacker_speed>9900||c.defender_speed<0||c.defender_speed>9900||c.road_rate<1||c.road_rate>100||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 GroundModifierStrengths result{c.offense,c.defense,0};
 if(c.mobile_percent&&!c.physical_base&&c.terrain==2&&(c.roughness&0xc0u)<0x80u&&c.attacker_speed>c.road_rate&&c.defender_speed<=c.attacker_speed){if(!percent(result.offense,c.mobile_percent,result.offense))return LogicError::overflow;result.applied|=1;}
 if(c.rough_percent&&(c.terrain>2||c.physical_base)&&c.attacker_speed>c.road_rate){if(!percent(result.defense,c.rough_percent,result.defense))return LogicError::overflow;result.applied|=2;}
 if(c.road_percent){if(!percent(result.offense,c.road_percent,result.offense))return LogicError::overflow;result.applied|=4;}
 if(c.uphill_percent&&(c.defender_altitude&0xe0u)>(c.attacker_altitude&0xe0u)&&!c.attacker_antigrav){if(!percent(result.defense,c.uphill_percent,result.defense))return LogicError::overflow;result.applied|=8;}
 if(c.downhill_percent&&(c.attacker_altitude&0xe0u)>(c.defender_altitude&0xe0u)&&!c.defender_antigrav){if(!percent(result.offense,c.downhill_percent,result.offense))return LogicError::overflow;result.applied|=16;}
 out=result;return LogicError::ok;
}
}
