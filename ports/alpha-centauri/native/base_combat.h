/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute base-infrastructure stage; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_BASE_COMBAT_H
#define AC_ALPHA_BASE_COMBAT_H
#include "terrain_defense.h"
namespace alpha {
struct BaseCombatContext {
 int32_t offense,defense; // incoming AFTER terrain multiplication
 bool attacker_present;
 int32_t attacker_prototype,attacker_weapon,attacker_triad,attacker_speed,attacker_faction;
 bool perimeter,naval_yard,aerospace,tachyon,citizens,maritime,cloudbase,blink;
 int32_t base_percent,infantry_percent;
};
struct BaseCombatAdjustment {
 int32_t offense,defense;uint32_t multiplier;
 bool base_percent_applied,infantry_percent_applied;
};
/* Only the conventional normal-combat (combat_type0) physical-base stage, with explicit incoming strengths
 * and facts. Does not apply prior terrain/faction or later sensor/ability
 * modifiers, install display caches, validate attacks or mutate World/RNG.
 * Applied flags report numeric branches, not appends to the4-entry UI ledger.
 * Checked original signed intermediates; error/overlap leaves output untouched.
 */
LogicError base_combat_adjustment(Variant,const BaseCombatContext&,BaseCombatAdjustment&);
/* Resolve actual defender physical base and facilities/project owner facts plus
 * actual attacker profile/effective Blink. Caller supplies incoming strengths
 * after terrain; this remains a stage query, not final battle strengths.
 */
LogicError world_base_combat_adjustment(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t defender,int32_t attacker,
 int32_t offense,int32_t defense,BaseCombatAdjustment&);
}
#endif
