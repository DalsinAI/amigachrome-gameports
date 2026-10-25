/* SPDX-License-Identifier: GPL-3.0-or-later
 * Movement expenditure adapted from Thinker order_veh/veh_skip (MIT).
 * Copyright (c) Thinker Mod authors; see THINKER-LICENSE.txt.
 * Source pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Integer RNG from OpenSMACX, Brendan Casey (C)2013-2021, pin2f7f4e24.
 */
#ifndef AC_ALPHA_MOVE_BUDGET_H
#define AC_ALPHA_MOVE_BUDGET_H
#include "game_logic.h"
namespace alpha {
struct MoveBudget {
  uint32_t speed, spent, state, random_seed;
};
struct MoveBudgetInput {
  uint32_t cost, road_rate;
  bool land, destination_fungus, native_unit, former_or_artifact;
  bool destination_occupied, human, attack_check, execute;
};
enum class MoveBudgetOutcome { exhausted, fractional_failure, preview, move };
struct MoveBudgetResult {
  MoveBudget budget;
  MoveBudgetOutcome outcome;
  bool drew_random;
  uint32_t random_value;
};
/* Source's offline integer-RNG budget stage ONLY, after legality checks.
 * Does not move a unit or authorize terrain access/combat/stacking.
 * speed is effective per-vehicle speed, not necessarily blueprint speed.
 * Preserves the source's over-budget successful moves and retry flag.
 * A failed fractional attempt spends the turn even when execute is false:
 * this matches order_veh, so callers wanting a pure UI preview must use a copy.
 * Multiplayer alternate RNG and tutorial early-outs are outside this API.
 * Rejects states beyond original byte-sized move storage, no wrapping writes.
 * Failure leaves output unchanged. No floating RNG or original globals.
 */
LogicError resolve_move_budget(const MoveBudget& before,const MoveBudgetInput& input,
                               MoveBudgetResult& output);
uint32_t integer_random(uint32_t& seed,uint32_t minimum,uint32_t maximum);
}
#endif
