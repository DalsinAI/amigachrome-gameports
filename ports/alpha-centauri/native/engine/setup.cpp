/* SPDX-License-Identifier: GPL-3.0-or-later
 * New game setup (WP7): faction start positions and supply pods (Thinker Mod
 * src/map.cpp find_reset, valid_start, apply_nutrient_bonus,
 * apply_supply_pods, find_start), faction setup (src/faction.cpp
 * mod_setup_player) and game setup (src/game.cpp init_world_config,
 * setup_game) at dabe62f2d1abba03de40f3dd3048494843f6a9af, ported line for
 * line. Copyright (c) Thinker Mod authors, MIT licence; see
 * engine/THINKER-LICENSE.
 *
 * Differences from Thinker, each marked "Fix:" or "Original:" below:
 * - preferences (prefs_put/prefs_load/prefs_use), undo, hotseat chat, net
 *   and council notification resets are user-interface or network state
 *   that the engine does not have; they are dropped;
 * - DEBUG-only paths (setup_values) are removed;
 * - calls into other packages go through engine_wp.h or wp7_deps.h.
 */
#include "engine.h"
#include "mapgen.h"
#include "setup.h"
#include "wp7_deps.h"

#include <ctype.h>

namespace alpha {
namespace engine {

uint32_t custom_game_rules = 0;
uint32_t custom_more_rules = 0;


static Points spawns;
static Points natives;
static Points goodtiles;

static void find_reset(int faction_id, int k) {
    spawns.clear();
    natives.clear();
    goodtiles.clear();
    /*
    This value defines how many tiles an area needs to have before it's
    considered sufficiently large to be a faction starting location.
    Map area square root values: Tiny = 33, Standard = 56, Huge = 90
    */
    int limit = clamp(*MapAreaTiles, 1024, 8192) / 40;
    size_t land_area = 0;
    MAP* sq;

    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y&1; x < *MapAreaX; x+=2) {
            if (y < k || y >= *MapAreaY - k || !(sq = mapsq(x, y))) {
                continue;
            }
            // LM_FRESH landmark is ignored for this check
            if (!is_ocean(sq) && !sq->is_rocky() && !sq->is_fungus()
            && sq->is_land_region() && !(sq->lm_items() & ~LM_FRESH)
            && Continents[sq->region].tile_count >= limit) {
                goodtiles.insert({x, y});
            }
            if (sq->is_land_region()) {
                land_area++;
            }
        }
    }
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        if (v->faction_id == 0) {
            natives.insert({v->x, v->y});
        }
        if (v->faction_id != 0 && v->faction_id != faction_id) {
            spawns.insert({v->x, v->y});
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id != faction_id) {
            spawns.insert({Bases[i].x, Bases[i].y});
        }
    }
    if (goodtiles.size() * 3 < land_area) {
        goodtiles.clear();
    }
    debug("find_reset x: %d y: %d sqrt: %d tiles: %d good: %d\n",
        *MapAreaX, *MapAreaY, *MapAreaSqRoot, *MapAreaTiles, (int)goodtiles.size()); // Fix: format type
}

static bool valid_start(int faction_id, int iter, int x, int y) {
    MAP* sq = mapsq(x, y);
    bool aquatic = MFactions[faction_id].is_aquatic();
    // Minimum spawn range is reduced for later spawns to account for less free space
    int spawn_limit = max((*BaseCount >= 20 ? 4 : 5 + (*MapAreaTiles >= 1600 ? 2 : 0)), 8 - iter/80);
    int native_limit = (goodtiles.size() > 0 ? 3 : 2) + ((int)natives.size() < *MapAreaTiles/80);

    if (!sq || !sq->allow_spawn()) { // Select only tiles where bases can be built
        return false;
    }
    // LM_FRESH landmark is incorrectly used on some maps
    if (aquatic != is_ocean(sq) || ((sq->lm_items() & ~LM_FRESH) && iter < 160)) {
        return false;
    }
    if ((mod_goody_at(x, y) || mod_bonus_at(x, y)) && iter < 160) {
        return false;
    }
    if (Continents[sq->region].tile_count < *MapAreaTiles / ((aquatic ? 20 : 40) << (iter/40))) {
        return false;
    }
    if (min_range(natives, x, y) < max(native_limit, 8 - iter/16)) {
        return false;
    }
    int spawn_range = min_range(spawns, x, y);
    int min_sc = 80 - iter/4 + 20 * max(0, 12 - spawn_range);
    int max_dst = (*MapAreaTiles >= 6400 ? 7 : 5);
    int dst[4] = {0, 0, 0, 0};
    int sea = 0;
    int sc = 0;
    if (spawn_range < clamp(*MapAreaSqRoot/4 + 8 - iter/8, spawn_limit, 32)) {
        return false;
    }
    for (auto& m : iterate_tiles(x, y, 0, 45)) {
        int bonus = mod_bonus_at(m.x, m.y);
        if (is_ocean(m.sq)) {
            if (!is_ocean_shelf(m.sq)) {
                sc -= 2;
            } else if (aquatic) {
                sc += 4;
            }
            sea++;
        } else {
            sc += (m.sq->is_rainy() ? 3 : (m.sq->is_moist() ? 2 : 1));
            if (m.sq->items & BIT_RIVER) {
                sc += 4;
            }
            if (m.sq->items & BIT_FOREST) {
                sc += 4;
            }
            if (m.sq->is_rolling()) {
                sc += 1;
            }
        }
        if (bonus != RES_NONE) {
            sc += (m.i < 25 ? 10 : 8);
        }
        if (mod_goody_at(m.x, m.y)) {
            sc += 10;
        }
        if (m.sq->items & BIT_FUNGUS) {
            sc -= (m.i < 25 ? 4 : 2) * (is_ocean(m.sq) ? 1 : 2);
        }
        if (m.sq->items & BIT_MONOLITH) {
            sc += 8;
        }
    }
    if (!aquatic) {
        for (int i = 1; i <= max_dst && !is_ocean(wrap(x-i*2), y); ++i) {
            dst[0] = i;
        }
        for (int i = 1; i <= max_dst && !is_ocean(wrap(x+i*2), y); ++i) {
            dst[1] = i;
        }
        for (int i = 1; i <= max_dst && !is_ocean(wrap(x), y-i*2); ++i) {
            dst[2] = i;
        }
        for (int i = 1; i <= max_dst && !is_ocean(wrap(x), y+i*2); ++i) {
            dst[3] = i;
        }
        sc -= 16*clamp(max_dst - std::max({dst[0], dst[1], dst[2], dst[3]}), 0, 3);
        sc -= 2*clamp(sea - 10, 0, 20);
        sc -= (!dst[0] && !dst[1] ? 20 : 0);
        sc -= (!dst[2] && !dst[3] ? 20 : 0);
    }
    debug("find_score %d %3d x: %3d y: %3d dst: %d %d %d %d sea: %2d rng: %2d min: %d score: %d\n",
        faction_id, iter, x, y, dst[0], dst[1], dst[2], dst[3], sea, spawn_range, min_sc, sc);

    if (!aquatic && iter < 200) {
        if (sea > 20 + (iter / 16)) {
            return false;
        }
        if (max(min(dst[0], dst[1]), min(dst[2], dst[3])) < clamp(iter % 8, 1, 4)) {
            return false;
        }
    }
    return sc >= min_sc;
}

