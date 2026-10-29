/* SPDX-License-Identifier: GPL-3.0-or-later
 * Integration of the engine packages (engine/integration.cpp).
 * Include engine.h and turn.h first.
 */
#pragma once

namespace alpha {
namespace engine {

/* Set every WP1 turn hook whose owning package has landed (see
 * integration.cpp). Runs automatically at start-up; call it again after
 * replacing turn_hooks() to restore the defaults. Hooks without a landed
 * owner are left as they are. */
void engine_install_turn_hooks();

/* Clear the package runtime state that is not part of a saved game (WP8
 * diplomacy globals). Called by load_daemon_bytes() after a valid load and
 * by engine_new_game() before the new game is set up. */
void engine_reset_runtime();

} // namespace engine
} // namespace alpha
