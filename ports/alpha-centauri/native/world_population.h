/* SPDX-License-Identifier: GPL-3.0-or-later
 * Growth arithmetic: pinned Thinker default restoration policy, MIT,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af::mod_base_growth.
 * Objective query: complete OpenSMACX is_objective, Brendan Casey,
 * 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, GPL-3.0-or-later.
 */
#ifndef ALPHA_WORLD_POPULATION_H
#define ALPHA_WORLD_POPULATION_H
#include "colony.h"
#include "world_convoy_nutrients.h"
namespace alpha {
struct WorldPopulationStage {StableIntakeStage intake;bool full_turn;};
struct WorldPopulationProposal {
 ConvoyNutrientResult food;
 PopulationInput facts;
 PopulationTransition transition;
 bool recompute_required,starvation;
 uint32_t convoy_cancellations[64]; // actor indices, order becomes NONE
 uint32_t convoy_cancellation_count;
};
/* Fresh human base-game population proposal, not a mutation or EndTurn.
 * Food, convoys, growth/cost/limits/projects/objectives derive from current
 * decoded world/rules. Saved resource displays are never fresh input.
 * full_turn is an explicit scheduler fact, not inferred from a saved turn.
 * A population change requires worker reassignment and resource recomputation;
 * removal is an explicit event requiring the future base-kill transaction.
 * Inspection0/current directed-worker restrictions still apply.
 * All inputs and output must be disjoint; any failure leaves output unchanged.
 */
StableIntakeError world_population_begin(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t,const WorldPopulationStage&,
 WorldPopulationProposal&);
/* Derive begin AGAIN from the actual before world, then recompute food against
 * an owned after world containing exactly the proposed population/stored food
 * a legal freshly assigned worked mask, and the proposed convoy cancellations. No caller-supplied surplus or
 * cached proposal can complete the stage. Other decoded state and opaque bytes
 * must match; same-population old masks fail if the population changed.
 * The caller stages after-world writes privately before calling this function.
 * Starvation preserves the original pre-recompute storage assignment, while
 * exposing fresh post-loss food for downstream stages. No source is mutated.
 */
StableIntakeError world_population_finish(const OriginalWorld& before,
 const OriginalWorld& after,const Rules&,const BasicRules&,const ResourceInfo&,
 size_t,const WorldPopulationStage&,WorldPopulationProposal&);
}
#endif
