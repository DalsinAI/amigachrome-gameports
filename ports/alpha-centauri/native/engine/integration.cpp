/* SPDX-License-Identifier: GPL-3.0-or-later
 * Integration of the engine packages: wires the WP1 turn hooks (turn.h) to
 * the routines of packages that have landed. Hooks whose owner has not landed
 * stay null, and WP1 skips them:
 *   pact_ends,
 *   can_call_council, call_council (WP8), plans_upkeep, probe_upkeep,
 *   move_upkeep (WP6), interlude (UI), energy_limit,
 *   random_base_event (executable only).
 * Installed:
 *   mon_tech_discovered -> WP2 base_monument.cpp (Thinker monument.cpp);
 *   consider_designs    -> WP5 design.cpp (time warp);
 *   world_climate       -> WP7 (Thinker map.cpp);
 *   crash_landing       -> WP7 newgame.cpp (turn 0 landing popup).
 */
#include "engine.h"
#include "turn.h"
#include "base_wp2.h"
#include "design.h"
#include "newgame.h"
#include "wp3_deps.h"
#include "integration.h"

namespace alpha {
namespace engine {

void engine_install_turn_hooks() {
    TurnHooks& h = turn_hooks();
    h.mon_tech_discovered = mon_tech_discovered;
    h.consider_designs = consider_designs;
    h.world_climate = world_climate;
    h.crash_landing = crash_landing;
}

/* Installed once at start-up so every program that links the engine gets the
 * same turn behaviour; turn_hooks() is zero-initialised before this runs. */
[[maybe_unused]] static const bool s_hooks_installed = (engine_install_turn_hooks(), true);

} // namespace engine
} // namespace alpha