static void apply_nutrient_bonus(int faction_id, int* x, int* y) {
    MAP* sq;
    Points addon;
    Points places;
    Points allpods;
    Points rivers;
    bool aquatic = MFactions[faction_id].is_aquatic();
    int adjust = (aquatic ? 0 : 8);
    int nutrient = 0;
    int num = 0;

    // Bonus resources are placed adjacent to each other only on diagonals
    for (auto& m : iterate_tiles(*x, *y, 0, 80)) {
        int bonus = mod_bonus_at(m.x, m.y);
        int goody = mod_goody_at(m.x, m.y);
        if (aquatic == is_ocean(m.sq) && m.sq->alt_level() >= ALT_OCEAN_SHELF
        && (m.i < 25 || (m.i < 45 && nutrient + (int)places.size()/4 < conf.nutrient_bonus))) {
            if (bonus == RES_NUTRIENT) {
                if (nutrient < conf.nutrient_bonus && m.sq->items & BIT_FUNGUS) {
                    m.sq->items &= ~BIT_FUNGUS;
                }
                if (m.sq->is_rocky()) {
                    rocky_set(m.x, m.y, LEVEL_ROLLING);
                }
                synch_bit(m.x, m.y, faction_id);
                nutrient++;
            } else if (!goody && bonus == RES_NONE
            && adjust > 0 && m.sq->items & BIT_RIVER) {
                if (m.i == 0) {
                    adjust = 0;
                }
                if (m.i > 0 && m.i <= adjust
                && m.sq->region == region_at(*x, *y)
                && can_build_base(m.x, m.y, faction_id, TRIAD_LAND)
                && min_range(spawns, m.x, m.y) >= 8) {
                    rivers.insert({m.x, m.y});
                }
            }
        }
        if (bonus || goody) {
            allpods.insert({m.x, m.y});
        }
        else if (m.i < 45 && (int)places.size()/4 < conf.nutrient_bonus
        && aquatic == is_ocean(m.sq) && m.sq->alt_level() >= ALT_OCEAN_SHELF
        && !(m.sq->items & (BIT_BASE_IN_TILE|BIT_VEH_IN_TILE|BIT_MONOLITH))) {
            places.insert({m.x, m.y});
        }
    }
    while (places.size() > 0 && nutrient + (int)addon.size() < conf.nutrient_bonus) {
        auto t = pick_random(places);
        bool found = false;
        for (auto& p : allpods) {
            if (x_dist(p.x, t.x) == 1 && abs(p.y - t.y) == 1) {
                found = true; break;
            }
        }
        if (!found) {
            addon.insert({t.x, t.y});
            allpods.insert({t.x, t.y});
        }
        places.erase(t);
    }
    for (auto& t : addon) {
        sq = mapsq(t.x, t.y);
        sq->items &= ~(BIT_FUNGUS | BIT_MINERAL_RES | BIT_ENERGY_RES);
        sq->items |= (BIT_SUPPLY_REMOVE | BIT_BONUS_RES | BIT_NUTRIENT_RES);
        if (sq->is_rocky()) {
            rocky_set(t.x, t.y, LEVEL_ROLLING);
        }
        synch_bit(t.x, t.y, faction_id);
        num++;
    }
    // Adjust position to adjacent river if currently not on river
    if (adjust > 0 && rivers.size() > 0) {
        auto t = pick_random(rivers);
        *x = t.x;
        *y = t.y;
    }
}

static void apply_supply_pods(int faction_id, int x, int y) {
    MAP* sq;
    Points places;
    int bonus = 0;
    int pods = 0;
    for (auto& m : iterate_tiles(x, y, 0, 25)) {
        if (conf.rare_supply_pods > 1) {
            m.sq->items |= BIT_SUPPLY_REMOVE;
            synch_bit(m.x, m.y, faction_id);
        } else if (mod_goody_at(m.x, m.y)) {
            pods++;
        } else if (mod_bonus_at(m.x, m.y)) {
            bonus++;
        } else if (m.i > 0 && m.sq->allow_supply()) {
            places.insert({m.x, m.y});
        }
    }
    if (conf.rare_supply_pods <= 1) {
        int adjust = clamp(random(10) - bonus, 2, 4) - pods;
        while (adjust > 0 && places.size() > 0) {
            auto t = pick_random(places);
            sq = mapsq(t.x, t.y);
            sq->items |= BIT_UNK_8000000;
            synch_bit(t.x, t.y, faction_id);
            adjust--;
            places.erase(t);
        }
    }
}

void __cdecl find_start(int faction_id, int* tx, int* ty) {
    bool aquatic = MFactions[faction_id].is_aquatic();
    int x = 0;
    int y = 0;
    int i = 0;
    int k = (*MapAreaY < 80 ? 4 : 8);
    find_reset(faction_id, k/2);

    while (++i <= 1000) {
        if (!aquatic && goodtiles.size() > 0 && i <= 200) {
            auto t = pick_random(goodtiles);
            y = t.y;
            x = t.x;
        } else {
            y = (random(*MapAreaY - k*2) + k);
            x = (random(*MapAreaX) & (~1)) + (y&1);
        }
        if (valid_start(faction_id, i, x, y)) {
            if (conf.nutrient_bonus > 0) {
                apply_nutrient_bonus(faction_id, &x, &y);
            }
            // No unity scattering can normally spawn pods at two tile range from the start
            if (*GameRules & RULES_NO_UNITY_SCATTERING) {
                apply_supply_pods(faction_id, x, y);
            }
            *tx = x;
            *ty = y;
            site_set(x, y, world_site(x, y, 0));
            break;
        }
    }
    debug("find_start %d %3d x: %3d y: %3d %s\n",
        faction_id, i, *tx, *ty, MFactions[faction_id].filename);
    flushlog();
}

