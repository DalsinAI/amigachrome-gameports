/* SPDX-License-Identifier: GPL-3.0-or-later. Independently implemented observed
 * original discovery reactor routing; inherited record layouts Thinker(MIT). */
#ifndef ALPHA_DISCOVERY_REACTOR_SEQUENCE_H
#define ALPHA_DISCOVERY_REACTOR_SEQUENCE_H
#include "discovery_reactor_stage.h"
namespace alpha {
/* kind0..3: history/scout/duplicate/reuse;4:allocation;5:no-slot refusal. */
struct DiscoveryReactorEvent {
 int32_t source,reactor,returned,kind,retired,history_cursor,last_created,source_flags;
};
class DiscoveryReactorSequencePlan {
 public:
  DiscoveryReactorSequencePlan()=default;
  DiscoveryReactorSequencePlan(const DiscoveryReactorSequencePlan&)=delete;
  DiscoveryReactorSequencePlan& operator=(const DiscoveryReactorSequencePlan&)=delete;
  bool ready()const{return ready_;}size_t count()const{return count_;}
  const DiscoveryReactorEvent* event(size_t i)const{return i<count_?events_+i:nullptr;}
  const OriginalPrototype* prototype(size_t i)const{return frame_.prototype(i);}
  const OriginalBase* base(size_t i)const{return frame_.base(i);}
  const OriginalFactionState* faction(size_t i)const{return frame_.faction(i);}
  const OriginalMapTile* tile(size_t i)const{return frame_.tile(i);}
  const OriginalVehicle* vehicle(size_t i)const{return frame_.vehicle(i);}
  const OriginalFactionDefinition* faction_definition(size_t i)const{return frame_.faction_definition(i);}
  const OriginalGamePreferences& preferences()const{return frame_.game().preferences;}
  const OriginalMapInfo& map_info()const{return frame_.game().map.info;}
  size_t vehicle_count()const{return frame_.game().vehicle_count;}
  uint32_t runtime_seed()const{return frame_.runtime_seed();}
  OriginalByteSpan monuments()const{return frame_.current_monuments();}
  OriginalByteSpan wire_baseline()const{return {frame_.original_bytes(),frame_.original_size()};}
  size_t base_count()const{return frame_.game().base_count;}
  OriginalByteSpan history()const{return {history_,ready_?size_t(4096):size_t(0)};}
  int32_t last_created()const{return -1;}int32_t upgrade_scratch()const{return upgrade_;}
  int32_t first_ui_cache()const{return -1;}int32_t second_ui_cache()const{return -1;}
  size_t memory_bytes()const{return frame_.memory_bytes()+sizeof(*this)-sizeof(OriginalWorld);}
 private:
  friend DiscoveryReactorError world_discovery_reactor_sequence(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorSequencePlan&,size_t);
  DiscoveryReactorError capture(const OriginalWorld&,size_t);
  DiscoveryReactorError apply_allocation(const UnreferencedAllocationPlan&,int32_t);
  void apply_existing(const ExistingProposalResult&);
  void apply_flags(const HumanPruningResult&);
  void swap(DiscoveryReactorSequencePlan&);
  OriginalWorld frame_;uint8_t history_[4096]{};DiscoveryReactorEvent events_[192]{};
  size_t count_=0;int32_t upgrade_=0;bool ready_=false;
};
/* Read-only full current decoded working frame for ordered base reactor calls.
 * Preserves original proposal/retirement/history/name/prune/queue order. All
 * actual cached counts are retained exactly; queues do NOT invent a recount.
 * Explicit runtime facts are data, never offline/session authority. Same human,
 * MPoff/both auto preferences OFF/already-owned/SkipA0 guards as the bounded
 * stage. Live-unit, unsupported strength/naming, local popup, SMAX/modal/network
 * and remaining discovery effects reject before any result publication.
 * No award, actual World/session mutation, raw savewriter or UI permission.
 * clone_limit bounds privately copied storage; bounded helper temporaries are
 * additional. All current decoded records/preferences/monuments are copied,
 * never reconstructed from stale imported bytes. Returned records are const.
 * Failed calls and every owned/raw/input alias preserve the prior plan/inputs.
 */
DiscoveryReactorError world_discovery_reactor_sequence(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorSequencePlan&,
 size_t clone_limit=64u*1024u*1024u);
}
#endif
