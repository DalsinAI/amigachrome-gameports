/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned adaptations of OpenSMACX cost_factor (Brendan Casey C2013-2021,
 * pin2f7f4e24) and Thinker map yields/base_growth (MIT, see THINKER-LICENSE.txt,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af).
 */
#ifndef AC_ALPHA_COLONY_H
#define AC_ALPHA_COLONY_H
#include "game_logic.h"
namespace alpha {
struct HumanCostFactors { int32_t nutrient,mineral; };
LogicError calculate_human_cost_factors(const BasicRules& rules,int32_t map_size,
 int32_t effective_growth,int32_t industry,HumanCostFactors& output);
struct InitialLandTile { bool base,forest,river;uint8_t rockiness,moisture; };
struct InitialYieldContext { int32_t economy,base_count;bool headquarters;bool unlocked[3]; };
/* Only unspecial land/base/forest, no improvements, facilities, bonuses,
 * landmarks, fungus, projects, events, governor or dust/solar effects. Caller
 * must exclude those conditions. Uses original two-resource tech caps. */
LogicError initial_land_yield(const ResourceInfo& resources,const InitialLandTile& tile,
 const InitialYieldContext& context,ResourceValues& output);
struct PopulationInput {
 int32_t population,accumulated,surplus,nutrient_factor,growth,population_limit;
 int32_t nutrient_per_citizen;
 bool habitation_dome,cloning_vats,full_turn,objective;
};
struct PopulationTransition {
 int32_t population,accumulated;
 bool recompute,append_recomputed_surplus,removed;
 int32_t recomputed_accumulation_ceiling; // -1 for none; boom uses pre-growth cost
};
/* Resource state after a population change MUST be recomputed before finish.
 * No stale-population surplus is added. This is growth, not full base upkeep:
 * production, support, riots, notifications and convoy cancellations absent.
 * Caller must handle the explicit removed outcome and those external effects.
 */
LogicError begin_population_growth(const PopulationInput& input,PopulationTransition& output);
LogicError finish_population_growth(const PopulationTransition& pending,int32_t recomputed_accumulated,
                                    int32_t recomputed_surplus,
                                    PopulationTransition& output);
}
#endif
