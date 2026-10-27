/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2 ordinary
 * conventional LAND path; stage attributions are in the included APIs. */
#ifndef ALPHA_LAND_ENGAGEMENT_H
#define ALPHA_LAND_ENGAGEMENT_H
#include "land_combat.h"
#include "retreat_init.h"
#include "retreat_destination.h"
namespace alpha {
enum class EngagementError {ok,bad_state,unsupported,out_of_memory,overflow};
enum class EngagementObservation {missing,supplied};
struct EngagementFacts {EngagementObservation observation;uint32_t crt_state,incoming_direction;bool multiplayer_active;PodCoefficients pods;};
enum class EngagementBoundary {attacker_casualty,defender_casualty,retreated};
struct EngagementSummary {LandCombatStrengths strengths;uint32_t rounds,draws,initial_counter,remaining_counter,next_crt_state,attacker_damage,defender_damage,attacker_health,defender_health;int32_t retreat_direction,retreat_x,retreat_y;EngagementBoundary boundary;};
/* Owns a CURRENT decoded World clone and exact opaque baseline. A bounded
 * ordinary conventional engagement stages byte damage and solitary noncargo
 * retreat storage only. Casualty boundaries do NOT delete a unit or acknowledge
 * its remaining effects. CRT state is explicit, separate from World movement
 * RNG, and returned in the summary without committing either caller stream.
 * No legal-attack/session authority, UI, turn or original TLS initialization. */
class LandEngagementPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld* frame()const{return ready_?&frame_:nullptr;}
  const EngagementSummary* summary()const{return ready_?&summary_:nullptr;}
 private:
  friend class PromotedLandResolutionPlan;
  friend class WorldPromotionPlan;
  friend class WorldTerritorialCombatPlan;
  friend class LandResolutionPlan;
  friend EngagementError stage_land_engagement(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const EngagementFacts&,LandEngagementPlan&,size_t);
  EngagementError capture(const OriginalWorld&,size_t);
  void swap(LandEngagementPlan&);
  OriginalWorld frame_;
  uint8_t history_[4096]{};
  EngagementSummary summary_{};bool ready_=false;
};
EngagementError stage_land_engagement(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,const EngagementFacts&,LandEngagementPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
