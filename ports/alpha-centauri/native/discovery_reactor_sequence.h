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
enum class DiscoveryReactorBoundary {post_reactor,reactor_presentation};
/* Explicit observations of the two original UI cache words. Data only: neither
 * these words nor MapOwner establish permission to execute or resume a dialog. */
struct DiscoveryReactorBoundaryFacts {DiscoveryRuntimeObservation runtime;int32_t first_ui_cache,second_ui_cache;};
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
  int32_t last_created()const{return last_;}int32_t upgrade_scratch()const{return upgrade_;}
  int32_t first_ui_cache()const{return cache_a_;}int32_t second_ui_cache()const{return cache_b_;}
  DiscoveryReactorBoundary boundary()const{return boundary_;}
  int32_t pending_reactor()const{return reactor_;}
  const char* pending_reactor_name()const{return ready_ && boundary_==DiscoveryReactorBoundary::reactor_presentation?reactor_name_:nullptr;}
  size_t memory_bytes()const{return frame_.memory_bytes()+sizeof(*this)-sizeof(OriginalWorld);}
 private:
  friend DiscoveryReactorError world_discovery_reactor_sequence(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorSequencePlan&,size_t);
  friend DiscoveryReactorError discovery_reactor_sequence_impl(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts*,DiscoveryReactorSequencePlan&,size_t);
  DiscoveryReactorError capture(const OriginalWorld&,size_t);
  DiscoveryReactorError apply_allocation(const UnreferencedAllocationPlan&,int32_t);
  void apply_existing(const ExistingProposalResult&);
  void apply_flags(const HumanPruningResult&);
  void swap(DiscoveryReactorSequencePlan&);
  OriginalWorld frame_;uint8_t history_[4096]{};DiscoveryReactorEvent events_[192]{};
  size_t count_=0;int32_t upgrade_=0,last_=-1,cache_a_=-1,cache_b_=-1,reactor_=0;
  DiscoveryReactorBoundary boundary_=DiscoveryReactorBoundary::post_reactor;
  char reactor_name_[128]{};bool ready_=false;
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
/* Additive suspension query. Runs the same complete checked sequence, then
 * retains its privately owned intermediate frame at the actual local reactor
 * parse_say boundary instead of inventing a modal return. The request name is
 * owned, reactor is the last eligible target even if its proposer returned -1,
 * and original runtime caches/last-created/scratch are preserved before reset.
 * Local no-pair and nonlocal paths reach post_reactor with final resets.
 * There is deliberately NO resume, acknowledgment, nested-award, World commit,
 * UI execution or session-authority API. Later generic presentation, Secrets,
 * reveal, effects, sharing and first-Secrets notification are NOT executed.
 * Boundary facts must be supplied; complete input/output alias checks and
 * transactional failure behavior match the original sequence query.
 */
DiscoveryReactorError world_discovery_reactor_suspension(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,
 DiscoveryReactorSequencePlan&,size_t clone_limit=64u*1024u*1024u);
}
#endif
