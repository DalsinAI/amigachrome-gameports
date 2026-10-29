/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_TERRITORIAL_AFTERMATH_H
#define AC_ALPHA_TERRITORIAL_AFTERMATH_H
#include "battle_aftermath.h"
#include "promoted_aftermath.h"
#include "territorial_promoted_body.h"
#include "world_battle_bookkeeping.h"
namespace alpha {
/* Conventional solitary pair deletion + selected original caller bookkeeping.
 * Pre-cleanup faction identities are captured before dense actor removal.
 * Retreat, artifact capture, base tiles and additional colocated actors remain
 * held. Original caller pool setup must be explicit present zero accumulators.
 * Territorial gift/friction is applied after rounds and before selection,
 * promotion and casualty cleanup. Explicit negotiation runtime is returned.
 * Original nonrandom promotion is applied before casualty cleanup. Required
 * selection/helper draws, promotion notification and monument UI remain held.
 * No legal attack, complete caller, session, event, turn or save commit.
 */
class TerritorialBattleAftermathPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?after_.frame():nullptr;}
  const WorldTerritorySummary*territorial_summary()const{return ready_?&territorial_:nullptr;}
  const TerritoryRuntime*diplomacy_runtime()const{return ready_?&diplomacy_:nullptr;}
  const PromotedBodySummary*summary()const{return ready_?&summary_:nullptr;}
  const RemovalRuntime*runtime()const{return ready_?&runtime_:nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const BattleBookkeepingResult*bookkeeping()const{return ready_?after_.bookkeeping():nullptr;}
 private:
  friend class HistoricalBattleAftermathPlan;
  friend PromotedAftermathError stage_territorial_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,TerritorialBattleAftermathPlan&,size_t);
  PromotedAftermathError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,size_t);
  void swap(TerritorialBattleAftermathPlan&);
  TerritorialPromotedLandPlan body_;WorldBattleBookkeepingPlan after_;
  WorldTerritorySummary territorial_{};TerritoryRuntime diplomacy_{};PromotedBodySummary summary_{};RemovalRuntime runtime_{};ResolutionPools pools_{};bool ready_=false;
};
PromotedAftermathError stage_territorial_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,const TerritoryRuntime&,TerritorialBattleAftermathPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
