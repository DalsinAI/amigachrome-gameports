/* SPDX-License-Identifier: GPL-3.0-or-later. Original-backed stage attribution
 * and original instruction differences are documented in the included APIs. */
#ifndef AC_ALPHA_LAND_COMBAT_H
#define AC_ALPHA_LAND_COMBAT_H
#include "basic_combat.h"
namespace alpha {
struct LandCombatStrengths {int32_t offense,defense,offense_morale,defense_morale;uint32_t terrain,defense_multiplier,sensor_coverage;};
/* Read-only coherent flags0, conventional, nonmissile land/land numeric pipeline.
 * Both actors present, non-Probe attacker. Current World facts only; no legal
 * attack/mode/session authority, ledger/cache/RNG/UI or World writes. Other
 * branches remain held. Unused basic-strength validation can refuse conservatively.
 */
LogicError world_land_combat_strengths(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,LandCombatStrengths&);
}
#endif
