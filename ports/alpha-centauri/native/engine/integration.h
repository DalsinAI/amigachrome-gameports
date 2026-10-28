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

} // namespace engine
} // namespace alpha
