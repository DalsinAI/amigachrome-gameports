/* SPDX-License-Identifier: GPL-3.0-or-later
 * Vanilla cost: complete OpenSMACX technology.cpp::tech_rate005BE6B0 and
 * general.cpp::quick_root005290E0, Brendan Casey(C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Research contribution/progress
 * restoration uses pinned Thinker base.cpp/tech.cpp with vanilla start delays,
 * not its revised cost or early-start defaults. See RESEARCH-PROGRESSION.md.
 */
#ifndef ALPHA_RESEARCH_PROGRESS_H
#define ALPHA_RESEARCH_PROGRESS_H
#include <stdint.h>
namespace alpha {
enum class ResearchError {ok,bad_state,bad_buffer,overflow,unsupported,needs_target,needs_riot_resolution,out_of_memory};
struct OriginalResearchCostFacts {
 int32_t faction,difficulty,current_turn,discovery_rate_pct,rule_techcost,research_base,map_tiles;
 int32_t ranking[8],deferred_techs[8],ranking_debt;
 bool stagnation;
};
struct OriginalResearchCostResult {
 int32_t cost,map_root,player_factor,top_factor,difficulty_factor,final_factor,discovery_rate;
};
/* Always computes the original uncached vanilla cost. tech_cost is not an
 * input. Human cost path only; historical ranking/deferred/debt counters are
 * current gameplay state, not replaced with an ownership-count guess.
 * Conservative normal nonnegative history/custom-cost domain; overflowing
 * original intermediates reject instead of wrapping. No writes on failure. */
ResearchError original_research_cost(const OriginalResearchCostFacts&,OriginalResearchCostResult&);
struct ResearchLedger {int32_t labs_total,rounding_remainder,accumulated;};
struct ResearchContributionFacts {
 int32_t fresh_labs,pending_research,current_turn,difficulty,fresh_cost;
 ResearchLedger before;
 bool no_tech_advances,tutorial_messages,punishment_sphere;
};
enum class ResearchSuppression {none,initial_turn,no_tech_advances,tutorial,negative_research_start,low_difficulty_start};
struct ResearchContributionResult {
 ResearchLedger after;
 int32_t labs_accepted,research_points;
 bool completion;
 ResearchSuppression suppression;
};
/* Explicit vanilla-compatible contribution policy: turn0 and tutorial turns
 * through5 skip; negative RESEARCH startup and lowest-two-difficulty delays
 * apply, Sphere halves labs, fractional points carry in net_random_event.
 * If points>0, fresh_cost must be current original cost; crossing it completes
 * one target and discards surplus, like the source. Points0 cannot complete.
 * Caller owns the running ledger; stale displayed labs never qualify income.
 * This emits no acquisition, selection, research/world/turn mutation. */
/* Conversion only, without reading fresh_cost or adding to accumulated. */
ResearchError research_labs(const ResearchContributionFacts&,ResearchContributionResult&);
ResearchError research_contribution(const ResearchContributionFacts&,ResearchContributionResult&);
}
#endif
