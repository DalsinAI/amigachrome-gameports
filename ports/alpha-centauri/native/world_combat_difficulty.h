/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_COMBAT_DIFFICULTY_H
#define AC_ALPHA_WORLD_COMBAT_DIFFICULTY_H
#include "original_world.h"
#include "combat_difficulty.h"
namespace alpha {
struct WorldCombatDifficultyRuntime {BattleEntryObservation observation;uint32_t human_status;int32_t offense;};
/* Resolves actor faction IDs and actual current faction diff_level fields.
 * Human status and offense AT the post-hasty/post-tutorial observation remain
 * explicit caller facts; no saved-status fallback or automatic prior-boundary
 * advancement exists. Read-only, nonnative scope. Does not establish that the
 * original intervening treaty/nerve/tutorial/UI paths have actually completed.
 */
BattleEntryError inspect_world_combat_difficulty(const OriginalWorld&,size_t,size_t,
 const WorldCombatDifficultyRuntime&,CombatDifficultyResult&);
}
#endif
