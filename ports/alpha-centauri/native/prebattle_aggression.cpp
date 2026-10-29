/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_aggression.h"
#include "technology_effects_ranges.h"
namespace alpha {
AggressionError inspect_prebattle_aggression(const PrebattleAggressionFacts&f,PrebattleAggressionProposal&out){
 using E=AggressionError;
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return E::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if((uint32_t)f.edition>1||f.attacker>7||f.defender>7||f.current_faction>7)return E::bad_state;
 if(!f.attacker||!f.defender||f.attacker==f.defender)return E::unsupported;
 if(f.reverse_status&7u)return E::double_cross_required;
 if(f.current_faction==f.attacker||f.current_faction==f.defender)return E::participant_ui_required;
 const bool reset=(f.forward_status&7u)!=0;
 const PrebattleAggressionProposal next{(f.forward_status&0xfaeef378u&0xff7fffffu)|0x80008118u,(f.reverse_status&0xfaeef378u&0xff7fffffu)|0x80008118u,reset?0:f.forward_combat_count,reset?0:f.reverse_combat_count,(uint32_t)reset};
 out=next;return E::ok;
}
}
