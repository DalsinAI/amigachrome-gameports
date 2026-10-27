/* SPDX-License-Identifier: GPL-3.0-or-later. Original promote 51a2f0/5062b0.
 * Original instruction behavior takes precedence over mod enhancements. */
#ifndef AC_ALPHA_WORLD_PROMOTION_H
#define AC_ALPHA_WORLD_PROMOTION_H
#include "land_engagement.h"
namespace alpha {
enum class PromotionError {ok,bad_state,unsupported,random_required,notification_required,out_of_memory,overflow};
struct PromotionRuntime {EngagementObservation observation;Variant edition;int32_t current_player;};
struct PromotionSummary {uint32_t raw_before,raw_after,effective_before;bool queried_morale,promoted;};
/* An already-selected recipient only, not caller promotion selection or a legal
 * attack. Own a current-frame clone and change only the recipient raw morale.
 * Qualified subset: nonnative/nonprobe, home=-1, nonrandom original branch.
 * Effective morale3..5 holds when a source random draw is needed. A current
 * human-player morale notification also holds without publishing any changes.
 * No zero/random TLS substitute, notification acknowledgment or RNG commit.
 */
class WorldPromotionPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?&storage_.frame_:nullptr;}
  const PromotionSummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend PromotionError stage_world_promotion(const OriginalWorld&,const UnitDefinitions&,const Rules&,size_t,const PromotionRuntime&,WorldPromotionPlan&,size_t);
  PromotionError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,size_t,const PromotionRuntime&,size_t);
  void swap(WorldPromotionPlan&);
  LandEngagementPlan storage_;
  PromotionSummary summary_{};bool ready_=false;
};
PromotionError stage_world_promotion(const OriginalWorld&,const UnitDefinitions&,const Rules&,size_t selected_recipient,const PromotionRuntime&,WorldPromotionPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
