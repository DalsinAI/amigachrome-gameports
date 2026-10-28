/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5: weak definitions of helpers owned by other packages (see wp5_deps.h).
 * Bodies ported from Thinker Mod src/base.cpp, src/game.cpp and src/plan.cpp
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Integration: mod_bases_reset, unit_support_plan, base_can_riot,
 * mon_secrets_of_tech and dword_90EA3C are WP2's and full_game_turn is in
 * WP2's base_deps.cpp, so those weak copies were removed; SkipTechScreenB is
 * defined here (WP2 no longer has a copy). need_police stays weak for WP6.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 */
#include "engine.h"
#include "tech.h"
#include "wp5_deps.h"

namespace alpha {
namespace engine {

static int s_skip_tech_screen_b = 0;
int* SkipTechScreenB = &s_skip_tech_screen_b;

__attribute__((weak)) bool need_police(int faction_id) {
    Faction& f = Factions[faction_id];
    return f.SE_police > -2 && f.SE_police < 3 && !has_project(FAC_TELEPATHIC_MATRIX, faction_id);
}

} // namespace engine
} // namespace alpha
