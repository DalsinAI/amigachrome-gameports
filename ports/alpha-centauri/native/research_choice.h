/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_MANUAL_RESEARCH_CHOICE_H
#define ALPHA_MANUAL_RESEARCH_CHOICE_H
#include <stdint.h>
namespace alpha {
struct ManualResearchState { int32_t target,accumulated,cached_cost; };
enum class ManualResearchStage:uint8_t { confirmation_declined,picker_returned };
enum class ManualResearchDisposition:uint8_t { declined,cancelled,same_target,initial_target,changed_target };
enum class ManualResearchError:uint8_t { ok,bad_stage,bad_choice };
struct ManualResearchResult {
 ManualResearchState state;
 ManualResearchDisposition disposition;
 /* Original caller synchronises after ANY picker return, including cancel.
  * This reports the unresolved boundary; it never executes synchronisation. */
 bool synchronisation_required;
};
bool manual_research_confirmation_required(const ManualResearchState&);
/* Pure manual Console_set_tech post-choice field transition, not tech_selection
 * or tech_research. No world/eligibility/pool/player/variant/network authority
 * is established here. Caller supplies a real chooser outcome AFTER handling
 * confirmation; negative results cancel, IDs0..88 are supported positive
 * results, larger IDs fail transactionally. Cached cost is always preserved.
 * Input may alias output.state. No callback, RNG, ownership or discovery runs.
 */
ManualResearchError manual_research_choice(const ManualResearchState&,ManualResearchStage,int32_t chosen,ManualResearchResult&);
}
#endif
