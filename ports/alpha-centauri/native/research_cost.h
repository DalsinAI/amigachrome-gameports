/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_ORIGINAL_RESEARCH_COST_H
#define ALPHA_ORIGINAL_RESEARCH_COST_H
#include "original_world.h"
#include "game_logic.h"
namespace alpha {
/* Explicit owned snapshot of original tech_rate inputs, not callback stubs.
 * faction0 participates only when queried; the original maximum scans1..7.
 */
struct ResearchCostFaction {int32_t cached_cost,ranking,earned,ranking_adjustment,difficulty,research_base,rule_techcost;};
struct ResearchCostContext {
 ResearchCostFaction factions[8];
 int32_t faction,discovery_rate,turn,global_difficulty,map_area_sqrt;
 uint8_t human_mask;
 bool stagnation,map_area_sqrt_known;
};
struct ResearchCostResult {int32_t cost;bool cached,research_disabled;};
/* Preserves original signed32 operations when representable. Unsafe original
 * overflow returns overflow, not Thinker's widened/clamped alternate result.
 * Cache and zero-rate shortcuts require none of the unused computed inputs.
 * Output and caller context remain unchanged on failure.
 */
LogicError original_research_cost(const ResearchCostContext&,ResearchCostResult&);
/* Recovered integer quick_root, no floating point or approximate sqrt. */
LogicError original_research_map_root(int32_t tiles,int32_t&);
/* Binds saved inputs directly and recovers MapAreaSqRoot from owned tile_count
 * with original quick_root (the same calculation as original load/map_init).
 */
LogicError world_research_cost(const OriginalWorld&,const BasicRules&,int32_t faction,ResearchCostResult&);
}
#endif
