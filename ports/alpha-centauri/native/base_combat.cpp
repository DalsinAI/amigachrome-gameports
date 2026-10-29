/* SPDX-License-Identifier: GPL-3.0-or-later. See base_combat.h. */
#include "base_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool word(int64_t x){return x>=(-2147483647LL-1)&&x<=2147483647LL;}
bool percent(int32_t x,int32_t p,int32_t&out){const int64_t factor=(int64_t)p+100;if(!word(factor))return false;const int64_t product=factor*x;if(!word(product))return false;out=(int32_t)(product/100);return true;}
}
LogicError base_combat_adjustment(Variant v,const BaseCombatContext&c,BaseCombatAdjustment&out){
 if((v!=Variant::base&&v!=Variant::alien_crossfire)||(c.attacker_present&&(c.attacker_prototype<0||c.attacker_prototype>=512||c.attacker_weapon< -128||c.attacker_weapon>127||c.attacker_triad<0||c.attacker_triad>2||c.attacker_speed<0||c.attacker_speed>255||c.attacker_faction<0||c.attacker_faction>7))||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 uint32_t multi=2;
 if(c.attacker_present){
  if((c.attacker_triad==0&&(c.perimeter||c.citizens))||(c.attacker_triad==1&&(c.naval_yard||c.maritime))||(c.attacker_triad==2&&(c.aerospace||(v==Variant::alien_crossfire&&c.cloudbase))))multi=4;
  if(c.tachyon)multi+=2;
 }
 BaseCombatAdjustment result{c.offense,c.defense,multi,false,false};
 if(multi==2&&(!c.attacker_present||!c.blink)&&c.base_percent){if(!percent(result.defense,c.base_percent,result.defense))return LogicError::overflow;result.base_percent_applied=true;}
 if(c.attacker_present&&c.blink&&multi>2)multi=2;
 if(c.infantry_percent&&c.attacker_present&&c.attacker_faction&&(c.attacker_prototype>=64||(c.attacker_weapon>=0&&c.attacker_prototype!=15))&&c.attacker_triad==0&&c.attacker_speed==1){if(!percent(result.offense,c.infantry_percent,result.offense))return LogicError::overflow;result.infantry_percent_applied=true;}
 const int64_t defense=(int64_t)result.defense*multi;if(!word(defense))return LogicError::overflow;result.defense=(int32_t)defense;result.multiplier=multi;out=result;return LogicError::ok;
}
}
