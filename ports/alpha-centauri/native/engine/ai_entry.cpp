/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: public wrappers declared in ai_api.h and the AI work
 * counters. New code (no Thinker counterpart).
 */
#include "ai.h"

namespace alpha {
namespace engine {

namespace ai {
AIStats stats = {};
} // namespace ai

const AIStats& ai_stats() {
    return ai::stats;
}

void ai_stats_reset() {
    ai::stats = AIStats();
}

void ai_plans_upkeep(int faction_id) {
    ai::plans_upkeep(faction_id);
}

void ai_move_upkeep(int faction_id, int mode) {
    ai::move_upkeep(faction_id, (ai::UpdateMode)clamp(mode, (int)ai::UM_Full, (int)ai::UM_Player));
}

void ai_reset_state() {
    ai::reset_state();
}

int ai_probe_upkeep(int UNUSED(faction_id)) {
    return 0;
}

} // namespace engine
} // namespace alpha
