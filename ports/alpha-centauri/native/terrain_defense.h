/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original defense_value; OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * Brendan Casey2013-2021. Original instructions take precedence over mod fixes.
 */
#ifndef AC_ALPHA_TERRAIN_DEFENSE_H
#define AC_ALPHA_TERRAIN_DEFENSE_H
#include "movement.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
struct TerrainDefenseContext {
 uint32_t altitude_byte,roughness_byte,items,owner_nibble;
 int32_t defender_faction,defender_prototype,defender_weapon,defender_triad;
 bool attacker_present;
 int32_t attacker_prototype,attacker_weapon,attacker_triad,attacker_faction;
 int32_t pholus_owner;
};
enum class TerrainDefenseLabel:uint32_t {unchanged=0,rocky=91,forest=291,fungus=338};
struct TerrainDefense {uint32_t multiplier;TerrainDefenseLabel label;};
/* Read-only original terrain multiplier and optional label replacement. Owner
 * is the stored tile nibble, not territory inference; Pholus owner is resolved
 * from the actual project's base. No defense infrastructure, legal attack,
 * battle cache write, strength application, damage, RNG or turn authority.
 * Both original editions use identical semantics. Invalid/overlap leaves output.
 */
LogicError terrain_defense(const TerrainDefenseContext&,TerrainDefense&);
/* Defender tile, stored owner bits, actual components and Pholus base owner.
 * Attacker -1 is the original absent-attacker inspection domain. Other negative
 * IDs reject. No inferred territory or live runtime authority from save bytes.
 */
LogicError world_terrain_defense(const OriginalWorld&,const UnitDefinitions&,
 size_t defender,int32_t attacker,TerrainDefense&);
}
#endif
