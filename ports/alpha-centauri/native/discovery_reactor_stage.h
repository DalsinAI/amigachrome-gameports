/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_DISCOVERY_REACTOR_STAGE_H
#define ALPHA_DISCOVERY_REACTOR_STAGE_H
#include "unreferenced_prototype_allocation.h"
namespace alpha {
enum class DiscoveryRuntimeObservation {unknown,supplied};
enum class SkipTechnologyScreenObservation {unknown,zero,nonzero};
struct DiscoveryReactorFacts {InactiveAllocationFacts allocation;ProposalMode proposal_mode;ExpansionObservation expansion;DiscoveryRuntimeObservation runtime;SkipTechnologyScreenObservation skip;int32_t map_owner,upgrade_scratch;};
enum class DiscoveryReactorError {ok,bad_state,runtime_required,stage_not_entered,award_context_required,ordered_sequence_required,reactor_presentation_required,proposal_required,live_reference_required,naming_context_required,naming_nonreturning,overflow,out_of_memory};
class DiscoveryReactorPlan {
 public:
  bool ready()const{return ready_;}bool has_proposal()const{return has_;}
  size_t proposal_count()const{return count_;}
  bool has_allocation()const{return has_ && !early_;}
  bool has_existing_return()const{return early_;}
  const ExistingProposalResult& existing_return()const{return existing_;}
  int32_t returned_id()const{return early_?existing_.id:(has_?proposal_.allocation().id():-1);}
  int32_t source_id()const{return source_;}int32_t reactor()const{return reactor_;}
  int32_t last_created()const{return -1;}int32_t upgrade_scratch()const{return upgrade_;}
  int32_t first_ui_cache()const{return -1;}int32_t second_ui_cache()const{return -1;}
  const UnreferencedAllocationPlan& proposal()const{return proposal_;}
  const OriginalPrototype& selected()const{return selected_;}
  uint16_t flags(size_t i)const{return i<512?flags_[i]:0;}
 private:
  friend DiscoveryReactorError world_discovery_reactor_stage(const OriginalWorld&,Variant,const Rules&,const UnitDefinitions&,int32_t,int32_t,const DiscoveryReactorFacts&,const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorPlan&);
  void swap(DiscoveryReactorPlan&);
  UnreferencedAllocationPlan proposal_;ExistingProposalResult existing_{};bool early_=false;size_t count_=0;OriginalPrototype selected_{};uint16_t flags_[512]{};int32_t source_=-1,reactor_=0,upgrade_=0;bool ready_=false,has_=false;
};
/* Complete bounded base ordinary reactor/auto-design-skip gameplay stage ONLY:
 * supplied runtime, human/MPoff, both original auto preferences OFF, World already
 * owns ordinary tech, and zero/one eligible reactor-source pair OR an ordered sequence consisting
 * entirely of unchanged history/scout/duplicate returns. Multiple-return metadata
 * identifies the LAST event; proposal_count is the complete number of calls.
 * Ordered flags participate in subsequent coverage and source request flags.
 * Reuse/allocation mixed into a sequence rejects the entire plan before output. One-pair path
 * requires nonlocal map owner (real reactor popup otherwise held) and a complete
 * checked allocation OR exact existing early-return result. Calls the real derived automatic proposer request;
 * follows with staged prune/upgrade-wrapper effects when UID differs, including
 * genuine -1 refusals. Existing reuse changes are retained exactly; unchanged
 * history/scout/duplicate returns allocate no storage or fictional record.
 * Final last-created/UI-cache resets included. No ownership/rank grant, presentation,
 * sharing/discovery lifecycle, World mutation or active authority. Multiple
 * changing sequential proposals/live/modal/SMAX context paths remain typed held.
 */
DiscoveryReactorError world_discovery_reactor_stage(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,int32_t faction,int32_t acquired_technology,const DiscoveryReactorFacts&,
 const PrototypeNameLabels&,const PrototypeNameSuffix&,DiscoveryReactorPlan&);
}
#endif
