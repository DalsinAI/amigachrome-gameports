/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_DIRECT_PROTOTYPE_RETIREMENT_H
#define ALPHA_DIRECT_PROTOTYPE_RETIREMENT_H
#include "unused_prototype_retirement.h"
namespace alpha {
enum class DirectRetirementError {ok,bad_state,live_callbacks_required,out_of_memory};
class DirectRetirementProposal {
 public:
  DirectRetirementProposal()=default;~DirectRetirementProposal(){clear();}
  DirectRetirementProposal(const DirectRetirementProposal&)=delete;
  DirectRetirementProposal& operator=(const DirectRetirementProposal&)=delete;
  void clear();bool ready()const{return ready_;}size_t count()const{return count_;}
  const OriginalVehicle* vehicle(size_t i)const{return i<count_?vehicles_+i:nullptr;}
  const OriginalFactionState& faction_state()const{return state_;}
  const RetirementHistoryResult& history()const{return history_;}
  const RetirementQueueProposal& queues()const{return queues_;}
  int32_t replacement()const{return replacement_;}size_t conversions()const{return conversions_;}
 private:
  friend DirectRetirementError world_direct_prototype_retirement(const OriginalWorld&,Variant,int32_t,int32_t,DirectRetirementProposal&);
  void swap(DirectRetirementProposal&);
  OriginalVehicle* vehicles_=nullptr;size_t count_=0,conversions_=0;
  OriginalFactionState state_{};RetirementHistoryResult history_{};RetirementQueueProposal queues_;
  int32_t replacement_=-1;bool ready_=false;
};
/* Complete original retirement proposal where every referencing vehicle takes
 * the ORIGINAL direct same-group fallback conversion branch; no-reference
 * cases also supported. Full history, actual reverse vehicle scan, modulo256
 * original byte count updates, all queues and active-bit clear are staged.
 * Human retirement faction, missing replacement, foreign owner or visible
 * non-base contexts needing kill/base_at/stack/salvage callbacks reject before
 * writes. Saved human membership is an explicit snapshot fact, NOT authority
 * for AI control/player actions. No choice, event, callback, award, World
 * commit/revision authority or save encoding is fabricated. Existing output
 * preserved on failure; disjoint from entire owning World allocations.
 */
DirectRetirementError world_direct_prototype_retirement(const OriginalWorld&,Variant,int32_t faction,int32_t id,DirectRetirementProposal&);
}
#endif
