/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_COLONY_SCENARIO_H
#define AC_ALPHA_COLONY_SCENARIO_H
#include "colony.h"
#include "movement.h"
namespace alpha {
struct EarlyColony { int32_t x,y,population,food;uint32_t workers;bool removed; };
struct ColonyMap { MovementMap movement;const uint8_t* moisture;size_t count; };
struct ColonyYield { ResourceValues totals;NutrientResult nutrients; };
/* A neutral, human, early land colony only: no facilities beyond HQ, tech
 * unlocks, bonuses, projects, events, support, riots, specialists or production.
 * Twenty work slots use the pinned source's first21 TableOffset entries.
 */
LogicError colony_work_index(const ColonyMap& map,const EarlyColony& colony,unsigned slot,size_t& index);
LogicError early_colony_yields(const ColonyMap& map,const EarlyColony& colony,
 const ResourceInfo& resources,const BasicRules& rules,ColonyYield& output);
LogicError early_colony_toggle_worker(const ColonyMap& map,const ResourceInfo& resources,
 const BasicRules& rules,unsigned slot,EarlyColony& colony);
/* On starvation, excess work slots are removed highest-slot first as an
 * EXPLICIT SCENARIO UI allocation policy, not the original governor algorithm.
 * This runs nutrition/growth and final resource recomputation only; no complete
 * turn or base-upkeep claim. Outputs commit only after all phases succeed.
 */
LogicError early_colony_food_turn(const ColonyMap& map,const ResourceInfo& resources,
 const BasicRules& rules,EarlyColony& colony,ColonyYield& output);
}
#endif
