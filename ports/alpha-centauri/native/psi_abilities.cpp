/* SPDX-License-Identifier: GPL-3.0-or-later. See psi_abilities.h. */
#include "psi_abilities.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool percent(int32_t x,int32_t p,int32_t&out){const int64_t f=100+(int64_t)p;if(f<(-2147483647LL-1)||f>2147483647LL)return false;const int64_t n=x*f;if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)(n/100);return true;}
}
LogicError psi_abilities(const PsiAbilitiesContext&c,PsiAbilitiesStrengths&out){
 const bool expansion=c.variant==Variant::alien_crossfire;
 if((c.variant!=Variant::base&&!expansion)||c.psi_mask>3||c.attacker_weapon>=(expansion?26u:21u)||c.defender_armor>=(expansion?14u:10u)||c.attacker_plan>255||c.attacker_plan==11||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 PsiAbilitiesStrengths result{c.offense,c.defense,0};
 if(expansion&&(c.psi_mask&2u)&&(c.attacker_weapon==12||c.attacker_weapon==13)){if(!percent(result.offense,25,result.offense))return LogicError::overflow;result.applied|=1;}
 if((c.psi_mask&2u)&&c.attacker_empath&&(!expansion||!c.defender_dissociative)&&c.empath_percent){if(!percent(result.offense,c.empath_percent,result.offense))return LogicError::overflow;result.applied|=2;}
 if((c.psi_mask&1u)&&c.defender_trance&&(!expansion||!c.attacker_dissociative)&&c.trance_percent){if(!percent(result.defense,c.trance_percent,result.defense))return LogicError::overflow;result.applied|=4;}
 if(expansion&&(c.psi_mask&1u)&&(c.defender_armor==11||c.defender_armor==13)){if(!percent(result.defense,25,result.defense))return LogicError::overflow;result.applied|=8;}
 out=result;return LogicError::ok;
}
}