static int veh_init_last(int unit_id, int faction_id, int x, int y) {
    int veh_id = veh_init_free(unit_id, faction_id, x, y);
    if (veh_id >= 0) {
        veh_demote(veh_id);
    }
    return veh_id;
}

static void veh_init_stack(int unit_id, int faction_id, int x, int y, int num) {
    for (int i = 0; i < num; i++) {
        veh_init_last(unit_id, faction_id, x, y);
    }
}

static bool has_suffix(const char* str) {
    int len = strlen(str);
    return len >= 2 && str[len-2] == '-' && isdigit(str[len-1]);
}

int __cdecl mod_setup_player(int faction_id, int setup_id, int is_probe) {
    Faction* plr = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    const bool special_spawn = setup_id == -282;
    const bool initial_spawn = !*CurrentTurn || special_spawn;
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool plr_alien = m->is_alien();
    const bool plr_aquatic = m->is_aquatic();
    debug("setup_player %d %d %d %d %s\n", *CurrentTurn, faction_id, setup_id, is_probe, m->filename);

    // Original: DEBUG builds with faction_placement = 0 call setup_values
    // (the executable's setup_player); release builds never do.
    // Modify the original version to check by filename if multiple similar factions are active.
    // If any duplicates are found, each one will have their faction_id as name suffix.
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id && initial_spawn && i != faction_id
        && !_stricmp(MFactions[i].filename, m->filename)) {
            char buf[StrBufLen];
            if (!has_suffix(m->name_leader)) {
                strcpy_n(buf, 22, m->name_leader);
                snprintf(m->name_leader, 24, "%.21s-%d", buf, faction_id); // Fix: precision states the strcpy_n bound
            }
            if (!has_suffix(m->noun_faction)) {
                strcpy_n(buf, 22, m->noun_faction);
                snprintf(m->noun_faction, 24, "%.21s-%d", buf, faction_id); // Fix: precision states the strcpy_n bound
            }
            if (!has_suffix(m->formal_name_faction)) {
                strcpy_n(buf, 38, m->formal_name_faction);
                snprintf(m->formal_name_faction, 40, "%.37s-%d", buf, faction_id); // Fix: precision states the strcpy_n bound
            }
            break;
        }
    }
    for (int i = *BaseCount - 1; i >= 0; --i) {
        if (Bases[i].faction_id == faction_id) {
            mod_base_kill(i);
        }
    }
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && (faction_id || veh->unit_id != BSC_FUNGAL_TOWER)) {
            kill(i);
        }
    }
    plr->target_y = -1;
    plr->target_x = -1;
    plr->base_id_attack_target = -1;
    if (initial_spawn) {
        clear_goals(faction_id);
        clear_bunglist(faction_id);
        plr->player_flags = 0;
        plr->base_governor_adv = (GOV_MAY_PROD_TRANSPORT|GOV_MAY_PROD_EXPLORE_VEH|GOV_MAY_PROD_PROBES|\
            GOV_MAY_PROD_PROTOTYPE|GOV_MAY_PROD_SP|GOV_MAY_PROD_COLONY_POD|GOV_MAY_PROD_FACILITIES|\
            GOV_MAY_PROD_TERRAFORMERS|GOV_MAY_PROD_AIR_DEFENSE|GOV_MAY_PROD_LAND_DEFENSE|GOV_MAY_PROD_AIR_COMBAT|\
            GOV_MAY_PROD_NAVAL_COMBAT|GOV_MAY_PROD_LAND_COMBAT|GOV_MANAGE_CITIZENS|GOV_MANAGE_PRODUCTION);
        if (!*DiffLevel) {
            plr->base_governor_adv |= (GOV_ACTIVE|GOV_UNK_40000000);
        }
        plr->base_name_offset = 0;
        plr->base_sea_name_offset = 0;
        plr->AI_fight = m->AI_fight;
        plr->AI_power = m->AI_power;
        plr->AI_tech = m->AI_tech;
        plr->AI_wealth = m->AI_wealth;
        plr->AI_growth = m->AI_growth;
        plr->unk_37 = 0;
        for (int i = 0; i < 8; i++) {
            plr->saved_queue_size[i] = 0;
            snprintf(&plr->saved_queue_name[i][0], 24, "%s %d", label_get(TL_Template), i+1);
        }
        memset(&plr->SE_Politics_pending, 0, 32u); // pending / current
        memset(&plr->SE_economy_pending, 0, 44u); // only pending
        memset(&plr->secret_project_intel, 0, 32u);
        plr->SE_upheaval_cost_paid = 0;
        plr->diff_level = is_human(faction_id) ? *DiffLevel : 3;
        plr->integrity_blemishes = 0;
        plr->diplo_reputation = 0;
        plr->traded_maps = 0;
        plr->energy_credits = 0;
        plr->atrocities = 0;
        plr->major_atrocities = 0;
        plr->clean_minerals_modifier = 0;
        plr->sanction_turns = 0;
        plr->council_call_turn = -999;
        TectonicDetonationCount[faction_id] = 0;
        // Fix: randomized leader personalities always selects at least one AI priority.
        if (*GameState & STATE_RAND_FAC_LEADER_PERSONALITIES) {
            plr->AI_fight = game_randv(3) - 1;
            int val = 0;
            while (!val) {
                val = game_randv(4096);
                val &= (val >> 8) & (val >> 4);
                if (val & 1) plr->AI_growth = 1;
                if (val & 2) plr->AI_tech = 1;
                if (val & 4) plr->AI_wealth = 1;
                if (val & 8) plr->AI_power = 1;
            }
        }
        if (*GameRules & RULES_INTENSE_RIVALRY) {
            plr->AI_fight = 1;
        }
        plr->total_combat_units = 0;
        plr->base_count = 0;
        plr->mil_strength_1 = 0;
        plr->mil_strength_2 = 0;
        plr->pop_total = 0;
        plr->unk_70 = 0;
        plr->mind_control_total = 0;
        memset(&plr->diplo_mind_control, 0, 32u);
        memset(&plr->diplo_stolen_techs, 0, 32u);
        plr->net_random_event = 0;
        for (int i = 0; i < MaxRegionNum; i++) {
            plr->region_total_combat_units[i] = 0;
            plr->region_total_bases[i] = 0;
            plr->region_total_offensive_units[i] = 0;
            plr->region_force_rating[i] = 0;
            plr->region_flags[i] = 0;
        }
        memset(&plr->unk_29, 0, 44u);
        memset(&plr->unk_30, 0, 44u);
        memset(&plr->corner_market_turn, 0, 40u);
        plr->unk_93 = 0;
        plr->goody_opened = 0;
        plr->goody_artifact = 0;
        plr->goody_earthquake = 0;
        plr->goody_tech = 0;
        plr->tech_achieved = 0;
        plr->time_bonus_count = 1;
    }
    plr->energy_credits += (*MultiplayerActive ? 100 : Rules->starting_energy_reserve) + m->rule_energy;
    plr->hurry_cost_total = 0;
    plr->tech_cost = -1;
    plr->last_base_turn = *CurrentTurn;
    for (int i = 0; i < MaxPlayerNum; i++) {
        if (!initial_spawn) {
            treaty_off(faction_id, i,
                DIPLO_UNK_80000000|DIPLO_UNK_10000000|DIPLO_UNK_8000|DIPLO_UNK_4000|DIPLO_WANT_TO_TALK|\
                DIPLO_HAVE_INFILTRATOR|DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_UNK_200|DIPLO_UNK_100|DIPLO_UNK_40|DIPLO_COMMLINK);
        } else {
            plr->diplo_status[i] = 0; // seems not to reset diplo_agenda?
            plr->diplo_spoke[i] = -1;
            plr->diplo_merc[i] = 0;
            plr->diplo_patience[i] = 0;
            int val = 3 * (*GameRules & RULES_INTENSE_RIVALRY ? 5 : *DiffLevel) + 8;
            plr->diplo_friction[i] = clamp(game_randv(val), 1, 16);
            // Fix: the game may not properly reset all these fields.
            plr->diplo_gifts[i] = 0;
            plr->diplo_wrongs[i] = 0;
            plr->diplo_betrayed[i] = 0;
            plr->diplo_combat_total[i] = 0;
            plr->diplo_combat_count[i] = 0;
        }
        plr->loan_balance[i] = 0;
        plr->loan_payment[i] = 0;
        plr->unk_1[i] = 0;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        plr->units_active[i] = 0;
        plr->units_queue[i] = 0;
        if (!*CurrentTurn) {
            plr->units_lost[i] = 0;
        }
    }
    if (initial_spawn) {
        memset(&plr->tech_pact_shared_goals, 0, 48u);
        memset(&plr->tech_trade_source, 0, 92u);
        memset(&plr->facility_announced, 0, 12u);
        plr->unk_27 = 0;
        plr->satellites_nutrient = 0;
        plr->satellites_mineral = 0;
        plr->satellites_energy = 0;
        plr->satellites_ODP = 0;
        plr->ODP_deployed = 0;
        plr->tech_count_transcendent = 0;
        plr->tech_ranking = 0;
        plr->tech_research_id = -1;
        plr->tech_accumulated = 0;
        plr->earned_techs_saved = 0;
        plr->unk_102 = 0;
        if (faction_id) {
            if (is_human(faction_id)) {
                plr->satellites_nutrient = conf.player_satellites[0];
                plr->satellites_mineral = conf.player_satellites[1];
                plr->satellites_energy = conf.player_satellites[2];
            } else {
                plr->satellites_nutrient = conf.computer_satellites[0];
                plr->satellites_mineral = conf.computer_satellites[1];
                plr->satellites_energy = conf.computer_satellites[2];
            }
            // Update default governor settings
            plr->base_governor_adv |= GOV_MAY_PROD_NATIVE;
        }
    }
    plr->unk_17 = 0;
    plr->unk_18 = 0;
    plr->best_mineral_output = 0;
    plr->energy_surplus_total = 0;
    plr->facility_maint_total = 0;
    plr->tech_commerce_bonus = 0;
    plr->tech_fungus_nutrient = 0;
    plr->tech_fungus_mineral = 0;
    plr->tech_fungus_energy = 0;
    plr->tech_fungus_unused = 0;
    plr->turn_commerce_income = 0;
    plr->best_weapon_value = 0;
    plr->best_armor_value = 0;
    plr->best_land_speed = 0;
    plr->best_psi_land_offense = 0;
    plr->best_psi_land_defense = 0;
    plr->enemy_best_weapon_value = 0;
    plr->enemy_best_armor_value = 0;
    plr->enemy_best_land_speed = 0;
    plr->enemy_best_psi_land_offense = 0;
    plr->enemy_best_psi_land_defense = 0;
    if (!faction_id) {
        return 0;
    }
    if (!is_alive(faction_id)) {
        return 0;
    }
    if (setup_id < -1 && !special_spawn) {
        set_alive(faction_id, false);
        return 0;
    }
    bool spawn_skip = *CurrentTurn > 100 || (!initial_spawn && *GameRules & RULES_DO_OR_DIE);
    bool spawn_flag = !is_probe && (plr->player_flags & PFLAG_UNK_1000);
    MAP* sq = NULL;
    int region = 0;
    int x = -1;
    int y = -1;
    // This replaces the entire old faction placement method
    if (is_probe || !spawn_flag) {
        find_start(faction_id, &x, &y);
        if (!(sq = mapsq(x, y))) {
            // skipped
        } else {
            region = sq->region;
        }
    }
    if (!is_probe && (spawn_flag || spawn_skip)) {
        if (!is_human(faction_id) || *MultiplayerActive) {
            plr->unk_102 = 0;
            set_alive(faction_id, false);
            int other_id = FactionRankings[7];
            if (faction_id == other_id) {
                other_id = FactionRankings[6];
            }
            int num_active = 0;
            int num_allied = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i)) {
                    num_active++;
                    if (!*MultiplayerActive && Factions[i].diplo_status[*CurrentPlayerFaction] & DIPLO_PACT
                    && Factions[i].diplo_status[*CurrentPlayerFaction] & DIPLO_HAVE_SURRENDERED) {
                        num_active--;
                    } else if (i != other_id && Factions[i].diplo_status[other_id] & DIPLO_PACT
                    && *GameRules & RULES_VICTORY_COOPERATIVE && num_allied < 2) {
                        num_allied++;
                    }
                }
            }
            if (is_player) {
                int k = (setup_id < 0 ? 0 : setup_id);
                parse_says(0, get_noun(k), -1, -1);
                // Original: net_game_close() and ControlTurnA = 1 when
                // MultiplayerActive; the engine has no network game.
                if (setup_id || *GameLanguage) {
                    X_pop("YOULOSE", 0);
                } else {
                    X_pop("YOULOSE2", 0);
                }
                *GameState |= STATE_GAME_DONE;
                *GameVictoryType = setup_id ? VIC_LOST_CAPTURE : VIC_LOST_REMOVE;
            }
            if (!is_player) {
                if (num_active - num_allied < 2) {
                    if (num_allied) {
                        popp(ScriptFile, "CONQCOOP", 0, "conq_sm.pcx", 0);
                        if (*GameVictoryType == VIC_NONE) {
                            *GameVictoryType = VIC_UNIFY_COOP;
                        }
                    } else if (other_id == *CurrentPlayerFaction) {
                        popp(ScriptFile, "CONQSING", 0, "conq_sm.pcx", 0);
                        if (*GameVictoryType == VIC_NONE) {
                            *GameVictoryType = VIC_UNIFY_SOLO;
                        }
                    }
                    *GameState |= (STATE_GAME_DONE | STATE_VICTORY_CONQUER);
                }
            }
        } else {
            *GameVictoryType = VIC_LOST_CAPTURE;
            *GameState |= STATE_GAME_DONE;
            set_alive(faction_id, false);
        }
        return 0; // Spawn failed, always return zero
    } else {
        if (!sq) {
            return 0; // No suitable spawn location found
        }
        assert(sq && sq->anything_at() < 0);
        assert(region > 0);
        plr->SE_alloc_labs = 5;
        plr->SE_alloc_psych = 0;
        if (initial_spawn && *DiffLevel >= 2) {
            if (plr->AI_wealth || plr->AI_tech) {
                if (!game_randv(4)) {
                    plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
                }
            }
            if (!_stricmp(m->filename, "ANGELS")) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
            }
            if (plr_aquatic) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_SEA_BASES;
                plr->player_flags |= PFLAG_EMPHASIZE_SEA_POWER;
            }
            if (plr_alien) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_MISSILES;
                plr->player_flags |= PFLAG_EMPHASIZE_AIR_POWER;
            }
            if (!_stricmp(m->filename, "DRONE")) {
                plr->player_flags_ext &= ~PFLAG_EXT_STRAT_LOTS_PROBE_TEAMS;
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_TERRAFORMERS;
            }
            if (Continents[region].tile_count < 250 && !game_randv(3)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_SEA_BASES;
            }
            if (plr->AI_fight >= 0 && !game_randv(4)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_MISSILES;
            }
            if (!game_randv(6)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_TERRAFORMERS;
            }
            if (!game_randv(6)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_COLONY_PODS;
            }
            if (!game_randv(5)) {
                plr->player_flags_ext |= PFLAG_EXT_STRAT_LOTS_ARTILLERY;
            }
            if ((*GameRules & RULES_INTENSE_RIVALRY || *DiffLevel >= 4)
            && plr->AI_fight > -((*GameRules & RULES_INTENSE_RIVALRY) != 0)) {
                if (!game_randv(4)) {
                    plr->player_flags_ext |= PFLAG_EXT_SHAMELESS_BETRAY_HUMANS;
                }
                if (!game_randv(4)) {
                    plr->player_flags |= PFLAG_COMMIT_ATROCITIES_WANTONLY;
                    if (!game_randv(2)) {
                        plr->player_flags |= PFLAG_OBLITERATE_CAPTURED_BASES;
                    }
                }
            }
            if (plr->AI_power || plr->AI_fight > 0) {
                if (Continents[region].tile_count >= 400 && !game_randv(4)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_LAND_POWER;
                }
                int val = Continents[region].tile_count > 250 ? 6 : 4;
                if (!game_randv(val)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_SEA_POWER;
                }
                if (!game_randv(4)) {
                    plr->player_flags |= PFLAG_EMPHASIZE_AIR_POWER;
                }
            }
        }
        for (auto& p : iterate_tiles(x, y, 0, 21)) {
            p.sq->visibility |= (1 << faction_id);
            synch_bit(p.x, p.y, faction_id);
        }
        int recon_unit_id = BSC_SCOUT_PATROL;
        if (initial_spawn) {
            int bonus_count = m->faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (m->faction_bonus_id[i] == RULE_TECH) {
                    tech_achieved(faction_id, m->faction_bonus_val1[i], 0, 0);
                }
            }
            if (*ExpansionEnabled && plr_alien && conf.alien_guaranteed_techs) {
                tech_achieved(faction_id, TECH_PrPsych, 0, 0);
                tech_achieved(faction_id, TECH_FldMod, 0, 0);
            }
            if (*MultiplayerActive) {
                for (int i = 0; ; ++i) {
                    int v1 = plr->SE_research_base <= 0 ? (plr->SE_research_base >= 0) - 1 : 1;
                    int v2 = plr->SE_morale_base <= 0 ? (plr->SE_morale_base >= 0) - 1 : 1;
                    if (i >= v1 - v2 + 1) {
                        break;
                    }
                    tech_achieved(faction_id, mod_tech_ai(faction_id), 0, 0);
                }
                plr->energy_credits += m->rule_energy / 2;
            }
            consider_designs(faction_id);
            for (int i = 0; i < MaxProtoFactionNum; i++) {
                int unit_id = i + faction_id * MaxProtoFactionNum;
                UNIT* u = &Units[unit_id];
                if (u->unit_flags & UNIT_ACTIVE) {
                    u->unit_flags |= UNIT_PROTOTYPED;
                    if (u->plan == PLAN_RECON) {
                        recon_unit_id = unit_id;
                    }
                }
            }
        }
        // First spawn additional units specified by config
        // These prototypes also use the same fixed choices as in the original game
        const int num_colony = is_human(faction_id) ? conf.player_colony_pods : conf.computer_colony_pods;
        const int num_former = is_human(faction_id) ? conf.player_formers : conf.computer_formers;
        const int colony_unit = plr_aquatic ? BSC_SEA_ESCAPE_POD : BSC_COLONY_POD;
        const int former_unit = plr_aquatic ? BSC_SEA_FORMERS : BSC_FORMERS;
        veh_init_stack(colony_unit, faction_id, x, y, num_colony);
        veh_init_stack(former_unit, faction_id, x, y, num_former);
        // For consistency these colony/former are always set as independent units
        // Colony pods spawned may change based on balance function (deprecated, skipped by default)
        if (!initial_spawn) {
            veh_init_free(colony_unit, faction_id, x, y);
            veh_init_last(former_unit, faction_id, x, y);
        } else {
            // Fix: original game always enabled Look First during MultiplayerActive if Time Warp is disabled
            if (*GameRules & RULES_LOOK_FIRST && !(*GameRules & RULES_TIME_WARP)) {
                veh_init_free(colony_unit, faction_id, x, y);
            } else {
                int base_id = mod_base_init(faction_id, x, y);
                if (base_id >= 0 && !_stricmp(m->filename, "FUNGBOY") && special_spawn) {
                    Bases[base_id].pop_size = 3;
                }
                if (base_id >= 0 && plr_alien && special_spawn) {
                    Bases[base_id].pop_size = 3;
                }
            }
            if (!plr_aquatic) {
                // Original game only spawns an extra recon unit
                veh_init_stack(BSC_COLONY_POD, faction_id, x, y, conf.spawn_free_units[0]);
                veh_init_stack(BSC_FORMERS, faction_id, x, y, conf.spawn_free_units[1]);
                veh_init_stack(recon_unit_id, faction_id, x, y, conf.spawn_free_units[2]);
            }
            if (*MultiplayerActive && !plr_aquatic) {
                veh_init_last(BSC_UNITY_ROVER, faction_id, x, y);
                veh_init_last(BSC_FORMERS, faction_id, x, y);
            }
            int coast = is_coast(x, y, 0);
            if (coast && (*DiffLevel > 1 || *MultiplayerActive || Continents[region].tile_count <= 32)) {
                // Calculation for nearby opponents is slightly modified from the original to exclude native units
                uint32_t nearby = 0;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (Factions[i].region_total_bases[region]) {
                        nearby |= (1 << Bases[i].faction_id);
                    }
                }
                for (int i = 0; i < *VehCount; i++) {
                    if (Vehs[i].faction_id && region_at(Vehs[i].x, Vehs[i].y) == region) {
                        nearby |= (1 << Vehs[i].faction_id);
                    }
                }
                int tile_ratio = Continents[region].tile_count / (bit_count(nearby) + 1);
                int num = (tile_ratio <= 50) + (*MultiplayerActive && is_human(faction_id) && tile_ratio <= 75);
                int x2 = wrap(x + TableOffsetX[coast]);
                int y2 = y + TableOffsetY[coast];
                while (--num >= 0) {
                    int veh_id = veh_init(BSC_UNITY_FOIL, faction_id, x2, y2);
                    if (veh_id >= 0) {
                        Vehs[veh_id].home_base_id = -1;
                        Vehs[veh_id].morale = 2;
                        veh_demote(veh_id);
                    }
                }
            }
            memset(&plr->SE_economy_base, 0, 44u);
            int bonus_count = m->faction_bonus_count;
            for (int i = 0; i < bonus_count; i++) {
                if (m->faction_bonus_id[i] == RULE_SOCIAL) {
                    assert(m->faction_bonus_val1[i] >= 0 && m->faction_bonus_val1[i] < 11);
                    auto effects = (CSocialEffect*)&plr->SE_economy_base;
                    effects->values[m->faction_bonus_val1[i]] += m->faction_bonus_val2[i];
                } else if (m->faction_bonus_id[i] == RULE_UNIT) {
                    veh_init_last(m->faction_bonus_val1[i], faction_id, x, y);
                    if (m->faction_bonus_val1[i] == BSC_COLONY_POD) {
                        plr->player_flags |= PFLAG_UNK_100;
                    }
                }
            }
            if (*ExpansionEnabled && plr_alien) {
                // Original game only spawns an extra colony pod and battle ogre
                veh_init_stack(BSC_COLONY_POD, faction_id, x, y, conf.spawn_free_units[6]);
                veh_init_stack(BSC_FORMERS, faction_id, x, y, conf.spawn_free_units[7]);
                veh_init_stack(BSC_BATTLE_OGRE_MK1, faction_id, x, y, conf.spawn_free_units[8]);
            }
            if (plr_aquatic) {
                if (!is_human(faction_id)) {
                    int unit_id = -1; // Fix: possibly uninitialized default value
                    for (int i = 0; i < MaxProtoFactionNum; i++) {
                        int k = i + faction_id * MaxProtoFactionNum;
                        if (!(Units[k].unit_flags & UNIT_ACTIVE)) {
                            unit_id = k;
                            break;
                        }
                    }
                    if (unit_id >= 0) {
                        char buf[StrBufLen] = {};
                        make_proto(unit_id, CHS_FOIL, WPN_COLONY_MODULE, ARM_NO_ARMOR, ABL_NONE, REC_FISSION);
                        name_proto(&buf[0], unit_id, faction_id,
                            CHS_FOIL, WPN_COLONY_MODULE, ARM_NO_ARMOR, ABL_NONE, REC_FISSION);
                        Units[unit_id].unit_flags |= UNIT_PROTOTYPED;
                        Units[unit_id].unit_flags &= ~UNIT_CUSTOM_NAME_SET;
                        strcpy_n(&Units[unit_id].name[0], MaxProtoNameLen, buf);
                    }
                }
                // Original game only spawns an extra colony pod and unity gunship
                veh_init_stack(BSC_SEA_ESCAPE_POD, faction_id, x, y, conf.spawn_free_units[3]);
                veh_init_stack(BSC_SEA_FORMERS, faction_id, x, y, conf.spawn_free_units[4]);
                veh_init_stack(BSC_UNITY_GUNSHIP, faction_id, x, y, conf.spawn_free_units[5]);
            }
        }
        if (!spawn_flag && !initial_spawn) {
            int spawn_id_a = BSC_SCOUT_PATROL; // Garrison unit
            int spawn_id_b = -1; // Mostly attacker unit
            int spawn_id_c = -1; // Fast recon unit
            int spawn_val_a = -1;
            int spawn_val_b = -1;
            int spawn_val_c = -1;
            for (int unit_id = 0; unit_id < MaxProtoNum; unit_id++) {
                UNIT* u = &Units[unit_id];
                if (veh_avail(unit_id, faction_id, -1) && u->offense_value() != 0) {
                    if ((plr_aquatic && u->triad() == TRIAD_SEA) || (!plr_aquatic && u->triad() == TRIAD_LAND)) {
                        int v1 = 8 * armor_val(unit_id, faction_id) - weap_val(unit_id, faction_id);
                        if (v1 > spawn_val_a) {
                            spawn_val_a = v1;
                            spawn_id_a = unit_id;
                        }
                        int v2 = u->speed() + 2 * armor_val(unit_id, faction_id) + 4 * weap_val(unit_id, faction_id);
                        if (v2 > spawn_val_b) {
                            spawn_val_b = v2;
                            spawn_id_b = unit_id;
                        }
                        int v3 = 4 * u->speed() - weap_val(unit_id, faction_id) - armor_val(unit_id, faction_id);
                        if (v3 > spawn_val_c && u->speed() > 1) {
                            spawn_val_c = v3;
                            spawn_id_c = unit_id;
                        }
                    }
                }
            }
            int veh_id = veh_init(colony_unit, faction_id, x, y);
            veh_init(spawn_id_a, faction_id, x, y);
            if (*CurrentTurn > 20 && spawn_id_c >= 0) {
                veh_init(spawn_id_c, faction_id, x, y);
            }
            if (*CurrentTurn > 40 && spawn_id_b >= 0) {
                veh_init(spawn_id_b, faction_id, x, y);
            }
            if (*CurrentTurn > 60 && spawn_id_a >= 0) {
                veh_init(spawn_id_a, faction_id, x, y);
            }
            if (*CurrentTurn > 80 && spawn_id_b >= 0) {
                veh_init(spawn_id_b, faction_id, x, y);
            }
            if (*CurrentTurn > 100) {
                veh_init(colony_unit, faction_id, x, y);
            }
            veh_promote(veh_id);
            plr->unk_27 = 3 * plr->tech_ranking / 4;
        }
        if (faction_id == *CurrentPlayerFaction) {
            MapWin->TileX = x;
            MapWin->TileY = y;
            for (int i = 0; i < 32; i++) {
                MapWin->CursorListX[i] = x;
                MapWin->CursorListY[i] = y;
            }
        }
        return 1; // Valid spawn, return value non-zero
    }
}

