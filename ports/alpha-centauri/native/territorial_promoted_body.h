/* SPDX-License-Identifier: GPL-3.0-or-later. Included original-backed stage
 * attributions apply. A bounded body composition, not a complete battle caller. */
#ifndef AC_ALPHA_TERRITORIAL_PROMOTED_BODY_H
#define AC_ALPHA_TERRITORIAL_PROMOTED_BODY_H
#include "promoted_land_body.h"
#include "world_territorial_combat.h"
#include "world_promotion.h"
#include "promotion_selection.h"
namespace alpha {
/* Retain real pre-round health, run engagement, bind caller selection from the
 * current post-round frame, stage nonrandom promotion BEFORE dense casualty
 * compaction, then ordinary deletion. Every intermediate frame is privately
 * owned and budgeted. Any pending draw/notification rejects transactionally.
 * Nonbase, no additional colocation, noncargo/nonartifact conventional LAND
 * only. Territorial diplomacy precedes promotion. No retreat/capture, caller bookkeeping/movement/UI/session,
 * full caller CRT provenance, save, turn, AI or legal attack commit. */
class TerritorialPromotedLandPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?deletion_.frame():nullptr;}
  const RemovalRuntime*runtime()const{return ready_?deletion_.runtime():nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const WorldTerritorySummary*territorial_summary()const{return ready_?&territorial_:nullptr;}
  const TerritoryRuntime*diplomacy_runtime()const{return ready_?&diplomacy_:nullptr;}
  const PromotedBodySummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend class TerritorialBattleAftermathPlan;
  friend PromotedBodyError stage_territorial_promoted_body(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const PromotionRuntime&,const TerritoryRuntime&,TerritorialPromotedLandPlan&,size_t);
  PromotedBodyError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const PromotionRuntime&,const TerritoryRuntime&,size_t);
  void swap(TerritorialPromotedLandPlan&);
  LandEngagementPlan engagement_;WorldTerritorialCombatPlan territory_;WorldTerritorySummary territorial_{};TerritoryRuntime diplomacy_{};WorldPromotionPlan promotion_;CasualtyPlan deletion_;
  ResolutionPools pools_{};PromotedBodySummary summary_{};bool ready_=false;
};
PromotedBodyError stage_territorial_promoted_body(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,const LandResolutionFacts&,const PromotionRuntime&,const TerritoryRuntime&,TerritorialPromotedLandPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
