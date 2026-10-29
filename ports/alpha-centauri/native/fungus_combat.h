/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute fungus attack arithmetic; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_FUNGUS_COMBAT_H
#define AC_ALPHA_FUNGUS_COMBAT_H
#include "terrain_defense.h"
namespace alpha {
struct FungusCombatContext {int32_t offense,attacker_prototype,attacker_weapon,defender_weapon;uint32_t altitude,items,attacker_faction;int32_t pholus_owner;uint32_t attacker_plan;};
struct FungusCombatStrength {int32_t offense;bool applied;};
/* Both actors present, flags0, non-Probe attacker, before Resonance/Empath.
 * Original adds offense/2 rather than multiplying by150. No output on errors.
 * Native design means prototype<64 with negative raw weapon, not faction0.
 */
LogicError fungus_combat_adjustment(const FungusCombatContext&,FungusCombatStrength&);
LogicError world_fungus_combat_adjustment(const OriginalWorld&,const UnitDefinitions&,size_t attacker,size_t defender,int32_t offense,FungusCombatStrength&);
}
#endif
