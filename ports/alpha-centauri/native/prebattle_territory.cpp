/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_territory.h"
#include "technology_effects_ranges.h"
namespace alpha {
PrebattleTerritoryError inspect_prebattle_territory(const PrebattleTerritoryFacts&f,PrebattleTerritoryProposal&out){
 using E=PrebattleTerritoryError;
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return E::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if((uint32_t)f.edition>1||f.attacker>7||f.defender>7||f.current_faction>7||f.owner< -128||f.owner>127)return E::bad_state;
 if(!f.attacker||!f.defender||f.attacker==f.defender)return E::unsupported;
 PrebattleTerritoryProposal next{f.owner_defender_status,f.defender_owner_status,f.defender_owner_friction,f.negotiation_accumulator,0,0};
 if(f.owner>=1&&f.owner<=7&&(uint32_t)f.owner!=f.attacker&&(uint32_t)f.owner!=f.defender&&(f.attacker_owner_status&1u)&&(f.defender_owner_status&0x18u)==8u&&f.owner_strength<f.defender_strength){
  next.owner_defender_status|=0x2000u;next.defender_owner_status|=0x2000u;next.treaty_applied=1;
  if(next.owner_defender_status&0x800000u){
   next.friction_applied=1;
   const uint32_t value=(uint32_t)f.defender_owner_friction+3u;
   next.defender_owner_friction=(value&0x80000000u)?0:(value>20u?20:(int32_t)value);
   if(f.negotiation_faction==f.defender&&f.negotiation_other==(uint32_t)f.owner)next.negotiation_accumulator+=3u;
  }
 }
 out=next;return E::ok;
}
}
