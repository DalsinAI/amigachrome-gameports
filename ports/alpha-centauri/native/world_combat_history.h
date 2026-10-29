/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_COMBAT_HISTORY_H
#define AC_ALPHA_WORLD_COMBAT_HISTORY_H
#include "land_engagement.h"
namespace alpha {
struct CombatHistoryFacts {EngagementObservation observation;Variant edition;uint32_t attacker_design,defender_design,attacker_faction,defender_faction,interlude_pending;};
struct CombatHistorySummary {uint32_t attacker_design,defender_design,attacker_faction,defender_faction,attacker_before,defender_before,attacker_after,defender_after,shared_design;};
enum class CombatHistoryError {ok,bad_state,unsupported,out_of_memory};
/* Original saved-design combat_factions writes. Witness IDs must be retained
 * before casualty compaction, never recovered from a deleted/current actor.
 * Any required interlude stays held; no event/session/attack authority. */
class WorldCombatHistoryPlan {
 public:
  const OriginalWorld*frame()const{return ready_?&storage_.frame_:nullptr;}
  const CombatHistorySummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend class HistoricalBattleAftermathPlan;
  friend CombatHistoryError stage_world_combat_history(const OriginalWorld&,const CombatHistoryFacts&,WorldCombatHistoryPlan&,size_t);
  CombatHistoryError prepare(const OriginalWorld&,const CombatHistoryFacts&,size_t);
  void swap(WorldCombatHistoryPlan&);
  LandEngagementPlan storage_;CombatHistorySummary summary_{};bool ready_=false;
};
CombatHistoryError stage_world_combat_history(const OriginalWorld&,const CombatHistoryFacts&,WorldCombatHistoryPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
