/* SPDX-License-Identifier: GPL-3.0-or-later. See fungus_combat.h. */
#include "fungus_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError fungus_combat_adjustment(const FungusCombatContext&c,FungusCombatStrength&out){
 if(c.attacker_prototype<0||c.attacker_prototype>=512||c.attacker_weapon< -128||c.attacker_weapon>127||c.defender_weapon< -128||c.defender_weapon>127||c.altitude>255||c.attacker_faction>7||c.pholus_owner< -1||c.pholus_owner>7||c.attacker_plan>255||c.attacker_plan==11||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 FungusCombatStrength r{c.offense,false};
 if((c.items&32u)&&c.altitude>=64&&((c.attacker_prototype<64&&c.attacker_weapon<0)||c.pholus_owner==(int32_t)c.attacker_faction)&&c.defender_weapon>=0){const int64_t sum=(int64_t)c.offense+c.offense/2;if(sum<(-2147483647LL-1)||sum>2147483647LL)return LogicError::overflow;r.offense=(int32_t)sum;r.applied=true;}
 out=r;return LogicError::ok;
}
}
