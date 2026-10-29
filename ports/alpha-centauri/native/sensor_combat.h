/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute sensor stage; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_SENSOR_COMBAT_H
#define AC_ALPHA_SENSOR_COMBAT_H
#include "terrain_defense.h"
namespace alpha {
struct SensorCombatAdjustment {int32_t defense;uint32_t coverage;};
/* Explicit source coverage flags:0 none,1 ordinary,3 GSP (SMAX only).
 * Apply percentage in original signed order once/twice. No output on error.
 * This is a battle stage, not final strength, cache/UI or attack authority.
 */
LogicError sensor_combat_adjustment(Variant,int32_t defense,uint32_t coverage,
 int32_t percent,SensorCombatAdjustment&);
/* Read-only actual25-tile coverage, original nearest-base last-tie rule,
 * actual stored territory/GameRules/diplomacy and physical GSP facilities.
 * Original unsafe sea-array/no-base or invalid owner paths reject explicitly.
 */
LogicError world_combat_sensor_coverage(const OriginalWorld&,size_t defender,uint32_t&);
LogicError world_sensor_combat_adjustment(const OriginalWorld&,const BasicRules&,
 size_t defender,int32_t defense,SensorCombatAdjustment&);
}
#endif
