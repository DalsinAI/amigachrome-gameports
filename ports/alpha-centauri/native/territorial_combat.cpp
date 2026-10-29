/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "territorial_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {
TerritoryError inspect_territorial_combat(const TerritorialCombatFacts&f,TerritorialCombatResult&out){
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return TerritoryError::bad_state;
 if(f.observation!=TerritoryObservation::supplied)return TerritoryError::missing_runtime;
 if((uint32_t)f.edition>1||f.climate>255||f.owner< -128||f.owner>127||f.attacker_faction<0||f.attacker_faction>7||f.defender_faction<0||f.defender_faction>7||f.attacker_faction==f.defender_faction)return TerritoryError::bad_state;
 const bool land=(f.climate&0xe0u)>=0x60u;
 if(land&&(f.owner<0||f.owner>7))return TerritoryError::unsupported;
 TerritorialCombatResult r{0,f.attacker_gift,f.attacker_gift,f.owner_friction,f.owner_friction,f.negotiation_accumulator,f.negotiation_accumulator};
 if(land&&f.owner!=f.attacker_faction&&f.owner!=f.defender_faction&&(f.owner_defender_status&0x10u)&&(f.owner_attacker_status&7u)){
  r.applied=1;if(!r.gift_after)r.gift_after=1;
  const uint32_t next=(uint32_t)f.owner_friction-1u;
  r.friction_after=(next&0x80000000u)?0:(next>20u?20:(int32_t)next);
  if(f.negotiation_faction==(uint32_t)f.owner&&f.negotiation_other==(uint32_t)f.attacker_faction)--r.negotiation_after;
 }
 out=r;return TerritoryError::ok;
}
}
