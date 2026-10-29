/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker monument.cpp::mon_enemy_destroyed and game.cpp::game_year,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af; THINKER-LICENSE.txt.
 */
#ifndef ALPHA_WORLD_BATTLE_MONUMENT_H
#define ALPHA_WORLD_BATTLE_MONUMENT_H
#include "combat_rounds.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
struct BattleMonumentUpdate {
 bool changed;
 int32_t year,first_faction,enemy_faction,first_achiever;
};
/* Exact pinned default-policy first enemy-destroyed monument proposal. Calendar
 * is Thinker Rules.normal_start_year+CurrentTurn, not saved displayed year and
 * not an unrecovered original StartingMissionYear runtime global. Source scans
 * all8 monument records, last/highest qualifying first-achiever index wins.
 * Alien winner/already achieved are no-ops. No global monument popup/mutation.
 */
CombatError world_battle_monument(const OriginalWorld&,const BasicRules&,
 int32_t winner,int32_t enemy,BattleMonumentUpdate&);
}
#endif
