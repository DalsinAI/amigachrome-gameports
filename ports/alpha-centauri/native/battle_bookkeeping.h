/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_BATTLE_BOOKKEEPING_H
#define AC_ALPHA_BATTLE_BOOKKEEPING_H
#include <stdint.h>
namespace alpha {
enum class BookkeepingEdition:uint32_t {smac,smax};
enum class BookkeepingOutcome:uint32_t {attacker_lost,defender_lost};
enum class BookkeepingObservation:uint32_t {absent,supplied};
enum class BookkeepingError:uint32_t {ok,invalid,missing_runtime,network_notification,monument_ui};
/* Raw counter bits preserve original x86 wrapping, including signed overflow.
 * These are selected numeric fields, NOT a wire/native struct layout. */
struct EnemyDestroyedRecord {uint32_t year,opponent,leader,awarded,rank,focus;};
struct BattleBookkeepingState {
 uint32_t wins[8],losses[8],combat_total[8][8],combat_count[8][8];
 EnemyDestroyedRecord enemy_destroyed[8];
};
struct BattleBookkeepingContext {
 BookkeepingEdition edition;int32_t attacker,defender;
 BookkeepingOutcome outcome;int32_t current_player;
 uint32_t multiplayer_active,pbem_active;
 BookkeepingObservation observation;
 uint32_t turn,start_year,rule_flags[8];
};
struct BattleBookkeepingResult {
 BattleBookkeepingState before_cleanup,after_cleanup;
 uint32_t winner,loser,monument_changed;
};
/* Explicit offline numeric caller composition only. before_cleanup preserves
 * source statistics ordering; after_cleanup adds remaining statistics and the
 * enemy-destroyed monument record. No cleanup, event acknowledgment or session
 * mutation occurs. Current-player monument UI is held BEFORE publishing any
 * result. The source calls game_year even on inactive monument paths; its pure
 * uint32 addition has no externally visible state and is evaluated here too.
 * No artifact treaty/friction, base/cargo caller or full battle claim.
 * Overlap/invalid/held calls leave output untouched. Inputs stay unchanged.
 */
BookkeepingError preview_battle_bookkeeping(const BattleBookkeepingContext&,
 const BattleBookkeepingState&,BattleBookkeepingResult&);
}
#endif
