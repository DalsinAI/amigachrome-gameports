/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_CASUALTY_H
#define AC_ALPHA_WORLD_CASUALTY_H
#include "world_vehicle_removal.h"
namespace alpha {
struct CasualtyAccumulator { bool present; int32_t value; };
struct CasualtyInputs {
 int32_t counterpart_actor;
 /* Raw battle_kill argument6; never inferred as winner faction. */
 int32_t faction_argument6;
 CasualtyAccumulator killed, captured, native_reward;
};
/* Bounded ordinary/native deletion route only. Artifacts and required interlude
 * remain unsupported, with no output/caller change. Reward is a caller pool,
 * never faction income. Owns current records/count/runtime and copied pools.
 * Wire baseline remains immutable; no session commit, save, attack, UI or turn.
 */
class CasualtyPlan {
 public:
  bool ready()const{return removal_.ready();}
  const OriginalWorld*frame()const{return removal_.frame();}
  const RemovalRuntime*runtime()const{return removal_.runtime();}
  const OriginalVehicle*removed()const{return removal_.removed();}
  const CasualtyInputs*accumulators()const{return ready()?&inputs_:nullptr;}
 private:
  friend class TerritorialPromotedLandPlan;
  friend class PromotedLandResolutionPlan;
  friend class LandResolutionPlan;
  friend RemovalError stage_casualty(const OriginalWorld&,size_t,const RemovalRuntime&,const CasualtyInputs&,CasualtyPlan&,size_t);
  RemovalError prepare(const OriginalWorld&,size_t,const RemovalRuntime&,const CasualtyInputs&,size_t);
  void swap(CasualtyPlan&);
  VehicleRemovalPlan removal_;
  CasualtyInputs inputs_{};
};
RemovalError stage_casualty(const OriginalWorld&,size_t victim,const RemovalRuntime&,const CasualtyInputs&,CasualtyPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
