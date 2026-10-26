/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_LAND_RESOLUTION_H
#define AC_ALPHA_LAND_RESOLUTION_H
#include "land_engagement.h"
#include "artifact_capture.h"
namespace alpha {
struct ResolutionPools {CasualtyAccumulator killed,captured,native_reward;};
struct LandResolutionFacts {
 EngagementFacts combat;
 RemovalRuntime removal;
 EngagementObservation caller_arguments;
 /* Raw source argument6 for each branch, supplied explicitly. The attacker-loss
  * stack caller and defender-loss caller must not be treated as the same route
  * or labeled with a universally inferred attacking/winning faction. */
 int32_t attacker_loss_argument6,defender_loss_argument6;
 ResolutionPools pools;
};
enum class LandResolutionKind {retreated,attacker_deleted,defender_deleted,defender_captured};
struct LandResolutionSummary {
 EngagementSummary engagement;
 LandResolutionKind kind;
 int32_t casualty_before,counterpart_before,argument6,source_casualty_return;
 int32_t attacker_after,defender_after;
};
/* Solitary noncargo conventional LAND pair: numeric engagement through one
 * immediate casualty/capture or retreat storage boundary. No source dialog,
 * diplomacy/statistics/cargo/full fight caller or legal session commit.
 * Attacker loss supplies counterpart -1; defender loss supplies the attacker.
 * Caller argument6 is explicit per branch. CRT state stays separate from World
 * RNG. A retreat carries NO RemovalRuntime claim (that stage does not own it).
 */
class LandResolutionPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const;
  const RemovalRuntime*runtime()const;
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const LandResolutionSummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend EngagementError stage_land_resolution(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,LandResolutionPlan&,size_t);
  EngagementError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,size_t);
  void swap(LandResolutionPlan&);
  LandEngagementPlan engagement_;
  CasualtyPlan deletion_;
  ArtifactCapturePlan capture_;
  ResolutionPools pools_{};LandResolutionSummary summary_{};bool ready_=false;
};
EngagementError stage_land_resolution(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,const LandResolutionFacts&,LandResolutionPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
