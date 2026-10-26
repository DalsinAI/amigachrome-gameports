/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_INACTIVE_PROTOTYPE_ALLOCATION_H
#define ALPHA_INACTIVE_PROTOTYPE_ALLOCATION_H
#include "existing_prototype_reuse.h"
#include "human_component_pruning.h"
#include "proposer_record_stages.h"
namespace alpha {
enum class AllocationCursorObservation {unknown,supplied};
struct InactiveAllocationFacts {PruningRuntime multiplayer;AllocationCursorObservation cursor;int32_t last_created;int32_t difficulty;};
enum class InactiveAllocationError {ok,bad_state,runtime_required,existing_return_required,strength_required,no_slot,retirement_required,live_reference_required,naming_context_required,naming_nonreturning,overflow,out_of_memory};
class InactiveAllocationPlan {
 public:
  InactiveAllocationPlan()=default;~InactiveAllocationPlan(){clear();}
  InactiveAllocationPlan(const InactiveAllocationPlan&)=delete;InactiveAllocationPlan& operator=(const InactiveAllocationPlan&)=delete;
  void clear();bool ready()const{return ready_;}int32_t id()const{return id_;}
  int32_t last_created()const{return last_;}const OriginalPrototype& selected()const{return selected_;}
  uint16_t flags(size_t i)const{return i<512?flags_[i]:0;}
  size_t count()const{return count_;}const OriginalBase* base(size_t i)const{return i<count_?bases_+i:nullptr;}
 private:
  friend InactiveAllocationError world_inactive_prototype_allocation(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,const ExistingProposalRequest&,const InactiveAllocationFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,InactiveAllocationPlan&);
  void swap(InactiveAllocationPlan&);
  OriginalPrototype selected_{};uint16_t flags_[512]{};OriginalBase* bases_=nullptr;size_t count_=0;int32_t id_=-1,last_=-1;bool ready_=false;
};
/* Complete automatic-name allocation gameplay plan ONLY where original chosen
 * slot is inactive (real retirement call not taken), human normal-mode, observed
 * multiplayer OFF, saved auto-prune OFF, base edition, no selected live actor.
 * Exact slot scoring precedes construction, preparation, genuine logical naming,
 * store, candidate-aware prune, same-group obsolete queue replacement and cursor.
 * Runtime observations/plan are not authority. No World/award/sharing commit.
 * Active selected slots and unresolved original callbacks remain typed held.
 * All errors preserve old plan and all inputs, including owned output aliases.
 */
InactiveAllocationError world_inactive_prototype_allocation(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const ExistingProposalRequest&,const InactiveAllocationFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,InactiveAllocationPlan&);
}
#endif
