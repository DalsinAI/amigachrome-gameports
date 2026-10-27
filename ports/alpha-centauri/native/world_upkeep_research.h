/* SPDX-License-Identifier: GPL-3.0-or-later. See RESEARCH-PROGRESSION.md. */
#ifndef ALPHA_WORLD_UPKEEP_RESEARCH_H
#define ALPHA_WORLD_UPKEEP_RESEARCH_H
#include "world_research.h"
#include "world_upkeep_accounting.h"
namespace alpha {
struct WorldUpkeepResearchResult {
 WorldUpkeepAccountingResult accounting;
 WorldResearchResult research;
};
/* Current-world fresh worker/resource/energy/psych resolution -> actual labs ->
 * vanilla research proposal. Calls accounting itself, accepts no caller result
 * or lab scalar as freshness authority, and refuses unresolved riot events.
 * Before is the authoritative running faction research ledger; it is never
 * inferred from saved displayed lab totals. Earlier production/growth/events
 * must already be represented by the current owned world. Commerce/grid remain
 * explicit fresh upstream facts. All inputs, output and full state capacity
 * must be disjoint. Late failures preserve both outputs; only actor count
 * states are written on success, leaving any buffer tail untouched.
 * Completion still requires all acquisition/selection effects. No mutation,
 * persistent scheduler authority, research commit or turn advance is granted. */
ResearchError world_upkeep_research(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,const WorldRules&,const ResourceInfo&,
 const CitizenDefinitions&,size_t base,const WorkerResolutionStage&,
 const ResearchLedger&before,WorldUpkeepResearchResult&,
 uint32_t*projected_support_states,size_t capacity);
}
#endif
