/* SPDX-License-Identifier: GPL-3.0-or-later. See research_progress.h. */
#ifndef ALPHA_WORLD_RESEARCH_H
#define ALPHA_WORLD_RESEARCH_H
#include "research_progress.h"
#include "world_technology_effects.h"
namespace alpha {
struct WorldResearchCost {OriginalResearchCostFacts facts;OriginalResearchCostResult result;};
ResearchError world_original_research_cost(const OriginalWorld&,const BasicRules&,int32_t faction,WorldResearchCost&);
struct WorldResearchStage {int32_t fresh_labs;ResearchLedger before;};
struct WorldResearchResult {
 ResearchContributionResult contribution;
 WorldResearchCost cost;
 int32_t target,target_after,ranking_after,ranking_debt_after,achieved_after;
 uint32_t owner_mask_after;
 TechnologyEffects effects_after;
 bool cost_computed,acquisition_required,next_choice_required,first_secrets_required;
};
/* Current-player/current-faction human base-game directed research proposal.
 * fresh_labs and the running ledger must be supplied by the current authoritative
 * stage; this low-level boundary does not infer freshness from a copied scalar.
 * Current selected target/prerequisites/ownership, rules, historical cost facts
 * and geometry are derived from world; saved labs/tech_cost are never inputs.
 * Completion emits acquisition/next-choice events plus proposed owner/counter/
 * fresh technology effects, not a silent acquisition. Prototype/free facility,
 * map, sharing/secrets, UI and scheduler effects remain required. No mutation.
 * Blind/AI/SMAX/repeated Transcendent Thought/pending bonus-secret paths reject.
 * All outputs and inputs disjoint; every failure leaves output unchanged. */
ResearchError world_base_research(const OriginalWorld&,const Rules&,const BasicRules&,
 size_t base,const WorldResearchStage&,WorldResearchResult&);
}
#endif
