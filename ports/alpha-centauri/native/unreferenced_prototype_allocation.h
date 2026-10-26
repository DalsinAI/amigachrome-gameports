/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UNREFERENCED_PROTOTYPE_ALLOCATION_H
#define ALPHA_UNREFERENCED_PROTOTYPE_ALLOCATION_H
#include "inactive_prototype_allocation.h"
#include "unused_prototype_retirement.h"
namespace alpha {
class DiscoveryReactorPlan;
struct DiscoveryReactorFacts;
enum class DiscoveryReactorError;
enum class UnreferencedAllocationError {ok,bad_state,runtime_required,existing_return_required,strength_required,no_slot,live_reference_required,naming_context_required,naming_nonreturning,overflow,out_of_memory};
class UnreferencedAllocationPlan {
 public:
  bool ready()const{return ready_;}bool retired()const{return retired_;}
  const InactiveAllocationPlan& allocation()const{return allocation_;}
  const RetirementHistoryResult& history()const{return history_;}
 private:
  friend class DiscoveryReactorPlan;
  friend DiscoveryReactorError world_discovery_reactor_stage(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorPlan&);
  void swap(UnreferencedAllocationPlan&);
  friend UnreferencedAllocationError world_unreferenced_prototype_allocation(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,const ExistingProposalRequest&,const InactiveAllocationFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,UnreferencedAllocationPlan&);
  InactiveAllocationPlan allocation_;RetirementHistoryResult history_{};bool ready_=false,retired_=false;
};
/* Read-only complete base human automatic-proposal plan for selected inactive
 * or ACTIVE NO-LIVE-REFERENCE winner. Actual ordered retirement history/cursor/
 * queues/deactivation precede construction; naming consumes changed owned history.
 * Full candidate/prune/queue/cursor effects owned, no World/award/authority commit.
 * Live-reference, modal/network/AI, SMAX provenance and sharing paths remain held.
 * Observations are input data, not authority. Errors/aliases preserve old plan.
 */
UnreferencedAllocationError world_unreferenced_prototype_allocation(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const ExistingProposalRequest&,const InactiveAllocationFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,UnreferencedAllocationPlan&);
}
#endif
