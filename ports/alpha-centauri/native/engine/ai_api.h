/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: public entry points beyond engine_wp.h.
 * Include engine.h first. The engine_wp.h WP6 functions (mod_enemy_turn,
 * enemy_strategy, mod_social_ai, mod_tech_ai, select_build, alien_fauna, ...)
 * are the main interface; these are the remaining Thinker routines that the
 * turn loop (WP1 turn hooks), the save loader and the UI call.
 */
#pragma once

namespace alpha {
namespace engine {

/* Work done by the AI since the last ai_stats_reset(), to judge the 68040
 * turn budget (see ENGINE-WP6.md). Counters only grow; they never affect play. */
struct AIStats {
    uint32_t enemy_turns;       // mod_enemy_turn calls
    uint32_t veh_moves;         // mod_enemy_move calls
    uint32_t move_upkeeps;      // move_upkeep calls (priority map rebuilds)
    uint32_t tile_search_steps; // tiles visited by TileSearch::get_next
    uint32_t path_find_calls;   // Path_find queries made by the AI
    uint32_t build_choices;     // select_build calls
    uint32_t strategy_calls;    // enemy_strategy calls
};
const AIStats& ai_stats();
void ai_stats_reset();

/* Thinker plans_upkeep (src/plan.cpp): per-faction plans, unit designs and
 * former targets. Called by faction upkeep (WP1 hook plans_upkeep). */
void ai_plans_upkeep(int faction_id);
/* Thinker move_upkeep (src/move.cpp): rebuild the priority maps for a faction.
 * mode 0 = UM_Full (WP1 hook move_upkeep), 1 = UM_Visual, 2 = UM_Player. */
void ai_move_upkeep(int faction_id, int mode);
/* Thinker reset_state (src/plan.cpp): drop cached AI state after a game is
 * loaded or started (savegame load and new game). */
void ai_reset_state();
/* Thinker probe_upkeep (src/probe.cpp) is not ported yet (probe teams are
 * deferred); returns 0 so WP1 can wire its hook unconditionally. */
int ai_probe_upkeep(int faction_id);

} // namespace engine
} // namespace alpha
