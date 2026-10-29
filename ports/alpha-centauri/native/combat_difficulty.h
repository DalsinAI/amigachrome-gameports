/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_COMBAT_DIFFICULTY_H
#define AC_ALPHA_COMBAT_DIFFICULTY_H
#include "battle_entry.h"
namespace alpha {
struct CombatDifficultyFacts {
 BattleEntryObservation observation;Variant edition;
 uint32_t attacker_faction,defender_faction,human_status;
 int32_t attacker_difficulty,defender_difficulty,offense;
};
struct CombatDifficultyResult {int32_t offense;uint32_t applied;};
/* Original post-hasty/post-tutorial ordinary nonnative difficulty fragment.
 * Actual faction difficulties, status mask and offense AT THIS OBSERVATION
 * POINT are explicit. Upper status bits are ignored as in the originals.
 * Applied bit1: human defender handicap; bit2: human attacker advantage.
 * Arithmetic wraps signed words before truncating division. No native/base
 * special branches, saved/live status inference, World mutation, label/UI
 * obligations, original caller advancement or combat-round/session authority.
 */
BattleEntryError inspect_combat_difficulty(const CombatDifficultyFacts&,CombatDifficultyResult&);
}
#endif
