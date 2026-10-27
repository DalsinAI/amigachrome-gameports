/* SPDX-License-Identifier: GPL-3.0-or-later. Included original-backed stage
 * attributions apply. A bounded body composition, not a complete battle caller. */
#ifndef AC_ALPHA_PROMOTED_LAND_BODY_H
#define AC_ALPHA_PROMOTED_LAND_BODY_H
#include "land_resolution.h"
#include "world_promotion.h"
#include "promotion_selection.h"
namespace alpha {
enum class PromotedBodyError {ok,bad_state,unsupported,out_of_memory,overflow,selection_random,promotion_random,promotion_notification};
struct PromotedBodySummary {
 LandResolutionSummary body;PromotionSelectionResult selection;
 bool helper_called;PromotionSummary promotion;
 uint32_t initial_attacker_health,initial_defender_health;
};
/* Retain real pre-round health, run engagement, bind caller selection from the
 * current post-round frame, stage nonrandom promotion BEFORE dense casualty
 * compaction, then ordinary deletion. Every intermediate frame is privately
 * owned and budgeted. Any pending draw/notification rejects transactionally.
 * Nonbase, no additional colocation, noncargo/nonartifact conventional LAND
 * only. No retreat/capture, caller bookkeeping/friction/movement/UI/session,
 * full caller CRT provenance, save, turn, AI or legal attack commit. */
class PromotedLandResolutionPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?deletion_.frame():nullptr;}
  const RemovalRuntime*runtime()const{return ready_?deletion_.runtime():nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const PromotedBodySummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend PromotedBodyError stage_promoted_land_body(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const PromotionRuntime&,PromotedLandResolutionPlan&,size_t);
  PromotedBodyError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const PromotionRuntime&,size_t);
  void swap(PromotedLandResolutionPlan&);
  LandEngagementPlan engagement_;WorldPromotionPlan promotion_;CasualtyPlan deletion_;
  ResolutionPools pools_{};PromotedBodySummary summary_{};bool ready_=false;
};
PromotedBodyError stage_promoted_land_body(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,const LandResolutionFacts&,const PromotionRuntime&,PromotedLandResolutionPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
