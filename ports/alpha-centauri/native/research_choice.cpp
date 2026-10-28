/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented from instruction-backed original field semantics.
 * Original post-chooser boundary and scope documented in RESEARCH-CHOICE.md.
 */
#include "research_choice.h"
namespace alpha {
bool manual_research_confirmation_required(const ManualResearchState& state) {
 return state.target>=0 && state.accumulated!=0;
}
ManualResearchError manual_research_choice(const ManualResearchState& before,ManualResearchStage stage,int32_t chosen,ManualResearchResult& output) {
 if(stage!=ManualResearchStage::confirmation_declined && stage!=ManualResearchStage::picker_returned)return ManualResearchError::bad_stage;
 ManualResearchResult result{before,ManualResearchDisposition::declined,false};
 if(stage==ManualResearchStage::picker_returned) {
  if(chosen>88)return ManualResearchError::bad_choice;
  result.synchronisation_required=true;
  if(chosen<0)result.disposition=ManualResearchDisposition::cancelled;
  else if(chosen==before.target)result.disposition=ManualResearchDisposition::same_target;
  else {
   result.disposition=before.target<0?ManualResearchDisposition::initial_target:ManualResearchDisposition::changed_target;
   if(before.target>=0)result.state.accumulated=before.accumulated/2;
   result.state.target=chosen;
  }
 }
 output=result;
 return ManualResearchError::ok;
}
}
