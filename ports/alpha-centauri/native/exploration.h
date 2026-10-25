/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_EXPLORATION_H
#define AC_ALPHA_EXPLORATION_H
#include "movement.h"
#include "move_budget.h"
namespace alpha {
/* An explicitly restricted, empty-land movement scenario, not a world save.
 * No enemies, pods, landmarks, projects, treaties or zone of control.
 * Optional explicitly owned base indices authorize only friendly land passage.
 * This slice admits only an undamaged predefined land unit without faction
 * speed bonuses. Terrain is supplied by the scenario, not generated as Planet.
 */
struct ExplorationUnit { size_t prototype;int32_t x,y;MoveBudget budget; };
enum class StepOutcome { moved, fractional_failure, exhausted, blocked };
struct StepResult { StepOutcome outcome;uint32_t cost; };
LogicError exploration_step(const MovementMap& map,const UnitDefinitions& definitions,
                            const MovementContext& context,int32_t direction,
                            ExplorationUnit& unit,StepResult& result,
                            const size_t* owned_bases=nullptr,size_t owned_base_count=0);
LogicError exploration_next_movement_turn(ExplorationUnit& unit,uint32_t& movement_turn);
}
#endif
