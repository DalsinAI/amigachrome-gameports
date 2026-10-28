/* SPDX-License-Identifier: GPL-3.0-or-later. See combat_factions.h. */
#include "combat_factions.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool word(int64_t n){return n>=(-2147483647LL-1)&&n<=2147483647LL;}
bool scale(int32_t&n,int64_t factor){if(!word(factor)||!word(factor*n))return false;n=(int32_t)((factor*n)/100);return true;}
}
LogicError combat_faction_modifiers(const CombatFactionContext&c,CombatFactionStrengths&out){
 if((c.variant!=Variant::base&&c.variant!=Variant::alien_crossfire)||c.psi_mask>3||c.attacker_plan>255||c.attacker_plan==11||c.attacker_faction>7||c.offense_count>8||c.defense_count>8||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 CombatFactionStrengths r{c.offense,c.defense,0};
 if(c.made_airdrop&&c.drop_pods&&c.airdrop_penalty){if(!scale(r.offense,100LL-c.airdrop_penalty))return LogicError::overflow;r.applied|=1;}
 if(c.fanatic&&c.fanatic_bonus&&!c.psi_mask){if(!scale(r.offense,100LL+c.fanatic_bonus))return LogicError::overflow;r.applied|=2;}
 if(c.variant==Variant::alien_crossfire)for(uint32_t i=0;i<c.offense_count;++i){if(!scale(r.offense,c.offense_factors[i]))return LogicError::overflow;r.applied|=4;}
 if(c.psi_mask&&c.attacker_faction&&c.active_planet&&c.planet_percent){const int64_t p=(int64_t)c.active_planet*c.planet_percent;if(!word(p)||!scale(r.offense,p+100))return LogicError::overflow;r.applied|=8;}
 if(c.variant==Variant::alien_crossfire)for(uint32_t i=0;i<c.defense_count;++i){if(!scale(r.defense,c.defense_factors[i]))return LogicError::overflow;r.applied|=16;}
 out=r;return LogicError::ok;
}
}
