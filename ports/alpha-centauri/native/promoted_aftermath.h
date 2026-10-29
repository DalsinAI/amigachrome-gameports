/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PROMOTED_AFTERMATH_H
#define AC_ALPHA_PROMOTED_AFTERMATH_H
#include "battle_aftermath.h"
#include "promoted_land_body.h"
#include "world_battle_bookkeeping.h"
namespace alpha {
enum class PromotedAftermathError {ok,bad_state,unsupported,out_of_memory,overflow,monument_ui,selection_random,promotion_random,promotion_notification};
/* Conventional solitary pair deletion + selected original caller bookkeeping.
 * Pre-cleanup faction identities are captured before dense actor removal.
 * Retreat, artifact capture, base tiles and additional colocated actors remain
 * held. Original caller pool setup must be explicit present zero accumulators.
 * Original nonrandom promotion is applied before casualty cleanup. Required
 * selection/helper draws, promotion notification and monument UI remain held.
 * No legal attack, complete caller, session, event, turn or save commit.
 */
class PromotedBattleAftermathPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?after_.frame():nullptr;}
  const PromotedBodySummary*summary()const{return ready_?&summary_:nullptr;}
  const RemovalRuntime*runtime()const{return ready_?&runtime_:nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const BattleBookkeepingResult*bookkeeping()const{return ready_?after_.bookkeeping():nullptr;}
 private:
  friend PromotedAftermathError stage_promoted_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,PromotedBattleAftermathPlan&,size_t);
  PromotedAftermathError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,size_t);
  void swap(PromotedBattleAftermathPlan&);
  PromotedLandResolutionPlan body_;WorldBattleBookkeepingPlan after_;
  PromotedBodySummary summary_{};RemovalRuntime runtime_{};ResolutionPools pools_{};bool ready_=false;
};
PromotedAftermathError stage_promoted_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,PromotedBattleAftermathPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
