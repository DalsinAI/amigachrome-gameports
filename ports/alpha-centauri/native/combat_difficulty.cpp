/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_difficulty.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
}
BattleEntryError inspect_combat_difficulty(const CombatDifficultyFacts&f,CombatDifficultyResult&out){
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return BattleEntryError::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return BattleEntryError::missing_runtime;
 if((uint32_t)f.edition>1||f.attacker_faction>7||f.defender_faction>7)return BattleEntryError::bad_state;
 if(!f.attacker_faction||!f.defender_faction)return BattleEntryError::unsupported;
 const uint32_t mask=f.human_status&255u;const bool a=(mask&(1u<<f.attacker_faction))!=0,b=(mask&(1u<<f.defender_faction))!=0;
 CombatDifficultyResult next{f.offense,0};
 if(f.defender_difficulty<2&&b&&!a){next.offense=signed_word(((uint32_t)f.defender_difficulty+1u)*(uint32_t)next.offense)/4;next.applied|=1;}
 if(f.attacker_difficulty<2&&a&&!b){next.offense=signed_word((4u-(uint32_t)f.attacker_difficulty)*(uint32_t)next.offense)/2;next.applied|=2;}
 out=next;return BattleEntryError::ok;
}
}
