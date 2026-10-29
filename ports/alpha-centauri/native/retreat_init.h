/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2 retreat
 * initialization. OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * Brendan Casey2013-2021; original edition instructions take precedence. */
#ifndef AC_ALPHA_RETREAT_INIT_H
#define AC_ALPHA_RETREAT_INIT_H
#include "combat_round.h"
namespace alpha {
struct RetreatInitFacts {
 bool expansion; uint32_t attacker_speed,defender_speed,fight_flags;
 int32_t defender_weapon;
 uint32_t attacker_triad,defender_triad,defender_state,defender_order,tile_items,stack_count;
 bool attacker_jammer,defender_dissociative,physical_base;
 CombatHealthContext defender;
};
struct RetreatInitResult {uint32_t health,counter;};
LogicError retreat_initialization(const RetreatInitFacts&,RetreatInitResult&);
/* Read-only ordinary nonmissile land/land threshold. Derives current health,
 * speed(...,true), edition abilities and validated linked-stack count. Saved
 * facts are not active battle authority. No destination, relocation, countdown,
 * damage, RNG, UI, legal attack or controller commit. */
LogicError world_land_retreat_initialization(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,uint32_t fight_flags,RetreatInitResult&);
}
#endif
