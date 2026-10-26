/* SPDX-License-Identifier: GPL-3.0-or-later. Independent continuation ordering;
 * inherited effect adaptations retain their own GPL/MIT source attributions. */
#ifndef ALPHA_DISCOVERY_CONTINUATION_H
#define ALPHA_DISCOVERY_CONTINUATION_H
#include "discovery_reactor_sequence.h"
namespace alpha {
enum class DiscoveryContinuationBoundary {reactor_presentation,generic_presentation,datalinks_award,techshare_award,first_secrets_tail,first_secrets_presentation,lifecycle_cleanup};
struct DiscoveryContinuationFacts {
 DiscoveryRuntimeObservation runtime;
 // Actual captured globals/caller facts, not values inferred as active authority.
 int32_t game_state,game_year,source,share,first_ever;
};
struct DiscoverySharedRequest {int32_t faction,technology,source,share;};
enum class DiscoveryContinuationError {ok,bad_state,runtime_required,reactor_held,effect_held,overflow};
class DiscoveryContinuationPlan {
 public:
  DiscoveryContinuationPlan()=default;
  DiscoveryContinuationPlan(const DiscoveryContinuationPlan&)=delete;
  DiscoveryContinuationPlan& operator=(const DiscoveryContinuationPlan&)=delete;
  bool ready()const{return ready_;}
  DiscoveryContinuationBoundary boundary()const{return boundary_;}
  const DiscoveryReactorSequencePlan& state()const{return state_;}
  bool has_shared_request()const{return ready_ && (boundary_==DiscoveryContinuationBoundary::datalinks_award || boundary_==DiscoveryContinuationBoundary::techshare_award);}
  const DiscoverySharedRequest* shared_request()const{return has_shared_request()?&shared_:nullptr;}
  const char* pending_secrets_technology_name()const{return ready_ && boundary_==DiscoveryContinuationBoundary::first_secrets_presentation?secrets_name_:nullptr;}
  const DiscoveryContinuationFacts& captured()const{return captured_;}
  int32_t faction()const{return faction_;}int32_t technology()const{return technology_;}
 private:
  friend DiscoveryContinuationError world_discovery_continuation(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,const DiscoveryContinuationFacts&,DiscoveryContinuationPlan&,size_t);
  friend DiscoveryContinuationError discovery_continuation_impl(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,const DiscoveryContinuationFacts&,DiscoveryContinuationPlan&,size_t,bool);
  DiscoveryContinuationError notice(const Rules&);
  DiscoveryContinuationError build(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,const DiscoveryContinuationFacts&,size_t);
  bool overlaps(const void*,size_t)const;
  void swap(DiscoveryContinuationPlan&);
  DiscoveryReactorSequencePlan state_;
  DiscoveryContinuationFacts captured_{};DiscoverySharedRequest shared_{};
  DiscoveryContinuationBoundary boundary_=DiscoveryContinuationBoundary::first_secrets_tail;
  int32_t faction_=0,technology_=0;char secrets_name_[128]{};bool ready_=false;
};
/* Bounded ordinary BASE continuation in a private owning staged frame. Calls
 * the complete reactor-suspension query first; then stops before a required
 * local/omniscient generic window. Otherwise runs complete nonlocal Secrets,
 * first map reveal and tech_effects recomputation in original order. Stops
 * BEFORE the first genuine recursive Datalinks/TECHSHARE award or the real
 * first-Secrets tail. Recursive requests are immutable obligations, NOT grants.
 * No acknowledge/resume, UI/network/constructor/destructor, nested award,
 * first-Secrets flag, research advance, World/session commit or authority API.
 * Input snapshots and facts stay unchanged; failed calls and all result/input
 * storage overlaps preserve prior output. Owned source/records/opaque baseline
 * and captured runtime are retained, without importing PC pointers.
 */
DiscoveryContinuationError world_discovery_continuation(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,
 const DiscoveryContinuationFacts&,DiscoveryContinuationPlan&,size_t clone_limit=64u*1024u*1024u);
/* Additive tail query: same exact earlier stops, plus the genuine post-sharing
 * first-Secrets predicate and flag0x40 effect. Stops BEFORE its first parse_say,
 * owns the technology name, and never acknowledges notification. When no notice
 * is required, stops BEFORE mandatory remaining object lifecycle. Does not run remaining
 * parse/popup/log/destructor or nested-award lifecycle. Original query retains
 * its pre-first-Secrets-tail behavior. All failure/alias guarantees unchanged. */
DiscoveryContinuationError world_discovery_notice_continuation(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,const DiscoveryReactorBoundaryFacts&,
 const DiscoveryContinuationFacts&,DiscoveryContinuationPlan&,size_t clone_limit=64u*1024u*1024u);
}
#endif