void init_world_config() {
    ThinkerData tv = thinker_vars_get(); // Original: ThinkerVars->game_time_spent = 0
    tv.game_time_spent = 0;
    thinker_vars_set(tv);
    /*
    Adjust Special Scenario Rules if any are selected from the mod menu.
    This also overrides settings for any scenarions unless all custom options are left empty.
    */
    if ((custom_game_rules || custom_more_rules) && !*MultiplayerActive) {
        *GameRules = (*GameRules & GAME_RULES_MASK) | custom_game_rules;
        *GameMoreRules = (*GameMoreRules & GAME_MRULES_MASK) | custom_more_rules;
        if (*GameRules & RULES_SCN_NO_NATIVE_LIFE) {
            for (int i = *VehCount - 1; i >= 0; --i) {
                if (!Vehs[i].faction_id && Vehs[i].unit_id == BSC_FUNGAL_TOWER) {
                    kill(i);
                }
            }
        }
    }
}

void __cdecl setup_game(int flag) {
    debug("setup_game %d\n", flag);
    LastSavePath[0] = 0;
    // Original: prefs_put("Latest Save"/"Latest Scenario"), the one-time
    // beginner preference reset (prefs_load/prefs_put_2) and prefs_use()
    // write Alpha Centauri.ini; the caller sets GamePreferences directly.
    *ObjectiveAchievePts = 0;
    *ObjectiveReqVictory = 9999;
    *ObjectivesSuddenDeathVictory = 9999;
    *VictoryAchieveBonusPts = 0;
    *StartingMissionYear = Rules->normal_start_year;

    if (*DiffLevel >= 3) {
        *EndingMissionYear = Rules->normal_end_year_high_three_diff;
    } else {
        *EndingMissionYear = Rules->normal_end_year_low_three_diff;
    }
    clear_monuments();
    // Original: wipe_undo() (map editor undo buffer, UI only).
    *ReplayEventSize = 0;
    *GameInterludeState = 0;
    *GameState &= (STATE_RAND_FAC_LEADER_SOCIAL_AGENDA | STATE_RAND_FAC_LEADER_PERSONALITIES);
    cs->dword_9A64AC = 0;
    cs->dword_9A64B0 = 0;
    cs->TutWinMapState[0] = 0;
    cs->TutWinMapState[1] = 0;
    cs->dword_9A64BC = 0;
    *CurrentTurn = 0;
    FactionStatus[1] = 0xFF;
    *BaseCount = 0;
    cs->dword_9A64D0 = 0;
    // Original: clear_all_offers, clear_hotseat_chat, clear_council_notify_2
    // and clear_tamper reset dialog, chat and network state the engine does
    // not keep.
    cs->GameTimeControl[1] = 0;
    *GovernorFaction = -1;
    for (int i = 0; i < MaxProposalNum; ++i) {
        ProposalPassCount[i] = 0;
        ProposalCallTurn[i] = -9999;
    }
    *ClimateValueA = 0;
    *ClimateValueB = 0;
    *ClimateValueC = 0;
    *ClimateFutureChange = 0;

    if (!*MultiplayerActive && flag) { // Original: && !*ControlTurnB (reload request)
        *GameState |= (STATE_OMNISCIENT_VIEW|STATE_SCENARIO_EDITOR|STATE_SCENARIO_CHEATED_FLAG);
    }
    memset(TechOwners, 0, 0x59u);
    memset(SecretProjects, 0xFF, 0x100u);
    memset(ReplayEvents, 0, 0x2000u);
    memset(FactionTurnMight, 0, 0x3E80u);
    // Original: memset(SunspotDuration, 0, 0x28u) clears the ten words from
    // SunspotDuration to ClimateLevel, which GamePrefs keeps in that order.
    memset(&cs->SunspotDuration, 0, 10 * sizeof(int32_t));
    static_assert(offsetof(GamePrefs, ClimateLevel) - offsetof(GamePrefs, SunspotDuration)
        == 9 * sizeof(int32_t), "GamePrefs order");
    // Fix: that also clears SaveFileVersion, which the original save code sets
    // to 12 when writing; set it here so a new game saves in the current version.
    upgrade_save_version();
    memset(TectonicDetonationCount, 0, 0x20u);
    // Original: memset(DiploStateB, 0, 0x100u) (diplomacy dialog state, UI only).
    /*
    Fix issue with randomized faction agendas where they might be given agendas that are
    their opposition social models making the choice unusable. Future social models can be
    also selected but much less often. The original version never selected future social models
    but these are used by several default expansion factions. As for soc_priority_effect,
    the game picks the first social effect that is more than zero in the default social model.
    */
    if (*GameState & STATE_RAND_FAC_LEADER_SOCIAL_AGENDA) {
        for (int fc = 1; fc < MaxPlayerNum; ++fc) {
            MFaction* fac = &MFactions[fc];
            fac->soc_priority_category = -1;
            fac->soc_priority_model = 0;
            fac->soc_priority_effect = -1; // Fix: always initialize
            fac->soc_opposition_category = -1;
            fac->soc_opposition_model = 0; // Fix: always initialize
            fac->soc_opposition_effect = -1; // Fix: always initialize
            for (int iter = 0; iter < 1000; ++iter) {
                int val = game_randv(4) ? 3 : 4;
                int sfield = game_randv(val);
                int smodel = game_randv(3) + 1;
                if (SocialField[sfield].soc_preq_tech[smodel] == TECH_Disable
                || (sfield == fac->soc_opposition_category && smodel == fac->soc_opposition_model)) {
                    continue;
                }
                bool valid = true;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (fc != i && is_alive(i)
                    && MFactions[i].soc_priority_category == sfield
                    && MFactions[i].soc_priority_model == smodel) {
                        valid = false;
                    }
                }
                if (valid) {
                    fac->soc_priority_category = sfield;
                    fac->soc_priority_model = smodel;
                    debug("setup_game agenda %s %d %d\n",  fac->filename, sfield, smodel);
                    for (int i = 0; i < MaxSocialEffectNum; ++i) {
                        if (SocialField[sfield].soc_effect[smodel].values[i] > 0) {
                            fac->soc_priority_effect = i;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    for (int i = 0; i < MaxPlayerNum; ++i) {
        if (is_human(i)) {
            mod_setup_player(i, -1 - flag, 0);
        }
    }
    for (int i = 0; i < MaxPlayerNum; ++i) {
        if (!is_human(i)) {
            mod_setup_player(i, -1 - flag, 0);
        }
    }
    strcpy_n(MFactions[0].formal_name_faction, sizeof(MFaction::formal_name_faction), label_get(TL_Aliens));
    strcpy_n(MFactions[0].noun_faction, sizeof(MFaction::noun_faction), label_get(TL_Aliens));
    strcpy_n(MFactions[0].adj_name_faction, sizeof(MFaction::adj_name_faction), label_get(TL_Alien));

    /*
    COMMFREQ = Gets an extra comm frequency (another faction to talk to) at beginning of game.
    Fix: the ability is applied at the start of the game and commlinks are picked
    with equal probability among all possible players.
    */
    if (!flag) {
        for (int a = 1; a < MaxPlayerNum; ++a) {
            if (MFactions[a].rule_flags & RFLAG_COMMFREQ) {
                int plrs[MaxPlayerNum] = {};
                int num = 0;
                for (int b = 1; b < MaxPlayerNum; ++b) {
                    if (a != b && is_alive(b) && !has_treaty(a, b, DIPLO_COMMLINK)) {
                        plrs[num] = b;
                        ++num;
                    }
                }
                if (num > 0) {
                    set_treaty(a, plrs[game_randv(num)], DIPLO_COMMLINK, 1);
                }
            }
        }
        if (!*MultiplayerActive || (*GameRules & RULES_INTENSE_RIVALRY)) {
            if (*GameState & STATE_RAND_FAC_LEADER_PERSONALITIES) {
                for (int a = 1; a < MaxPlayerNum; ++a) {
                    if (is_human(a)) {
                        int diff_val = (Factions[a].diff_level >= 3)
                            + (*GameRules & RULES_INTENSE_RIVALRY ? 1 : 0);
                        for (int i = 0; i < diff_val; ++i) {
                            for (int iter = 0; iter < 100; ++iter) {
                                int b = game_rand() % (MaxPlayerNum-1) + 1;
                                if (!(Factions[b].diplo_agenda[a] & AGENDA_UNK_200)
                                && is_alive(b) && !is_human(b)) {
                                    agenda_on(b, a, AGENDA_UNK_200);
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                for (int a = 1; a < MaxPlayerNum; ++a) {
                    for (int b = 1; b < MaxPlayerNum; ++b) {
                        if (a == b) {
                            continue;
                        }
                        const char* key1 = MFactions[a].search_key;
                        const char* key2 = MFactions[b].search_key;
                        if (MFactions[a].is_alien() || MFactions[b].is_alien()
                        || (!_stricmp(key1, "PEACE")   && !_stricmp(key2, "SPARTANS"))
                        || (!_stricmp(key1, "GAIANS")  && !_stricmp(key2, "MORGAN"))
                        || (!_stricmp(key1, "HIVE")    && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "UNIV")    && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "FUNGBOY") && !_stricmp(key2, "BELIEVE"))
                        || (!_stricmp(key1, "PEACE")   && !_stricmp(key2, "PIRATES"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "PIRATES"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "MORGAN"))
                        || (!_stricmp(key1, "DRONE")   && !_stricmp(key2, "HIVE"))
                        || (!_stricmp(key1, "ANGELS")  && !_stricmp(key2, "HIVE"))
                        || (!_stricmp(key1, "ANGELS")  && !_stricmp(key2, "CYBORG"))) {
                            agenda_on(a, b, AGENDA_UNK_200);
                        }
                        if (MFactions[a].is_alien() && MFactions[b].is_alien()) {
                            set_treaty(b, a, DIPLO_UNK_80000000|DIPLO_UNK_40000000|DIPLO_UNK_8000000|\
                                DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_UNK_800|DIPLO_SHALL_BETRAY|\
                                DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_COMMLINK, 1);
                            set_agenda(b, a, AGENDA_PERMANENT|AGENDA_UNK_800|AGENDA_UNK_400|AGENDA_UNK_20|\
                                AGENDA_FIGHT_TO_DEATH|AGENDA_UNK_4|AGENDA_UNK_1, 1);
                        }
                    }
                }
            }
        }
        if (*GameRules & RULES_TIME_WARP) {
            mod_time_warp();
        } else if (!conf.skip_default_balance) {
            balance();
        }
    }
}

} // namespace engine
} // namespace alpha
