/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_TERRITORIAL_COMBAT_H
#define AC_ALPHA_WORLD_TERRITORIAL_COMBAT_H
#include "territorial_combat.h"
#include "land_engagement.h"
namespace alpha {
struct TerritoryRuntime {TerritoryObservation observation;Variant edition;uint32_t negotiation_faction,negotiation_other,negotiation_accumulator,multiplayer_active,pbem_active;};
enum class WorldTerritoryError {ok,bad_state,missing_runtime,unsupported,out_of_memory};
struct WorldTerritorySummary {uint32_t tile_index;int32_t owner,attacker_faction,defender_faction;TerritorialCombatResult effect;};
/* Owns a current World clone; derives all territorial facts from that frame.
 * The caller must supply the genuine post-round frame before promotion/deletion
 * and explicit active negotiation state. Does not prove attack authority or
 * commit runtime/session, movement, random, event, save or turn state. */
class WorldTerritorialCombatPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?&storage_.frame_:nullptr;}
  const WorldTerritorySummary*summary()const{return ready_?&summary_:nullptr;}
  const TerritoryRuntime*runtime()const{return ready_?&runtime_:nullptr;}
 private:
  friend WorldTerritoryError stage_world_territorial_combat(const OriginalWorld&,size_t,size_t,const TerritoryRuntime&,WorldTerritorialCombatPlan&,size_t);
  WorldTerritoryError prepare(const OriginalWorld&,size_t,size_t,const TerritoryRuntime&,size_t);
  void swap(WorldTerritorialCombatPlan&);
  LandEngagementPlan storage_;WorldTerritorySummary summary_{};TerritoryRuntime runtime_{};bool ready_=false;
};
WorldTerritoryError stage_world_territorial_combat(const OriginalWorld&,size_t attacker,size_t defender,const TerritoryRuntime&,WorldTerritorialCombatPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
