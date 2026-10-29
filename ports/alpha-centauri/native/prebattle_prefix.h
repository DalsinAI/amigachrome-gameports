/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PREBATTLE_PREFIX_H
#define AC_ALPHA_PREBATTLE_PREFIX_H
#include "world_combat_text.h"
#include "world_prebattle_diplomacy.h"
#include "world_prebattle_territory.h"
#include "land_engagement.h"
namespace alpha {
enum class PrefixError {ok,bad_state,missing_runtime,unsupported,overflow,missing_label,text_too_long,notification_required,double_cross_required,participant_ui_required,out_of_memory};
struct PrebattlePrefixRuntime {
 BattleEntryObservation observation;
 uint32_t attack_flags;int32_t mode,execution_argument;
 uint32_t multiplayer_active,pbem_active,current_faction,human_status,game_flags,popup_flag,turn,prior_warning;
 uint32_t negotiation_faction,negotiation_other,negotiation_accumulator;
};
struct PrebattlePrefixSummary {
 WorldCombatTextInspection reporting;
 PrebattleNotificationProposal notification;
 PrebattleAggressionProposal aggression;
 WorldPrebattleTerritoryProposal territory;
};
/* Owns a source-order pre-round prefix clone, never the live source World.
 * Requires resolved original report text and only callback-free observer/silent
 * notification paths. Notification, double-cross and participant audio/redraw
 * paths are held. Stops AFTER third-party treaty/friction, BEFORE remaining
 * nerve/tutorial/display/stack/input/retreat/round gates. No legal action,
 * session commit, callback acknowledgment, CRT draw, filesystem save or turn.
 */
class PrebattlePrefixPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?&storage_.frame_:nullptr;}
  const PrebattlePrefixSummary*summary()const{return ready_?&summary_:nullptr;}
 private:
  friend PrefixError stage_prebattle_prefix(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const PrebattlePrefixRuntime&,const AttackModifierLedger&,const OriginalLabelsView&,const CombatTextLedger&,PrebattlePrefixPlan&,size_t);
  PrefixError prepare(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const PrebattlePrefixRuntime&,const AttackModifierLedger&,const OriginalLabelsView&,const CombatTextLedger&,size_t);
  void swap(PrebattlePrefixPlan&);
  LandEngagementPlan storage_;PrebattlePrefixSummary summary_{};bool ready_=false;
};
PrefixError stage_prebattle_prefix(const OriginalWorld&,const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,const PrebattlePrefixRuntime&,const AttackModifierLedger&,const OriginalLabelsView&,const CombatTextLedger&,PrebattlePrefixPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
