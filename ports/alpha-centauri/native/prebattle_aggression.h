/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PREBATTLE_AGGRESSION_H
#define AC_ALPHA_PREBATTLE_AGGRESSION_H
#include "battle_entry.h"
namespace alpha {
enum class AggressionError {ok,bad_state,missing_runtime,unsupported,double_cross_required,participant_ui_required};
struct PrebattleAggressionFacts {
 BattleEntryObservation observation;Variant edition;
 uint32_t attacker,defender,current_faction,forward_status,reverse_status;
 int32_t forward_combat_count,reverse_combat_count;
};
struct PrebattleAggressionProposal {
 uint32_t forward_status,reverse_status;
 int32_t forward_combat_count,reverse_combat_count;
 uint32_t combat_counts_reset;
};
/* Real act_of_aggression -> treaty_on(0x8010), bounded nonnative observer path.
 * Actual current faction must differ from both participants; reverse low7 must
 * be clear, proving original double_cross and participant audio/redraw branches
 * absent. No guessed current faction, callback results or event completion.
 * Returns a proposal only; earlier SURPRISE/observer and subsequent territory
 * friction paths are NOT established by this helper. No World/caller commit.
 */
AggressionError inspect_prebattle_aggression(const PrebattleAggressionFacts&,PrebattleAggressionProposal&);
}
#endif
