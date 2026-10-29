/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_HISTORICAL_AFTERMATH_H
#define AC_ALPHA_HISTORICAL_AFTERMATH_H
#include "territorial_aftermath.h"
#include "world_combat_history.h"
namespace alpha {
class HistoricalBattleAftermathPlan {
 public:
  const OriginalWorld*frame()const{return ready_?history_.frame():nullptr;}
  const PromotedBodySummary*summary()const{return ready_?&summary_:nullptr;}
  const WorldTerritorySummary*territorial_summary()const{return ready_?&territorial_:nullptr;}
  const TerritoryRuntime*diplomacy_runtime()const{return ready_?&diplomacy_:nullptr;}
  const RemovalRuntime*runtime()const{return ready_?&runtime_:nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const BattleBookkeepingResult*bookkeeping()const{return ready_?&book_:nullptr;}
  const CombatHistorySummary*design_history()const{return ready_?history_.summary():nullptr;}
 private:
  friend PromotedAftermathError stage_historical_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,HistoricalBattleAftermathPlan&,size_t);
  PromotedAftermathError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,size_t);
  void swap(HistoricalBattleAftermathPlan&);
  TerritorialBattleAftermathPlan body_;WorldCombatHistoryPlan history_;
  PromotedBodySummary summary_{};WorldTerritorySummary territorial_{};TerritoryRuntime diplomacy_{};RemovalRuntime runtime_{};ResolutionPools pools_{};BattleBookkeepingResult book_{};bool ready_=false;
};
/* Nonbase/non-native inherited scope proves the original interlude trigger
 * absent (the sole setter requires native attacker AND a base). Saved design
 * IDs/factions are captured before any casualty, then history edits follow the
 * selected body/bookkeeping. Not complete caller/movement/event/legal attack. */
PromotedAftermathError stage_historical_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,HistoricalBattleAftermathPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
