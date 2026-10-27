/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_BATTLE_BOOKKEEPING_H
#define AC_ALPHA_WORLD_BATTLE_BOOKKEEPING_H
#include "original_world.h"
#include "battle_bookkeeping.h"
namespace alpha {
enum class WorldBookkeepingError {ok,bad_state,unsupported,monument_ui,out_of_memory};
struct WorldBookkeepingFacts {
 BookkeepingObservation observation;
 BookkeepingEdition edition;
 int32_t attacker_faction,defender_faction;
 BookkeepingOutcome outcome;
 int32_t current_player;
 uint32_t multiplayer_active,pbem_active,start_year;
 uint32_t wins[8],losses[8];
};
/* Caller-field binding only: input World may already contain casualty damage,
 * removal or capture changes. Faction IDs are PRE-cleanup caller identities,
 * never looked up through post-compaction actor IDs. Clone all current records,
 * update only combat matrices and six original monument fields, and copy runtime
 * statistics in the result. Explicit edition must match the supported imported
 * layout profile; cross-edition legacy-save fixup is not qualified. Does not execute cleanup or establish its occurrence.
 * No legal attack/session/event commit or active-player monument acknowledgment.
 */
class WorldBattleBookkeepingPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?&frame_:nullptr;}
  const BattleBookkeepingResult*bookkeeping()const{return ready_?&result_:nullptr;}
 private:
  friend WorldBookkeepingError stage_world_battle_bookkeeping(const OriginalWorld&,const WorldBookkeepingFacts&,WorldBattleBookkeepingPlan&,size_t);
  WorldBookkeepingError capture(const OriginalWorld&,size_t);
  WorldBookkeepingError prepare(const OriginalWorld&,const WorldBookkeepingFacts&,size_t);
  void swap(WorldBattleBookkeepingPlan&);
  OriginalWorld frame_;
  uint8_t history_[4096]{};
  BattleBookkeepingResult result_{};
  bool ready_=false;
};
WorldBookkeepingError stage_world_battle_bookkeeping(const OriginalWorld&,const WorldBookkeepingFacts&,WorldBattleBookkeepingPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
