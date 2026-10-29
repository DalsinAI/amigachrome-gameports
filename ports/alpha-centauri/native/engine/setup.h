/* SPDX-License-Identifier: GPL-3.0-or-later
 * New game setup (WP7). Declarations for engine/setup.cpp and
 * engine/newgame.cpp that are not already in engine_wp.h. Thinker names
 * follow src/game.h at dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see
 * engine/THINKER-LICENSE). Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

// Always available settings for random maps started from the main menu
// Select only those settings that are not set in Special Scenario Rules
const uint32_t GAME_RULES_MASK = 0x7808FFFF;
const uint32_t GAME_MRULES_MASK = 0xFFFFFFF0;

/* Special Scenario Rules picked in the new game settings (Thinker mod menu). */
extern uint32_t custom_game_rules;
extern uint32_t custom_more_rules;

} // namespace engine
} // namespace alpha
