/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_notification.h"
#include "technology_effects_ranges.h"
namespace alpha {
PrebattleNotificationError inspect_prebattle_notification(const PrebattleNotificationFacts&f,PrebattleNotificationProposal&out){
 using E=PrebattleNotificationError;using R=PrebattleNotificationRoute;
 if(detail::effects_range_overlap(&f,sizeof f,&out,sizeof out))return E::bad_state;
 if(f.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if((uint32_t)f.edition>1||f.attacker>7||f.defender>7||f.current_faction>7)return E::bad_state;
 if(!f.attacker||!f.defender||f.attacker==f.defender)return E::unsupported;
 const uint32_t sf=f.status[f.attacker*8+f.defender],sr=f.status[f.defender*8+f.attacker];
 const bool surprise=(f.human_status&(1u<<f.defender))&&
  !((sr&6u)==0&&(((sr&8u)==0&&(sf&0x800u)==0)||(sr&16u)!=0));
 PrebattleNotificationProposal next{R::observer,f.prior_warning,f.prior_spoke};
 if(surprise){
  next.route=f.current_faction==f.defender?R::current_defender:f.popup_flag?R::remote_popup:R::silent_contact;
  if(next.route==R::silent_contact)next.spoke=f.turn;
 }else if(((sf|sr)&2u)&&f.current_faction!=f.attacker&&f.current_faction!=f.defender){
  if(f.status[f.current_faction*8+f.defender]&1u)next.warning=2;
  else if(f.status[f.current_faction*8+f.attacker]&1u)next.warning=3;
  else if(f.game_flags&0x80u)next.warning=1;
 }
 out=next;return E::ok;
}
}
