/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original get_basic_offense/get_basic_defense semantics; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 * Original instruction behavior takes precedence over later mod fixes.
 */
#ifndef AC_ALPHA_BASIC_COMBAT_H
#define AC_ALPHA_BASIC_COMBAT_H
#include "combat_prototype.h"
namespace alpha {
struct BasicCombatContext {
 int32_t prototype,weapon_offense,armor_defense,plan,opposing_plan;
 int32_t initial_morale,active_se_morale,pending_se_morale;
 bool at_base,creche,brood,opponent_native,dissociative,soporific,offense_mode;
 int32_t factored_offense,factored_defense;
};
struct BasicCombatStrengths {int32_t offense,defense,offense_morale,defense_morale;bool fixed_defense;};
/* Explicit already resolved prototype/psi strengths and morale dependencies.
 * Original offense argument5 is retained as offense_mode; no attack authority.
 * Original HQ bonus mod fix is absent, combined Creche/Brood increments retained.
 * Checked original signed arithmetic; unsafe overflow rejects before output.
 */
LogicError basic_combat_strengths(Variant,const BasicCombatContext&,BasicCombatStrengths&);
/* Real opposing actor required. Physical base lookup follows stored map base bit;
 * stale set bit without matching base rejects before original rebuild/UI path.
 * Read-only query reports original cached morale values but does not install a
 * battle context, infer psi battle mode, mutate UI/RNG, or legalize attacks.
 */
LogicError world_basic_combat_strengths(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t actor,size_t opposing,bool psi_combat,
 bool bombard,bool offense_mode,BasicCombatStrengths&);
}
#endif
