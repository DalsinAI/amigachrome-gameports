/* SPDX-License-Identifier: GPL-3.0-or-later
 * Integration of the engine packages: wires the WP1 turn hooks (turn.h) to
 * the routines of packages that have landed. Hooks whose owner has not landed
 * stay null, and WP1 skips them:
 *   world_climate (WP7; base_deps.cpp only has a placeholder), pact_ends,
 *   can_call_council, call_council (WP8), plans_upkeep, probe_upkeep,
 *   move_upkeep (WP6), consider_designs (WP5/WP6), interlude (UI),
 *   crash_landing, energy_limit, random_base_event (executable only).
 * Installed:
 *   mon_tech_discovered -> WP2 base_monument.cpp (Thinker monument.cpp).
 */
#include "engine.h"
#include "turn.h"
#include "base_wp2.h"
#include "integration.h"

namespace alpha {
namespace engine {

void engine_install_turn_hooks() {
    TurnHooks& h = turn_hooks();
    h.mon_tech_discovered = mon_tech_discovered;
}

/* Installed once at start-up so every program that links the engine gets the
 * same turn behaviour; turn_hooks() is zero-initialised before this runs. */
[[maybe_unused]] static const bool s_hooks_installed = (engine_install_turn_hooks(), true);

} // namespace engine
} // namespace alpha
