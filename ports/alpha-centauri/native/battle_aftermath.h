/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_BATTLE_AFTERMATH_H
#define AC_ALPHA_BATTLE_AFTERMATH_H
#include "land_resolution.h"
#include "world_battle_bookkeeping.h"
namespace alpha {
enum class BattleAftermathError {ok,bad_state,unsupported,out_of_memory,overflow,monument_ui};
struct BattleAftermathRuntime {
 BookkeepingObservation observation;BookkeepingEdition edition;
 int32_t current_player;uint32_t multiplayer_active,pbem_active,start_year;
 uint32_t wins[8],losses[8];
};
/* Conventional solitary pair deletion + selected original caller bookkeeping.
 * Pre-cleanup faction identities are captured before dense actor removal.
 * Retreat, artifact capture, base tiles and additional colocated actors remain
 * held. Original caller pool setup must be explicit present zero accumulators.
 * No legal attack, complete caller, session, event, turn or save commit.
 */
class BattleAftermathPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?after_.frame():nullptr;}
  const LandResolutionSummary*summary()const{return ready_?&summary_:nullptr;}
  const RemovalRuntime*runtime()const{return ready_?&runtime_:nullptr;}
  const ResolutionPools*accumulators()const{return ready_?&pools_:nullptr;}
  const BattleBookkeepingResult*bookkeeping()const{return ready_?after_.bookkeeping():nullptr;}
 private:
  friend BattleAftermathError stage_battle_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,BattleAftermathPlan&,size_t);
  BattleAftermathError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,size_t);
  void swap(BattleAftermathPlan&);
  LandResolutionPlan body_;WorldBattleBookkeepingPlan after_;
  LandResolutionSummary summary_{};RemovalRuntime runtime_{};ResolutionPools pools_{};bool ready_=false;
};
BattleAftermathError stage_battle_aftermath(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const LandResolutionFacts&,const BattleAftermathRuntime&,BattleAftermathPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
