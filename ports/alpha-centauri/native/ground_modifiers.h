/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute ground modifiers; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_GROUND_MODIFIERS_H
#define AC_ALPHA_GROUND_MODIFIERS_H
#include "terrain_defense.h"
namespace alpha {
struct GroundModifierContext {
 int32_t offense,defense; // incoming after earlier faction/other modifiers
 uint32_t terrain,roughness;bool physical_base;
 int32_t attacker_speed,defender_speed,road_rate;
 uint32_t attacker_altitude,defender_altitude;bool attacker_antigrav,defender_antigrav;
 int32_t mobile_percent,rough_percent,road_percent,uphill_percent,downhill_percent;
};
struct GroundModifierStrengths {int32_t offense,defense;uint32_t applied;};
/* Normal combat_type0, conventional land attacker. Before terrain/base/sensor
 * multiplication. Original equal-speed open-ground and unconditional road
 * percentage preserved. Applied bits1/2/4/8/16 report numeric branches only,
 * not writes to a four-entry presentation ledger. Checked signed arithmetic.
 */
LogicError ground_combat_modifiers(const GroundModifierContext&,GroundModifierStrengths&);
/* Both actual actors must be land units. Resolves original prototype speed
 * (not actor elite/damage speed), actual effective Antigrav and source map facts.
 * Explicit incoming strengths; no inferred psi/attack/session authority.
 */
LogicError world_ground_combat_modifiers(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,
 int32_t offense,int32_t defense,GroundModifierStrengths&);
}
#endif
