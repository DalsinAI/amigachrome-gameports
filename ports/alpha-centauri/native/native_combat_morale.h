/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from OpenSMACX veh.cpp::morale_alien, Brendan Casey (C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c; original edition differences retained.
 */
#ifndef AC_ALPHA_NATIVE_COMBAT_MORALE_H
#define AC_ALPHA_NATIVE_COMBAT_MORALE_H
#include "original_world.h"
#include "game_logic.h"
#include "rules_text.h"
namespace alpha {
struct NativeCombatMoraleContext {
 int32_t turn,unit_id,opponent_faction,major_atrocities,tectonic_detonations;
 uint32_t state,flags,fungus_neighbors;
};
/* Actual actor only (0..511 prototype); opponent -1 means no opposing faction.
 * Count qualifying adjacent fungus at ocean-shelf altitude or above, excluding
 * center. Tower override is SMAX only; no per-unit morale byte is consulted.
 * Output unchanged on bad input/source alias. No battle, action or RNG draw.
 */
LogicError native_combat_morale(Variant variant,const NativeCombatMoraleContext& context,uint32_t& output);
/* Saved-world query for a real actor. Opponent is a faction, not design owner.
 * Version11 with positive opponent rejects: tectonic runtime is absent in that
 * container and has not been reconstructed. Version12 reads its explicit LE
 * array; SMAC has no tectonic bonus. Does not infer missing runtime zeros.
 */
LogicError world_native_combat_morale(const OriginalWorld& world,size_t actor,int32_t opponent_faction,uint32_t& output);
}
#endif
