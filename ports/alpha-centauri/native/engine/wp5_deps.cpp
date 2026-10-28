/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5: weak definitions of helpers owned by other packages (see wp5_deps.h).
 * Bodies ported from Thinker Mod src/base.cpp, src/game.cpp and src/plan.cpp
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 */
#include "engine.h"
#include "tech.h"
#include "wp5_deps.h"

namespace alpha {
namespace engine {

static int s_skip_tech_screen_b = 0;
static int s_upgrade_all = 0;
__attribute__((weak)) int* SkipTechScreenB = &s_skip_tech_screen_b;
__attribute__((weak)) int* dword_90EA3C = &s_upgrade_all;

__attribute__((weak)) void __cdecl mod_bases_reset(int UNUSED(region), int UNUSED(faction_id), int UNUSED(defend_only)) {
    not_implemented("mod_bases_reset");
}

__attribute__((weak)) int unit_support_plan() {
    return (conf.modify_unit_support == 1 ? PLAN_SUPPLY :
        (conf.modify_unit_support == 2 ? PLAN_PROBE : PLAN_TERRAFORM));
}

__attribute__((weak)) bool base_can_riot(int base_id, bool allow_staple) {
    BASE* b = &Bases[base_id];
    return (!allow_staple || !b->nerve_staple_turns_left)
        && !has_project(FAC_TELEPATHIC_MATRIX, b->faction_id)
        && !has_fac_built(FAC_PUNISHMENT_SPHERE, base_id);
}

__attribute__((weak)) void __cdecl mon_secrets_of_tech(int UNUSED(faction_id)) {
    not_implemented("mon_secrets_of_tech");
}

__attribute__((weak)) bool full_game_turn() {
    return !(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE);
}

__attribute__((weak)) bool need_police(int faction_id) {
    Faction& f = Factions[faction_id];
    return f.SE_police > -2 && f.SE_police < 3 && !has_project(FAC_TELEPATHIC_MATRIX, faction_id);
}

} // namespace engine
} // namespace alpha
