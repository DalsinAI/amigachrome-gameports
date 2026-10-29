/* SPDX-License-Identifier: GPL-3.0-or-later. See retreat_init.h. */
#include "retreat_init.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError retreat_initialization(const RetreatInitFacts&c,RetreatInitResult&out){
 if(c.attacker_triad>2||c.defender_triad>2||c.defender_order>255||c.defender_weapon < -128||c.defender_weapon>127||c.stack_count>32767||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 CombatHealth h;const auto e=combat_health(c.defender,h);if(e!=LogicError::ok)return e;
 const bool eligible=c.attacker_speed<c.defender_speed&&!c.fight_flags&&c.defender_weapon!=0&&!(c.defender_state&64u)&&c.attacker_triad!=2&&c.defender_triad!=2&&(!c.attacker_jammer||(c.expansion&&c.defender_dissociative))&&c.defender_order!=2&&!c.physical_base&&!(c.tile_items&0x40800u)&&c.stack_count==1;
 const RetreatInitResult r{h.remaining,eligible?(h.remaining+1)/2:0};out=r;return LogicError::ok;
}
}
