/* SPDX-License-Identifier: GPL-3.0-or-later. See promotion_selection.h. */
#include "promotion_selection.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
int32_t scaled(uint32_t reactor,uint32_t health,int32_t strength,uint32_t divisor){return signed32(reactor*health*(uint32_t)strength)/(int32_t)divisor;}
}
LogicError inspect_promotion_selection(const PromotionSelectionContext&c,PromotionSelectionResult&out){
 if(detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 if(c.observation!=SelectionObservation::supplied||(c.outcome!=SelectionOutcome::attacker_lost&&c.outcome!=SelectionOutcome::attacker_won)||!c.initial_attacker_health||c.initial_attacker_health>9999||!c.initial_defender_health||c.initial_defender_health>9999||c.selected_effective_morale>6)return LogicError::bad_state;
 CombatHealth a{},d{};auto e=combat_health(c.attacker,a);if(e!=LogicError::ok)return e;e=combat_health(c.defender,d);if(e!=LogicError::ok)return e;
 const bool won=c.outcome==SelectionOutcome::attacker_won;
 const int32_t sa=won?scaled(c.attacker.reactor,a.remaining,c.attacker_strength,c.initial_attacker_health):scaled(c.attacker.reactor,c.initial_attacker_health,c.attacker_strength,a.power);
 const int32_t sd=won?scaled(c.defender.reactor,c.initial_defender_health,c.defender_strength,d.power):scaled(c.defender.reactor,d.remaining,c.defender_strength,c.initial_defender_health);
 PromotionSelectionResult result{PromotionIntent::promote,won?0u:1u,sa,sd};
 if((won?c.defender.plan:c.attacker.plan)==12)result.intent=PromotionIntent::none;
 else if(c.selected_effective_morale>1){
  // Original LEA(sum-1); TEST; JG uses signed WRAPPED bits, including the
  // INT_MIN sum edge. A zero roll is valid only on the actual no-draw branch.
  if(signed32((uint32_t)sa+(uint32_t)sd-1u)>0)result.intent=PromotionIntent::random_required;
  else if((won?sd:sa)<0&&(won||!c.target_base_present||c.selected_effective_morale>=4))result.intent=PromotionIntent::none;
 }
 if(result.intent!=PromotionIntent::promote)result.recipient=0xffffffffu;
 out=result;return LogicError::ok;
}
}
