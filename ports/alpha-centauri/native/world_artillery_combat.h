/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in artillery_combat.h. */
#ifndef AC_ALPHA_WORLD_ARTILLERY_COMBAT_H
#define AC_ALPHA_WORLD_ARTILLERY_COMBAT_H
#include "artillery_combat.h"
namespace alpha {
/* Explicit numeric artillery inspection from actual actors/chassis/map/rules.
 * Caller supplies supported bit0x2 battle flags and incoming strengths. This
 * gives no artillery legality/range, active battle/session or UI/action authority.
 * Read-only World/RNG; output aliases and invalid components reject.
 */
LogicError world_artillery_combat(const OriginalWorld&,const UnitDefinitions&,
 const BasicRules&,size_t attacker,size_t defender,uint32_t battle_flags,
 int32_t offense,int32_t defense,ArtilleryStrengths&);
}
#endif
