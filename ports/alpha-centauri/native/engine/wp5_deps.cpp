/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5: weak definitions of helpers owned by other packages (see wp5_deps.h).
 * Bodies ported from Thinker Mod src/base.cpp, src/game.cpp and src/plan.cpp
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Integration: mod_bases_reset, unit_support_plan, base_can_riot,
 * mon_secrets_of_tech and dword_90EA3C are WP2's and full_game_turn is in
 * WP2's base_deps.cpp, so those weak copies were removed; SkipTechScreenB is
 * defined here (WP2 no longer has a copy). need_police is WP6's (integration.cpp).
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 */
#include "engine.h"
#include "tech.h"
#include "wp5_deps.h"

namespace alpha {
namespace engine {

static int s_skip_tech_screen_b = 0;
int* SkipTechScreenB = &s_skip_tech_screen_b;

} // namespace engine
} // namespace alpha
