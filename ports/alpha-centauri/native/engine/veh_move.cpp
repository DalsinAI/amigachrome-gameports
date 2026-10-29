/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 movement speed, supply pods, monoliths, unit wake/skip and visibility,
 * ported from Thinker Mod src/veh.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: presentation calls go to veh_action_ui.h; mod_morale_veh is WP4's
 * morale_veh; label_get and _stricmp come from the rules loader (rules.h).
 * Deliberate changes are marked "Fix:".
 */
#include "engine.h"
#include "veh_action.h"
#include "veh_action_ui.h"
#include "wp3_deps.h"

namespace alpha {
namespace engine {

using namespace wp3_ui;

int __cdecl veh_find(int x, int y, int faction_id, int skip_veh_id) {
    *VehFindDist = 9999;
    int best_dist = 9999;
    int best_id = -1;

    for (int i = 0; i < *VehCount; i++) {
        if (i == skip_veh_id) {
            continue;
        }
        if (faction_id >= 0 && Vehs[i].faction_id != faction_id) {
            continue;
        }
        int dist = vector_dist(x, y, Vehs[i].x, Vehs[i].y);
        if (dist <= best_dist) {
            best_dist = dist;
            best_id = i;
        }
    }
    if (best_id >= 0) {
        *VehFindDist = best_dist;
    }
    return best_id;
}

/*
Renamed from speed_proto. Calculate the road move speed for specified prototype.
*/
int __cdecl proto_speed(int unit_id) {
    if (unit_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    UNIT* u = &Units[unit_id];
    int speed_val = u->speed();
    int triad = u->triad();
    int weapon_id = u->weapon_id;

    if (triad == TRIAD_AIR) {
        speed_val += u->reactor_id * 2;
    }
    if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_SLOW)) {
        speed_val--;
    }
    if (has_abil(unit_id, ABL_ANTIGRAV_STRUTS)) {
        speed_val += (triad == TRIAD_AIR) ? u->reactor_id * 2 : 1;
    }
    if (triad == TRIAD_AIR) {
        if (has_abil(unit_id, ABL_FUEL_NANOCELLS)) {
            speed_val += 2;
        }
        if (has_project(FAC_CLOUDBASE_ACADEMY, unit_id / MaxProtoFactionNum)) {
            speed_val += 2; // bonus from Aerospace Complex
        }
        if (has_abil(unit_id, ABL_AIR_SUPERIORITY)) {
            // generally -20% to -25%, in some cases higher due to lossy division rounding
            speed_val = (speed_val * 4) / 5;
        }
        if (Weapon[weapon_id].mode == WMODE_TRANSPORT) {
            speed_val /= 2; // 2nd penalty for air transports: -50%
        }
    }
    speed_val = clamp(speed_val, 1, 99) * Rules->move_rate_roads;
    return speed_val;
}

/*
Renamed from speed. Calculate the speed of a unit on roads taking into consideration
various factors. Parameter skip_morale should be normally set to false and
seems to be only set to true for certain combat calculations in battle_fight.
Fix: due to limited size of moves_spent field, speeds over 255 are not allowed.
*/
int __cdecl veh_speed(int veh_id, int skip_morale) {
    VEH* veh = &Vehs[veh_id];
    UNIT* u = &Units[veh->unit_id];
    if (veh->unit_id == BSC_FUNGAL_TOWER) {
        return 0; // cannot move
    }
    bool is_native = veh->is_native_unit();
    int speed_val = proto_speed(veh->unit_id);
    int triad = u->triad();
    if (triad == TRIAD_SEA && has_project(FAC_MARITIME_CONTROL_CENTER, veh->faction_id)) {
        speed_val += Rules->move_rate_roads * 2;
    }
    if (!skip_morale && morale_veh(veh_id, true, 0) == MORALE_ELITE
    && ((is_native && conf.native_elite_moves > 0)
    || (!is_native && conf.normal_elite_moves > 0))) {
        speed_val += Rules->move_rate_roads;
    }
    if (veh->damage_taken && triad != TRIAD_AIR) {
        int moves = speed_val / Rules->move_rate_roads;
        int reactor_val = veh->max_hitpoints();
        speed_val = clamp(reactor_val, 1, 99);
        speed_val = (moves * clamp(reactor_val - veh->damage_taken, 0, 9999)
            + speed_val - 1) / speed_val;
        speed_val = clamp(speed_val, (triad == TRIAD_SEA) ? 2 : 1, 999) * Rules->move_rate_roads;
    }
    return min(conf.max_movement_rate, speed_val);
}

/*
Determine whether the specified unit is eligible for monolith healing or morale upgrade.
*/
int __cdecl want_monolith(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    return veh_id >= 0 && veh->triad() != TRIAD_AIR
        && can_monolith(veh->unit_id)
        && (veh->damage_taken
        || (!(veh->state & VSTATE_MONOLITH_UPGRADED)
        && veh->offense_value() != 0
        && veh->morale < MORALE_ELITE
        && morale_veh(veh_id, true, 0) < MORALE_ELITE));
}

void __cdecl monolith(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    const bool is_player = veh->faction_id == *CurrentPlayerFaction;
    if (!can_monolith(veh->unit_id)) {
        return;
    }
    if (is_player && !*MultiplayerActive) {
        if (veh->order == ORDER_MOVE_TO
        && veh->x == veh->waypoint_x[0] && veh->y == veh->waypoint_y[0]) {
            veh->order = ORDER_NONE;
        }
        if (veh->order != ORDER_NONE) {
            return;
        }
    }
    if (!veh->offense_value()) {
        if (veh->damage_taken) {
            veh->damage_taken = 0;
            veh_skip(veh_id);
            draw_tile(veh->x, veh->y, 2);
            if (!is_player) {
                return;
            }
            NetMsg_pop(NetMsg, "MONOLITHHEAL", 5000, 0, "monolith_sm.pcx");
            *GameState |= STATE_UNK_4;
        } else if (!(veh->state & VSTATE_MONOLITH_UPGRADED)) {
            veh->state |= VSTATE_MONOLITH_UPGRADED;
            if (is_player && !*MultiplayerActive
            && !(*GameMorePreferences & MPREF_AUTO_ALWAYS_INSPECT_MONOLITH)) {
                NetMsg_pop(NetMsg, "SEEMONOLITH", -5000, 0, "monolith_sm.pcx");
                *GameState |= STATE_UNK_4;
            }
        }
    } else {
        for (int i = 1; i < MaxPlayerNum; i++) {
            del_site(i, 0, veh->x, veh->y, 0);
        }
        if (is_human(veh->faction_id)) {
            if (!*MultiplayerActive && !(*GameMorePreferences & MPREF_AUTO_ALWAYS_INSPECT_MONOLITH)) {
                FX_play(Sounds, 6);
                int value = popp(ScriptFile, "MONOLITH", 0, "monolith_sm.pcx", 0);
                FX_stop(Sounds, 6);
                if (value == 2) {
                    *GameMorePreferences |= MPREF_AUTO_ALWAYS_INSPECT_MONOLITH;
                    prefs_save(0);
                } else if (!value) {
                    for (int i = 1; i < MaxPlayerNum; i++) {
                        if (sq && sq->visible_items[i - 1] & BIT_MONOLITH) {
                            add_site(i, 0, 2, veh->x, veh->y);
                        }
                    }
                    return;
                }
            }
        } else {
            if (veh->state & VSTATE_MONOLITH_UPGRADED
            || morale_veh(veh_id, 1, 0) >= 6 || veh->morale >= 6 || !veh->offense_value()) {
                if (veh->damage_taken) {
                    veh->damage_taken = 0;
                    veh_skip(veh_id);
                    draw_tile(veh->x, veh->y, 2);
                }
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (sq && sq->visible_items[i - 1] & BIT_MONOLITH) {
                        add_site(i, 0, 2, veh->x, veh->y);
                    }
                }
                return;
            }
        }
        if (veh->damage_taken) {
            veh->damage_taken = 0;
            veh_skip(veh_id);
            draw_tile(veh->x, veh->y, 2);
            if (veh->state & VSTATE_MONOLITH_UPGRADED || morale_veh(veh_id, 1, 0) >= 6) {
                if (!is_player) {
                    return;
                }
                if (*GameMorePreferences & MPREF_AUTO_ALWAYS_INSPECT_MONOLITH) {
                    NetMsg_pop(NetMsg, "MONOLITHHEAL", 5000, 0, "monolith_sm.pcx");
                } else {
                    NetMsg_pop(NetMsg, "MONOLITHHEAL", -5000, 0, "monolith_sm.pcx");
                }
                *GameState |= STATE_UNK_4;
                return;
            }
        }
        if (veh->state & VSTATE_MONOLITH_UPGRADED || morale_veh(veh_id, 1, 0) >= 6) {
            if (is_player) {
                if (*GameMorePreferences & MPREF_AUTO_ALWAYS_INSPECT_MONOLITH) {
                    NetMsg_pop(NetMsg, "MONOLITHNO", 5000, 0, "monolith_sm.pcx");
                } else {
                    NetMsg_pop(NetMsg, "MONOLITHNO", -5000, 0, "monolith_sm.pcx");
                }
            }
        } else {
            int rnd_val = game_rand() % 32;
            if (!(sq && sq->items & BIT_UNK_2000000) || *CurrentTurn < 100) {
                rnd_val = 1;
                bit_set(veh->x, veh->y, BIT_UNK_2000000, 1);
            }
            veh->state |= VSTATE_MONOLITH_UPGRADED;
            veh->morale = clamp(veh->morale + 1, 0, 6);
            veh_skip(veh_id);
            mod_say_morale2(StrBuffer, veh_id, 0);
            for (char* c = StrBuffer; *c; c++) { // Original: CharLowerA
                *c = (char)tolower((unsigned char)*c);
            }
            parse_says(0, StrBuffer, -1, -1);
            if (rnd_val) {
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (sq && sq->visible_items[i - 1] & BIT_MONOLITH) {
                        add_site(i, 0, 2, veh->x, veh->y);
                    }
                }
            } else { // Monolith disappears
                bit_set(veh->x, veh->y, BIT_MONOLITH, 0);
                synch_bit(veh->x, veh->y, veh->faction_id);
            }
            draw_tile(veh->x, veh->y, 2);
            if (is_player) {
                FX_play(Sounds, 34);
                StrBuffer[0] = '\0';
                snprintf(StrBuffer, StrBufLen, "MONOLITH%d", rnd_val != 0);
                if (*GameMorePreferences & MPREF_AUTO_ALWAYS_INSPECT_MONOLITH) {
                    NetMsg_pop(NetMsg, StrBuffer, 5000, 0, "monolith_sm.pcx");
                } else {
                    NetMsg_pop(NetMsg, StrBuffer, -5000, 0, "monolith_sm.pcx");
                }
                *GameState |= STATE_UNK_4;
            }
        }
    }
}

int __cdecl do_unity_crash(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (!sq || (sq->landmarks & (LM_DISABLE|LM_UNITY)) != LM_UNITY) {
        return 0;
    }
    int code = sq->code_at();
    switch (code) {
    case 0:
    case 8: {
        int unit_id = (code == 0 ? BSC_UNITY_MINING_LASER : BSC_UNITY_SCOUT_CHOPPER);
        const char* label = (code == 0 ? "UNITYMINER" : "UNITYCHOPPER");
        if (faction_id == MapWin->cOwner) {
            FX_play(Sounds, 22);
            NetMsg_pop(NetMsg, label, -5000, 0, "unitfound_sm.pcx");
            FX_fade(Sounds, 22);
        }
        int veh_id = veh_init(unit_id, faction_id, x, y);
        if (veh_id >= 0) { // Fix: check return value
            Vehs[veh_id].morale = MORALE_DISCIPLINED;
        }
        return 1;
    }
    case 10: {
        int credits = (*CurrentTurn >= 50 ? 300 : 150);
        Factions[faction_id].energy_credits += credits;
        parse_num(0, credits);
        InfoWin_on_redraw(InfoWin);
        if (faction_id == MapWin->cOwner) {
            FX_play(Sounds, 22);
            NetMsg_pop(NetMsg, "UNITYENGINE", -5000, 0, "foundeng_sm.pcx");
            FX_fade(Sounds, 22);
        }
        return 1;
    }
    case 19: {
        for (int i = 0; i < *BaseCount; i++) {
            MFaction* fac = &MFactions[Bases[i].faction_id];
            if (has_fac_built(FAC_HEADQUARTERS, i) && !fac->is_alien()) {
                if (_stricmp(fac->filename, "FUNGBOY") != 0) {
                    spot_loc(Bases[i].x, Bases[i].y, faction_id);
                }
            }
        }
        if (faction_id == MapWin->cOwner) {
            draw_map(1);
            *GameDrawState |= 4;
            GraphicWin_redraw(WorldWin);
            FX_play(Sounds, 22);
            NetMsg_pop(NetMsg, "UNITYRADAR", -5000, 0, "foundradar_sm.pcx");
            FX_fade(Sounds, 22);
        }
        return 1;
    }
    default:
        return 0;
    }
}

static int goody_rand(int value) {
    return (value > 1 ? game_rand() % value : 0);
}

int __cdecl goody_box(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    Faction* f = &Factions[veh->faction_id];
    const int faction_id = veh->faction_id;
    const bool is_sea = is_ocean(sq);
    const bool is_player = faction_id == *CurrentPlayerFaction;
    const bool perihelion = *GameState & STATE_PERIHELION_ACTIVE;
    int x = veh->x;
    int y = veh->y;
    int event_value;
    int event_flag = 0;
    int landing_site_pod = 0;

    if (!sq || do_unity_crash(x, y, faction_id)) {
        return 0;
    }
    f->goody_opened++;
    if (*MultiplayerActive) {
        game_srand(*MapRandomSeed + f->goody_opened * 37);
    }
    if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)
    && (conf.early_research_start || *CurrentTurn >= -5 * f->SE_research_pending)) {
        f->tech_accumulated++;
    }
    int base_id = base_find(x, y);
    if (sq->items & (BIT_UNK_8000000|BIT_UNK_4000000)
    && (!f->base_count || (base_id >= 0 && Bases[base_id].faction_id == faction_id))) {
        landing_site_pod = 1;
        event_value = (sq->items & BIT_UNK_4000000 ? 9 : 10);
        bit_set(x, y, BIT_UNK_8000000|BIT_UNK_4000000, 0);
    } else if (*BaseFindDist > 2 || base_id < 0 || Bases[base_id].faction_id == faction_id) {
        if (sq->is_fungus() || is_sea) {
            goto GOODY_NEXT;
        }
        int modifier = 20;
        if (*BaseFindDist <= 12 || !f->region_total_bases[sq->region]) {
            modifier = 40;
        }
        if ((veh->plan() == PLAN_TERRAFORM && (*BaseFindDist < 12 || *CurrentTurn < 40)) || veh->plan() == PLAN_COLONY) {
            modifier += 10;
        }
        event_value = goody_rand(modifier);
    } else {
        event_value = 16;
        event_flag = 1;
    }

GOODY_START:
    assert(is_ocean(sq) == is_sea);
    assert(x == veh->x && y == veh->y);
    assert(event_flag >= 0 && event_flag < 4);
    assert(event_value >= 0 && event_value < 50);
    if (veh->plan() == PLAN_ARTIFACT) {
        if (!goody_rand(3) && !(veh->flags & VFLAG_IS_OBJECTIVE)) {
            if (is_player) {
                NetMsg_pop(NetMsg, is_sea ? "GOODYWHIRLPOOL" : "GOODYVANISH", 5000, 0, 0);
            }
            kill(veh_id);
            return 1; // Special return value
        }
    }
    switch (event_value) {
    case 0:
        int credits;
        credits = (goody_rand(is_sea + 3) + 1) * (*CurrentTurn >= (is_sea ? 100 : 50) ? 50 : 25);
        f->energy_credits += credits;
        if (is_player) {
            parse_num(0, credits);
            FX_play(Sounds, 22);
            NetMsg_pop(NetMsg, is_sea ? "GOODYHYDROGEN" : "GOODYURANIUM", -5000, 0, is_sea ? 0 : "supply_sm.pcx");
            FX_fade(Sounds, 22);
        }
        return 0;
    case 1:
        if (is_sea || sq->items & BIT_RIVER) {
            goto GOODY_NEXT;
        }
        for (auto& m : iterate_tiles(x, y, 0, 9)) {
            if (is_ocean(m.sq) || m.sq->items & BIT_RIVER) {
                goto GOODY_NEXT;
            }
        }
        if (is_player) {
            FX_play(Sounds, 38);
            NetMsg_pop(NetMsg, "GOODYRIVER", 5000, 0, "supply_sm.pcx");
        }
        bit_set(x, y, BIT_RIVER_SRC, 1);
        world_climate();
        draw_map(1);
        return 0;
    case 2:
        if (is_sea || sq->alt_level() >= ALT_TWO_ABOVE_SEA) {
            for (auto& m : iterate_tiles(x, y, 0, 121)) {
                if (!is_sea || (is_ocean(m.sq) && m.sq->region == sq->region)) {
                    spot_loc(m.x, m.y, faction_id);
                }
            }
            if (is_player) {
                FX_play(Sounds, 22);
                NetMsg_pop(NetMsg, is_sea ? "GOODYSONAR" : "GOODYVIEW", 5000, 0, "supply_sm.pcx");
                FX_fade(Sounds, 22);
                draw_map(1);
            }
            return 0;
        }
        if (*CurrentTurn >= 50) {
            goto GOODY_NEXT;
        }
        event_value = 8;
        goto GOODY_START;
    case 3:
        if (*GameRules & RULES_SCN_FORCE_CURRENT_DIFF_LEVEL && *DiffLevel <= 1) {
            goto GOODY_NEXT;
        }
        if (is_sea || f->base_count < 2 || sq->alt_level() >= ALT_TWO_ABOVE_SEA || *MultiplayerActive) {
            goto GOODY_NEXT;
        }
        if (goody_rand(2 * f->goody_earthquake + 2)) {
            goto GOODY_NEXT;
        }
        f->goody_earthquake++;
        FX_play(Sounds, 8);
        world_raise_alt(x, y);
        world_raise_alt(x, y);
        for (auto& m : iterate_tiles(x, y, 0, 25)) {
            if (m.sq->items & BIT_ROAD && !goody_rand(3)) {
                bit_set(m.x, m.y, BIT_ROAD, 0);
                synch_bit(m.x, m.y, faction_id);
            }
            if (m.i < 9) {
                rocky_set(m.x, m.y, mod_minerals_at(m.x, m.y));
            }
        }
        world_climate();
        draw_map(1);
        if (is_player) {
            NetMsg_pop(NetMsg, "GOODYQUAKE", -5000, 0, "quake_sm.pcx");
            if (!tut_check(0x800)) {
                return 0;
            }
            TutWin_tut_map(TutWin, "TERRAFORM", x, y, -1, 0, 0, -1, -1);
        }
        return 0;
    case 4:
        if (perihelion && !is_sea) {
            event_value = 7;
            goto GOODY_START;
        }
        if (*MultiplayerActive || veh->plan() == PLAN_ARTIFACT
        || (*GameRules & RULES_SCN_FORCE_CURRENT_DIFF_LEVEL && *DiffLevel <= 1)) {
            goto GOODY_NEXT;
        }
        if (*CurrentTurn < 25 || (*CurrentTurn < 50 && *DiffLevel <= 1)) {
            event_value = 8;
            goto GOODY_START;
        }
        int max_dist, tx, ty;
        if (veh->triad() == TRIAD_LAND && veh->plan() != PLAN_COLONY) {
            max_dist = veh->plan() != PLAN_TERRAFORM ? 24 : 8;
        }  else if (goody_rand(2)) {
            max_dist = 8;
        } else {
            goto GOODY_NEXT;
        }
        for (int iter = 0;;) {
            tx = goody_rand(*MapAreaX);
            ty = goody_rand(*MapAreaY);
            if (tx & 1) {
                tx--;
            }
            if (ty & 1) {
                tx++;
            }
            if (++iter >= 1000) {
                goto GOODY_NEXT;
            }
            MAP* bsq = mapsq(tx, ty);
            if (is_ocean(bsq) == is_sea && bsq->region == sq->region) {
                if (bsq->anything_at() < 0 && !zoc_any(tx, ty, faction_id)) {
                    if (!(f->player_flags & PFLAG_MAP_REVEALED) && !bsq->is_visible(faction_id)) {
                        int dist = map_range(x, y, tx, ty);
                        if (dist > 4 && dist <= max_dist) {
                            break;
                        }
                    }
                }
            }
        }
        if (is_player) {
            NetMsg_pop(NetMsg, is_sea ? "GOODYWAVE" : "GOODYGATE", -5000, 0, is_sea ? 0 : "stars_sm.pcx");
        }
        veh->moves_spent = 0;
        veh->order = 0;
        veh->state &= ~(VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_ON_ALERT);
        stack_put(stack_veh(veh_id, 1), tx, ty);
        if (is_player) {
            draw_tile_fixup(x, y, has_abil(veh->unit_id, ABL_DEEP_RADAR), 2);
        } else {
            draw_tile(x, y, 2);
        }
        if (is_player) {
            Console_focus(MapWin, tx, ty, faction_id);
            GraphicWin_redraw(WorldWin);
        }
        spot_all(veh_id, 1);
        return 0;
    case 5:
        if (perihelion && !is_sea) {
            event_value = 7;
            goto GOODY_START;
        }
        int fc_base_id, item_id;
        fc_base_id = base_find_2(x, y, faction_id);
        if (!*MultiplayerActive && fc_base_id >= 0
        && (item_id = Bases[fc_base_id].queue_items[0], item_id > -SP_ID_First)) {
            int cost;
            if (item_id < 0) {
                cost = Facility[-item_id].cost;
                parse_says(0, &Facility[-item_id].name[0], -1, -1);
            } else {
                if (Units[item_id].plan == PLAN_COLONY && Bases[fc_base_id].pop_size == 1) {
                    goto GOODY_NEXT;
                }
                cost = mod_veh_cost(item_id, fc_base_id, 0);
                parse_says(0, &Units[item_id].name[0], -1, -1);
            }
            int minerals = Bases[fc_base_id].minerals_accumulated;
            int prod_cost = cost * mod_cost_factor(faction_id, RSC_MINERAL, -1);
            if (minerals < 3 * prod_cost / 4) {
                if (conf.rare_supply_pods && prod_cost - minerals > 40 + goody_rand(60)) {
                    goto GOODY_NEXT;
                }
                Bases[fc_base_id].minerals_accumulated = prod_cost;
                if (is_player) {
                    parse_says(1, &Bases[fc_base_id].name[0], -1, -1);
                    NetMsg_pop(NetMsg, "GOODYBUILD", -5000, 0, "indbm_sm.pcx");
                }
                return 0;
            }
        }
        if (!is_sea && !(*GameRules & RULES_SCN_UNITY_PODS_NO_ARTIFACTS)) {
            int artifacts;
            artifacts = f->units_active[BSC_ALIEN_ARTIFACT];
            for (int i = 0; i < *BaseCount; i++) {
                if (Bases[i].faction_id == faction_id && Bases[i].state_flags & BSTATE_ARTIFACT_LINKED) {
                    artifacts++;
                }
            }
            if (artifacts < f->base_count || *GameState & STATE_SCN_VICT_ALL_ARTIFACTS_OBJ_UNIT) {
    case 6: // Skip conditions from previous label
                if (*GameRules & RULES_SCN_UNITY_PODS_NO_ARTIFACTS) {
                    goto GOODY_NEXT;
                }
                if (event_flag > 1 && event_value == 6) {
                    event_value = 5;
                    goto GOODY_START;
                }
                if (!is_sea || stack_check(veh_id, 8, -1, -1, -1) > 0) {
                    if (*MultiplayerActive && !(*GameState & STATE_SCN_VICT_ALL_ARTIFACTS_OBJ_UNIT)) {
                        for (int i = 1; i < MaxPlayerNum; i++) {
                            if (i != faction_id && is_human(i) && f->goody_artifact > Factions[i].goody_artifact) {
                                goto GOODY_NEXT;
                            }
                        }
                    }
                    f->goody_artifact++;
                    if (veh->plan() != PLAN_ARTIFACT) {
                        int spawn_id = veh_init(BSC_ALIEN_ARTIFACT, faction_id, x, y);
                        if (spawn_id >= 0) {
                            veh_skip(spawn_id);
                            draw_tile(x, y, 2);
                            if (is_player) {
                                NetMsg_pop(NetMsg, "GOODYARTIFACT", -5000, 0, "art_dis_sm.pcx");
                                FX_fade(Sounds, 27);
                            }
                        }
                        return 0;
                    }
                }
            }
        }
        goto GOODY_NEXT;
    case 7:
        if (event_flag > 1 || *GameRules & RULES_SCN_NO_NATIVE_LIFE
        || (*GameRules & RULES_SCN_FORCE_CURRENT_DIFF_LEVEL && *DiffLevel <= 1)) {
            goto GOODY_NEXT;
        }
        if (is_sea) {
            event_value = 8;
            goto GOODY_START;
        }
        base_find(x, y);
        if (*BaseFindDist < 3) {
            goto GOODY_NEXT;
        }
        if (*MultiplayerActive && faction_id == *RankingFactionIDUnk2 && !goody_rand(2)) {
            goto GOODY_NEXT;
        }
        bool val1, val2;
        val1 = 0;
        val2 = goody_rand(2) && *ExpansionEnabled;
        if (is_player) {
            FX_play(Sounds, 9);
            NetMsg_pop(NetMsg, "GOODYFUNGUS", -5000, 0, "fung_sm.pcx");
        }
        site_radius(x, y);
        for (auto& m : iterate_tiles(x, y, 0, 9)) {
            if (m.sq->alt_level() >= ALT_OCEAN_SHELF) {
                bit_set(m.x, m.y, BIT_FUNGUS, 1);
                bit_set(m.x, m.y, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
                synch_bit(m.x, m.y, faction_id);
                if (val1 == 0 && val2 == 1 && goody_rand(2) && conf.spawn_fungal_towers) {
                    if (*ExpansionEnabled && m.sq->anything_at() < 0 && !is_ocean(m.sq)) {
                        veh_init(BSC_FUNGAL_TOWER, 0, m.x, m.y);
                        val1 = 1;
                        val2 = 0;
                    }
                }
                draw_tile(m.x, m.y, 2);
            }
        }
        return 0;
    case 8:
        if (*GameRules & RULES_SCN_UNITY_PODS_NO_MONOLITHS || is_sea || bonus_at(x, y)) {
            goto GOODY_NEXT;
        }
        site_radius(x, y);
        bit_set(x, y, BIT_MONOLITH, 1);
        bit_set(x, y, BIT_FUNGUS, 0);
        synch_bit(x, y, faction_id);
        draw_tiles(x, y, 2);
        monolith(veh_id);
        return 0;
    case 9:
        if (*GameMoreRules & MRULES_SCN_UNITY_PODS_NO_VEHICLES || TechOwners[TECH_Orbital]
        || (is_sea && stack_check(veh_id, 8, -1, -1, -1) <= 0)) {
            goto GOODY_NEXT;
        }
        int alt_unit_id;
        alt_unit_id = BSC_UNITY_ROVER;
        if (TechOwners[TECH_Fossil]) {
            if (!goody_rand(3) && !f->units_active[BSC_UNITY_SCOUT_CHOPPER]) {
                base_find_2(x, y, faction_id);
                if (*BaseFindDist <= 3 * proto_speed(BSC_UNITY_SCOUT_CHOPPER)) {
                    alt_unit_id = BSC_UNITY_SCOUT_CHOPPER;
                }
            }
        }
        int alt_count, unit_count;
        unit_count = f->units_active[BSC_UNITY_FOIL] + (Continents[sq->region].tile_count > 50 ? 3 : 2);
        if (f->units_active[BSC_UNITY_FOIL] >= 2 || goody_rand(unit_count)) {
            // skipped
        } else if (is_sea) {
            alt_unit_id = BSC_UNITY_FOIL;
        } else {
            for (auto& m : iterate_tiles(x, y, 1, 9)) {
                if (is_ocean(m.sq) && Continents[m.sq->region].tile_count >= 80) {
                    x = m.x;
                    y = m.y;
                    alt_unit_id = BSC_UNITY_FOIL;
                    break;
                }
            }
        }
        alt_count = f->units_active[alt_unit_id];
        unit_count = f->units_active[BSC_UNITY_ROVER] + f->units_active[BSC_UNITY_SCOUT_CHOPPER] + f->units_active[BSC_UNITY_FOIL];
        if (alt_count && (alt_count > 1 || unit_count > (f->base_count + 1) / 2)) {
            event_value = goody_rand(2) + 12;
            x = veh->x;
            y = veh->y;
            sq = mapsq(x, y);
            goto GOODY_START;
        }
        if (conf.spawn_battle_ogres && alt_unit_id == BSC_UNITY_ROVER && !goody_rand(5) && *ExpansionEnabled) {
            int val = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (has_tech(TECH_Bioadap, i) && val < 1) {
                    val = 1;
                }
                if (has_tech(TECH_SentRes, i)) {
                    val = 2;
                }
            }
            alt_unit_id = (val > 1 ? BSC_BATTLE_OGRE_MK3 : (val > 0 ? BSC_BATTLE_OGRE_MK2 : BSC_BATTLE_OGRE_MK1));
        }
        int alt_veh_id;
        alt_veh_id = veh_init(alt_unit_id, faction_id, x, y);
        if (alt_veh_id >= 0) {
            if (landing_site_pod || *CurrentTurn < 50) {
                Vehs[alt_veh_id].home_base_id = -1;
            }
            Vehs[alt_veh_id].morale = (alt_unit_id == BSC_BATTLE_OGRE_MK2 ? 4 : (alt_unit_id == BSC_BATTLE_OGRE_MK3 ? 6 : 2));
            if (is_sea && Units[alt_unit_id].triad() == TRIAD_LAND) {
                sleep(alt_veh_id);
            }
            if (is_player) {
                parse_says(0, &Units[alt_unit_id].name[0], -1, -1);
                if (is_battle_ogre(alt_unit_id)) {
                    NetMsg_pop(NetMsg, "OGREPOD", is_sea ? 5000 : -5000, 0, "ogre_sm.pcx");
                } else {
                    NetMsg_pop(NetMsg, "GOODYPOD", is_sea ? 5000 : -5000, 0, "supply_sm.pcx");
                }
            }
            return 0;
        }
        goto GOODY_NEXT;
    case 10:
        if (*GameMoreRules & MRULES_SCN_UNITY_PODS_NO_TECH) {
            goto GOODY_NEXT;
        }
        int rnd_id;
        if (!goody_rand(4)
        && !(rnd_id = goody_rand(7) + 1, rnd_id == faction_id || f->diplo_status[rnd_id] & DIPLO_COMMLINK)) {
            if (is_human(faction_id) && is_alive(rnd_id) && !is_human(rnd_id) && !is_alien(rnd_id)) {
                treaty_on(faction_id, rnd_id, DIPLO_COMMLINK);
                if (is_player) {
                    parse_says(0, get_title(rnd_id), -1, -1);
                    parse_says(1, get_name(rnd_id), -1, -1);
                    parse_says(2, get_noun(rnd_id), -1, -1);
                    NetMsg_pop(NetMsg, "GOODYCOMM", is_sea ? 5000 : -5000, 0, "supply_sm.pcx");
                }
                return 0;
            }
        }
        if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
            if (*MultiplayerActive && !landing_site_pod) {
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (i != faction_id && is_human(i)
                    && f->goody_tech > Factions[i].goody_tech + 1
                    && (f->ranking > Factions[i].ranking || f->tech_ranking > Factions[i].tech_ranking)) {
                        goto GOODY_NEXT;
                    }
                }
            }
            int iter_val = 0;
            int rand_val = goody_rand(MaxTechnologyNum);
            while (true) {
                int tech_id = (rand_val + iter_val) % MaxTechnologyNum;
                if (!has_tech(tech_id, faction_id) && tech_avail(tech_id, faction_id)
                && (!tech_is_preq(tech_id, TECH_Brain, 9999) || TechOwners[TECH_Brain])) {
                    int val = 0;
                    for (int iter_id : {Tech[tech_id].preq_tech1, Tech[tech_id].preq_tech2}) {
                        if (iter_id == TECH_None) {
                            val++;
                        }
                        if (iter_id >= 0 && Tech[iter_id].preq_tech1 == TECH_None && Tech[iter_id].preq_tech2 == TECH_None) {
                            val++;
                            break;
                        }
                    }
                    if (val >= 2) {
                        if (tech_id != TECH_PrPsych && tech_id != TECH_FldMod && tech_id != TECH_AdapEco
                        && tech_id != TECH_Bioadap && tech_id != TECH_SentRes) {
                            if (!landing_site_pod) {
                                f->goody_tech++;
                            }
                            if (is_player) {
                                if (*MultiplayerActive) {
                                    strncpy(StrBuffer, "GOODYDATA1", StrBufLen);
                                    parse_says(0, Tech[tech_id].name, -1, -1);
                                } else {
                                    strncpy(StrBuffer, "GOODYDATA", StrBufLen);
                                }
                                NetMsg_pop(NetMsg, StrBuffer, is_sea ? 5000 : -5000, 0, "supply_sm.pcx");
                            }
                            tech_achieved(faction_id, tech_id, 0, 0);
                            if (landing_site_pod) {
                                f->tech_ranking--;
                            }
                            return 0;
                        }
                        goto GOODY_NEXT;
                    }
                }
                if (++iter_val >= MaxTechnologyNum) {
                    goto GOODY_NEXT;
                }
            }
        }
        goto GOODY_NEXT;
    case 11:
        if (bad_reg(sq->region) || sq->is_rocky()) {
            goto GOODY_NEXT;
        }
        if (sq->alt_level() >= ALT_OCEAN_SHELF) {
            int choice;
            if (is_sea) {
                choice = 0;
            } else {
                base_find(x, y);
                if (*BaseFindDist < 3) {
                    goto GOODY_NEXT;
                }
                choice = goody_rand(4);
            }
            site_radius(x, y);
            int terrain_addons = 0;
            int values[9] = {};
            for (int i = 1; i < 9; i++) {
                values[i] = goody_rand(3 - (choice > 1));
            }
            for (auto& m : iterate_tiles(x, y, 0, 9)) {
                if (!m.i || values[m.i]) {
                    if (m.i && m.sq->items & BIT_SUPPLY_REMOVE) {
                        continue;
                    }
                    if (is_sea) {
                        if (m.sq->alt_level() >= ALT_SHORE_LINE) {
                            continue;
                        }
                        if (m.sq->alt_level() < ALT_OCEAN_SHELF) {
                            if (m.i) {
                                continue;
                            }
                            alt_set_both(m.x, m.y, 2);
                        }
                    } else {
                        if (m.sq->alt_level() < ALT_SHORE_LINE) {
                            continue;
                        }
                        if (m.sq->is_rocky() && choice < 2) {
                            continue;
                        }
                        if (!m.sq->is_rocky() && !m.sq->is_rolling() && choice == 2) {
                            continue;
                        }
                    }
                    bit_set(m.x, m.y, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_FUNGUS|BIT_MINE, 0);
                    if (is_sea) {
                        bit_set(m.x, m.y, BIT_FARM, 1);
                    } else if (choice == 0) {
                        bit_set(m.x, m.y, BIT_FOREST, 1);
                        bit_set(m.x, m.y, BIT_FARM|BIT_SOLAR|BIT_MINE, 0);
                    } else if (choice == 1) {
                        bit_set(m.x, m.y, BIT_FARM, 1);
                        bit_set(m.x, m.y, BIT_FOREST, 0);
                    } else if (choice == 2) {
                        bit_set(m.x, m.y, BIT_MINE|BIT_ROAD, 1);
                        bit_set(m.x, m.y, BIT_FOREST|BIT_SOLAR, 0);
                    } else if (choice == 3) {
                        bit_set(m.x, m.y, BIT_SOLAR, 1);
                        bit_set(m.x, m.y, BIT_FOREST|BIT_MINE, 0);
                    }
                    synch_bit(m.x, m.y, faction_id);
                    draw_tile(m.x, m.y, 2);
                    terrain_addons++;
                }
            }
            if (!terrain_addons) {
                goto GOODY_NEXT;
            }
            if (is_player) {
                if (is_sea) {
                    NetMsg_pop(NetMsg, "GOODYKELP", -5000, 0, "kelp_sm.pcx");
                } else if (choice == 0) {
                    NetMsg_pop(NetMsg, "GOODYFOREST", -5000, 0, "forgr_sm.pcx");
                } else if (choice == 1) {
                    NetMsg_pop(NetMsg, "GOODYFARM", -5000, 0, "forgr_sm.pcx");
                } else if (choice == 2) {
                    NetMsg_pop(NetMsg, "GOODYMINE", -5000, 0, "supply_sm.pcx");
                } else if (choice == 3) {
                    NetMsg_pop(NetMsg, "GOODYSOLAR", -5000, 0, "supply_sm.pcx");
                }
            }
            return 0;
        }
        event_value = 12;
        goto GOODY_START;
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        if (*GameRules & RULES_SCN_NO_NATIVE_LIFE || (*GameRules & RULES_SCN_FORCE_CURRENT_DIFF_LEVEL && *DiffLevel <= 1)) {
            goto GOODY_NEXT;
        }
        int find_dist, is_spore_launcher;
        is_spore_launcher = 0;
        base_find_2(x, y, faction_id);
        find_dist = *BaseFindDist;
        base_find(x, y);
        if (*BaseFindDist <= 1) {
            goto GOODY_NEXT;
        }
        if (event_value > 13) {
            int val = (sq->is_fungus() ? 2 : 4);
            if (find_dist * (perihelion ? 2 : 1) < val * (event_value - 13) + 2 * (4 - *MapNativeLifeForms)) {
                goto GOODY_NEXT;
            }
        }
        if (*MultiplayerActive && faction_id == *RankingFactionIDUnk2 && !goody_rand(2)) {
            goto GOODY_NEXT;
        }
        if (is_sea) {
            if (event_value > (perihelion ? 13 : 12)) {
                goto GOODY_NEXT;
            }
            veh->moves_spent += Rules->move_rate_roads;
        }
        int native_spawns, tile_offset;
        native_spawns = 0;
        tile_offset = 0;
        while (tile_offset < 8) {
            int i = (*CurrentTurn + tile_offset) % 8;
            int bx = wrap(x + BaseOffsetX[i]);
            int by = y + BaseOffsetY[i];
            MAP* bsq = mapsq(bx, by);
            if (bsq && is_ocean(bsq) == is_sea && bsq->anything_at() < 0) {
                if (is_sea) {
                    int spawn_veh_id = veh_init(BSC_ISLE_OF_THE_DEEP, 0, bx, by);
                    if (spawn_veh_id >= 0) {
                        native_spawns++;
                        Vehs[spawn_veh_id].visibility |= bsq->visibility;
                        int val = min(veh_cargo(spawn_veh_id), goody_rand(2) + 1);
                        while (--val >= 0) {
                            if (goody_rand(5) || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                                veh_init(BSC_MIND_WORMS, 0, bx, by);
                            } else {
                                veh_init(BSC_SPORE_LAUNCHER, 0, bx, by);
                                is_spore_launcher = 1;
                            }
                        }
                        break;
                    }
                } else {
                    base_find(bx, by);
                    if (*BaseFindDist > 1) {
                        int spawn_id;
                        if (goody_rand(5) || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                            spawn_id = veh_init(BSC_MIND_WORMS, 0, bx, by);
                        } else {
                            spawn_id = veh_init(BSC_SPORE_LAUNCHER, 0, bx, by);
                            is_spore_launcher = 1;
                        }
                        if (spawn_id >= 0) {
                            native_spawns++;
                            Vehs[spawn_id].visibility |= bsq->visibility;
                            draw_tile(bx, by, 2);
                            if (event_value > (perihelion ? 13 : 12) || *CurrentTurn < 15 || *BaseFindDist <= 3) {
                                break;
                            }
                        }
                    }
                }
            }
            int val = clamp(native_spawns ? 5 - f->base_count / 2 : 1, 1, 4);
            tile_offset += val;
        }
        if (!native_spawns) {
            goto GOODY_NEXT;
        }
        if (is_player) {
            FX_play(Sounds, 17);
            if (is_sea) {
                NetMsg_pop(NetMsg, "GOODYISLE", -5000, 0, "isle_sm.pcx");
            } else if (is_spore_launcher) {
                NetMsg_pop(NetMsg, "GOODYWORMS", -5000, 0, "sporlnch_sm.pcx");
            } else {
                NetMsg_pop(NetMsg, "GOODYWORMS", -5000, 0, "mindworm_sm.pcx");
            }
            FX_fade(Sounds, 17);
        }
        return 0;
    case 18:
        if (is_sea) {
            if (goody_rand(4)) {
                goto GOODY_NEXT;
            }
        } else if (!veh->offense_value()) {
            goto GOODY_NEXT;
        }
        if (veh->plan() == PLAN_ARTIFACT) {
            goto GOODY_NEXT;
        }
        if (*MultiplayerActive) {
            int found = 0;
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (is_alive(i) && is_human(i) && i != faction_id) {
                    if (is_sea) {
                        if (Factions[i].unk_70 >= f->unk_70) {
                            found = 1;
                        }
                    } else if (Factions[i].total_combat_units >= f->total_combat_units) {
                        found = 1;
                    }
                    if (Factions[i].total_combat_units < f->total_combat_units / 2) {
                        goto GOODY_NEXT;
                    }
                }
            }
            if (!found) {
                goto GOODY_NEXT;
            }
        }
        int clone_id;
        clone_id = veh_init(veh->unit_id, faction_id, x, y);
        if (clone_id >= 0) {
            Vehs[clone_id].home_base_id = -1;
            Vehs[clone_id].morale = veh->morale;
            Vehs[clone_id].damage_taken = veh->damage_taken;
            if (is_player) {
                parse_says(0, veh->name(), -1, -1);
                NetMsg_pop(NetMsg, "GOODYCLONE", -5000, 0, "clone_sm.pcx");
            }
            return 0;
        }
GOODY_NEXT:
        x = veh->x;
        y = veh->y;
        sq = mapsq(x, y);
        event_value = goody_rand(14);
        if (event_flag) {
            if (event_flag > 2) {
                return 0;
            }
            if (event_flag == 2) {
                event_flag = 3;
            }
        } else {
            event_flag = 1;
        }
        if ((!y || y == *MapAreaY - 1) && !goody_rand(3)) {
            event_value = 12;
        }
        if (!goody_rand(100)) {
            event_value = 18;
        }
        goto GOODY_START;
    default:
        if (event_flag > 1) {
            return 0;
        }
        if (!is_sea || sq->alt_level() >= ALT_OCEAN_SHELF) {
            if (!sq->is_fungus() && y > 1 && y < *MapAreaY - 2
            && !(*GameRules & RULES_SCN_UNITY_PODS_NO_RESOURCES)) {
                bit_set(x, y, BIT_BONUS_RES, 1);
                int bonus_val = bonus_at(x, y);
                site_radius(x, y);
                if (is_player && bonus_val) {
                    TextLabel label = TL_Bonus;
                    if (bonus_val == RES_NUTRIENT) {
                        wave_it(40);
                        label = TL_Nutrient;
                    } else if (bonus_val == RES_MINERAL) {
                        wave_it(42);
                        label = TL_Minerals;
                    } else if (bonus_val == RES_ENERGY) {
                        ambience(210);
                        wave_it(41);
                        label = TL_Energy;
                    }
                    parse_says(0, label_get(label), -1, -1);
                    NetMsg_pop(NetMsg, "GOODYRESOURCE", 5000, 0, "supply_sm.pcx");
                }
                draw_tile(x, y, 2);
                if (!goody_rand(3) || event_flag) {
                    return 0;
                }
                event_flag = 2;
            }
        }
        goto GOODY_NEXT;
    }
}

int __cdecl study_artifact(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int x = veh->x;
    int y = veh->y;
    int faction_id = veh->faction_id;
    int base_id = base_at(x, y);
    if (base_id >= 0 && project_base(FAC_UNIVERSAL_TRANSLATOR) != base_id) {
        BASE* base = &Bases[base_id];
        if (base->state_flags & BSTATE_ARTIFACT_LINKED) {
            if (!(base->state_flags & BSTATE_ARTIFACT_ALREADY_LINKED)
            && faction_id == *CurrentPlayerFaction) {
                parse_says(0, base->name, -1, -1);
                NetMsg_pop(NetMsg, "ALREADYARTIFACT", -5000, 0, "art_dis_sm.pcx");
                base->state_flags |= BSTATE_ARTIFACT_ALREADY_LINKED;
            }
            return 0;
        }
    }
    veh_kill(veh_id);
    draw_tile(x, y, 2);
    if (faction_id == *CurrentPlayerFaction) {
        FX_play(Sounds, 35);
        popp(ScriptFile, "ARTIFACTTECH", 0, "art_dis_sm.pcx", 0);
    }
    if (Factions[faction_id].player_flags & PFLAG_UNK_10) {
        Factions[faction_id].player_flags |= PFLAG_UNK_4;
    } else {
        Factions[faction_id].player_flags |= PFLAG_UNK_10;
    }
    if (base_id >= 0) {
        Bases[base_id].state_flags |= BSTATE_ARTIFACT_LINKED;
    }
    int best_tech_id = -1;
    int best_val = -1;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (!has_tech(i, faction_id) && tech_avail(i, faction_id)) {
            // Fix: removed here additional code that was never used
            // due to conditions in tech_avail already excluding it.
            int rnd_val = game_randv(8);
            if (rnd_val > best_val) {
                best_val = rnd_val;
                best_tech_id = i;
            }
        }
    }
    if (best_tech_id >= 0) {
        tech_achieved(faction_id, best_tech_id, 0, 0);
    }
    return 1;
}

/*
Skip vehicle turn by adjusting spent moves to maximum available moves.
Formers are a special case since they can build items without moving on the same turn.
To work around several issues, this version adjusts more fields in addition to moves_spent.
*/
void __cdecl veh_skip(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    int moves = veh_speed(veh_id, 0);

    if (is_human(veh->faction_id)) {
        if (veh->is_former() && veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO) {
            veh->state |= VSTATE_WORKING;
        }
        if (conf.activate_skipped_units) {
            if (!veh->moves_spent && veh->order < ORDER_FARM
            && !(veh->state & (VSTATE_HAS_MOVED|VSTATE_MADE_AIRDROP))) {
                veh->flags |= VFLAG_FULL_MOVE_SKIPPED;
            } else if (veh->triad() != TRIAD_AIR) {
                // veh_skip may get called twice on aircraft units
                veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
            }
        }
    } else if (veh->faction_id > 0) {
        if (want_monolith(veh_id) && bit_at(veh->x, veh->y) & BIT_MONOLITH) {
            monolith(veh_id);
        }
    }
    veh->moves_spent = moves;
}

int __cdecl mod_veh_skip(int veh_id) {
    veh_skip(veh_id);
    return VEH_SKIP;
}

int __cdecl veh_wake(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    if (veh->order >= ORDER_FARM && veh->order < ORDER_MOVE_TO && !(veh->state & VSTATE_WORKING)) {
        veh->moves_spent = veh_speed(veh_id, 0) - Rules->move_rate_roads;
        if (veh->movement_turns) {
            veh->movement_turns = max(0, veh->movement_turns - contribution(veh_id, veh->order - 4));
        }
    }
    if (veh->state & VSTATE_ON_ALERT && !(veh->state & VSTATE_HAS_MOVED)
    && veh->order_auto_type == ORDERA_ON_ALERT) {
        veh->moves_spent = 0;
    }
    if (conf.activate_skipped_units && is_human(veh->faction_id)) {
        if (veh->flags & VFLAG_FULL_MOVE_SKIPPED
        && !(veh->state & (VSTATE_HAS_MOVED|VSTATE_MADE_AIRDROP))) {
            veh->moves_spent = 0;
        }
        veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
    }
    veh->order = ORDER_NONE;
    veh->state &= ~(VSTATE_UNK_8000000|VSTATE_UNK_2000000|VSTATE_UNK_1000000|VSTATE_EXPLORE|VSTATE_ON_ALERT);
    return veh_id;
}

int __cdecl find_return_base(int veh_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq;
    debug("find_return_base %2d %2d %s\n", veh->x, veh->y, veh->name());
    TileSearch ts;
    ts.init(veh->x, veh->y, veh->triad() == TRIAD_SEA ? TS_SEA_AND_SHORE : veh->triad());
    while ((sq = ts.get_next()) != NULL) {
        if (sq->is_base() && sq->owner == veh->faction_id) {
            return base_at(ts.rx, ts.ry);
        }
    }
    int region = region_at(veh->x, veh->y);
    int min_dist = INT_MAX;
    int base_id = -1;
    bool sea = region > MaxRegionLandNum;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == veh->faction_id) {
            int base_region = region_at(base->x, base->y);
            int dist = map_range(base->x, base->y, veh->x, veh->y)
                * ((base_region > MaxRegionLandNum) == sea ? 1 : 4)
                * (base_region == region ? 1 : 8);
            if (dist < min_dist) {
                base_id = i;
                min_dist = dist;
            }
        }
    }
    return base_id;
}

void __cdecl spot_base(int base_id, int faction_id) {
    BASE* base = &Bases[base_id];
    base->visibility |= 1 << faction_id;
    base->factions_pop_size_intel[faction_id] = base->pop_size;
    int x = base->x;
    int y = base->y;
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (faction_id == MapWin->cOwner && !sq->is_visible(faction_id)
        && !(*GameState & STATE_OMNISCIENT_VIEW) && !map_is_known(faction_id)) {
            sq->landmarks |= LM_UNK_400000;
            *GameDrawState |= 1;
        }
        sq->visibility |= 1 << faction_id;
        synch_bit(x, y, faction_id);
    }
}

void __cdecl spot_stack(int veh_id, int faction_id) {
    if (veh_id >= 0) {
        int x = Vehs[veh_id].x;
        int y = Vehs[veh_id].y;
        MAP* sq = mapsq(x, y);
        if (sq) {
            if (faction_id == MapWin->cOwner && !sq->is_visible(faction_id)
            && !(*GameState & STATE_OMNISCIENT_VIEW) && !map_is_known(faction_id)) {
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
            sq->visibility |= 1 << faction_id;
            synch_bit(x, y, faction_id);
        }
    }
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        Vehs[i].visibility |= 1 << faction_id;
        if (faction_id) {
            Vehs[i].flags &= ~(VFLAG_INVISIBLE|VFLAG_LURKER);
        }
    }
}

void __cdecl unspot_stack(int veh_id) {
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        Vehs[i].visibility = 0;
    }
}

void __cdecl spot_loc(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        if (faction_id == MapWin->cOwner && !sq->is_visible(faction_id)
        && !(*GameState & STATE_OMNISCIENT_VIEW) && !map_is_known(faction_id)) {
            sq->landmarks |= LM_UNK_400000;
            *GameDrawState |= 1;
        }
        sq->visibility |= 1 << faction_id;
        synch_bit(x, y, faction_id);
    }
    int base_id = base_at(x, y);
    if (base_id < 0) {
        spot_stack(veh_at(x, y), faction_id);
    } else {
        spot_base(base_id, faction_id);
    }
}

int __cdecl want_to_wake(int faction_id, int veh_id, int veh_id2) {
    int triad = Vehs[veh_id].triad();
    if (triad == TRIAD_AIR && Vehs[veh_id].movement_turns && Vehs[veh_id].order == ORDER_MOVE_TO) {
        MAP* sq = mapsq(Vehs[veh_id].waypoint_x[0], Vehs[veh_id].waypoint_y[0]);
        if (sq && sq->is_base()) {
            int owner = sq->veh_owner();
            int veh_faction = Vehs[veh_id].faction_id;
            if (owner == veh_faction || (Factions[veh_faction].diplo_status[owner] & DIPLO_PACT)) {
                return 0;
            }
        }
    }
    uint32_t status = Factions[Vehs[veh_id].faction_id].diplo_status[faction_id];
    int value;
    if (*MultiplayerActive) {
        value = !(status & DIPLO_PACT);
    } else {
        int prefs = *GamePreferences;
        if (status & DIPLO_PACT) {
            value = prefs & PREF_AUTO_END_MOVE_SPOT_VEH_PACT;
        } else if (status & DIPLO_TREATY) {
            value = prefs & PREF_AUTO_END_MOVE_SPOT_VEH_TREATY;
        } else if (status & DIPLO_TRUCE) {
            value = prefs & PREF_AUTO_END_MOVE_SPOT_VEH_TRUCE;
        } else {
            value = prefs & PREF_AUTO_END_MOVE_SPOT_VEH_WAR;
        }
        if (!(prefs & PREF_AUTO_DONT_END_MOVE_DIFF_TRIAD)) {
            return value;
        }
    }
    if (veh_id >= 0 && veh_id2 >= 0) {
        int other_triad = Vehs[veh_id2].triad();
        if (triad != other_triad
        && ((Vehs[veh_id].state & (VSTATE_UNK_1000000 | VSTATE_ON_ALERT)) != (VSTATE_UNK_1000000 | VSTATE_ON_ALERT)
        || other_triad == TRIAD_LAND)) {
            return 0;
        }
    }
    return value;
}

void __cdecl wake_stack(int veh_id) {
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        if (Vehs[i].order == ORDER_SENTRY_BOARD) {
            MAP* sq = mapsq(Vehs[i].x, Vehs[i].y);
            if (Vehs[i].triad() != TRIAD_LAND
            || (!is_ocean(sq)
            && (Vehs[i].waypoint_x[0] < 0
            || Vehs[Vehs[i].waypoint_x[0]].triad() != TRIAD_AIR
            || sq->is_airbase()))) {
                Vehs[i].order = ORDER_NONE;
                Vehs[i].state &= ~VSTATE_EXPLORE;
            }
        }
    }
}

void __cdecl spot_all(int veh_id, int flag) {
    if (!*MultiplayerActive) {
        flag = 1;
    }
    VEH* const veh = &Vehs[veh_id];
    const int veh_x = veh->x;
    const int veh_y = veh->y;
    const int triad = veh->triad();
    MAP* const veh_sq = mapsq(veh_x, veh_y);
    if (!veh_sq) {
        assert(0);
        return;
    }
    int nexus_count = 0;
    int has_tower = 0;
    int has_native = 0;
    int stealth = 0;
    if (has_abil(veh->unit_id, ABL_DEEP_PRESSURE_HULL)
    || has_abil(veh->unit_id, ABL_CLOAKED)
    || (veh->flags & VFLAG_INVISIBLE)) {
        stealth = 1;
    } else if (veh_sq->is_fungus() && veh->unit_id != BSC_FUNGAL_TOWER) {
        stealth = 2;
    }
    const int faction_id = veh->faction_id;
    const int faction_bit = 1 << faction_id;
    const bool radar = has_abil(veh->unit_id, ABL_DEEP_RADAR)
        || (is_sensor(veh_x, veh_y) && whose_territory(faction_id, veh_x, veh_y, 0, 0) == faction_id);
    const bool on_ocean = is_ocean(veh_sq);
    Faction* const plr = &Factions[faction_id];
    const int visible_end = radar ? 25 : 9;
    const int scan_end = radar ? 49 : 25;
    int encounter_mask = 0;
    int draw_range = 0;

    auto clear_alert = [](VEH* v) {
        if ((v->state & (VSTATE_UNK_2000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_2000000|VSTATE_ON_ALERT)
        && v->patrol_current_point < v->waypoint_count) {
            v->state &= ~(VSTATE_UNK_2000000|VSTATE_ON_ALERT);
        }
        if ((v->state & (VSTATE_UNK_1000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_1000000|VSTATE_ON_ALERT)) {
            v->state &= ~(VSTATE_UNK_1000000|VSTATE_ON_ALERT);
        }
        v->state &= ~VSTATE_EXPLORE;
        if (v->order == ORDER_MOVE_TO) {
            v->order = ORDER_NONE;
        }
    };
    auto spot_sensor = [&](int x, int y, MAP* sq) {
        if (!is_sensor(x, y)) {
            return;
        }
        int owner = sq->owner;
        if (owner <= 0 || owner == faction_id || stealth == 1 || sq->base_who() >= 0) {
            return;
        }
        spot_stack(veh_id, owner);
        if ((plr->diplo_status[owner] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) || !flag) {
            return;
        }
        if (triad != TRIAD_LAND) {
            add_goal(owner, AI_GOAL_UNK_1, 2, x, y, -1);
        } else {
            add_goal(owner, AI_GOAL_ATTACK, 2, x, y, -1);
            add_goal(owner, AI_GOAL_DEFEND, 1, x, y, -1);
        }
    };
    auto check_nearby = [&](int x, int y, MAP* sq) -> bool {
        // Fix: added tile_owner bounds checking and radar related early return
        for (int n = 0; n < 25; n++) {
            int nx = wrap(x + TableOffsetX[n]);
            int ny = y + TableOffsetY[n];
            if (!on_map(nx, ny)) {
                continue;
            }
            MAP* nsq = mapsq(nx, ny);
            if (n < 9) {
                int tile_owner = nsq->anything_at();
                if (tile_owner == faction_id) {
                    return true;
                }
                if (tile_owner >= 0 && (plr->diplo_status[tile_owner] & DIPLO_PACT)) {
                    for (int k = veh_at(nx, ny); k >= 0; k = Vehs[k].next_veh_id_stack) {
                        if (Vehs[k].faction_id == faction_id) {
                            return true;
                        }
                    }
                }
            }
            if (is_sensor(nx, ny)) {
                if (nsq->owner == faction_id) {
                    return true;
                }
                int base_id = base_find(veh_x, veh_y);
                if (base_id >= 0) {
                    int dx = x_dist(veh_x, Bases[base_id].x);
                    int dy = abs(veh_y - Bases[base_id].y);
                    if ((dx == 0 || dx == 2) && (dy == 0 || dy == 2)) {
                        if (has_fac_built(FAC_GEOSYNC_SURVEY_POD, base_id)) {
                            return true;
                        }
                    }
                }
            }
            if (nsq->base_who() == faction_id) {
                return true;
            }
            int veh_owner = nsq->veh_who();
            if (veh_owner == faction_id
            || (veh_owner >= 0 && (Factions[veh_owner].diplo_status[faction_id] & DIPLO_PACT))) {
                for (int k = veh_top(veh_at(nx, ny)); k >= 0; k = Vehs[k].next_veh_id_stack) {
                    if (Vehs[k].faction_id == faction_id
                    && has_abil(Vehs[k].unit_id, ABL_DEEP_RADAR)) {
                        int other_triad = Vehs[k].triad();
                        if (other_triad == TRIAD_AIR
                        || is_ocean(sq) == (other_triad == TRIAD_SEA)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    };
    if (!faction_id) {
        veh->visibility &= veh_sq->visibility;
    }
    if ((*GameMorePreferences & MPREF_MAP_SHOW_FOG_WAR)
    && faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)) {
        for (int i = 0; i < scan_end; i++) {
            int x = wrap(veh_x + TableOffsetX[i]);
            int y = veh_y + TableOffsetY[i];
            if (!on_map(x, y)) {
                continue;
            }
            MAP* sq = mapsq(x, y);
            bool visible = true;
            if (i >= visible_end) {
                if (sq->landmarks & LM_UNK_200000) {
                    continue;
                }
                visible = check_nearby(x, y, sq);
            }
            if (((sq->landmarks & LM_UNK_200000) != 0) == visible) {
                int draw_level = i < 9 ? 1 : (i < 25 ? 2 : 3);
                draw_range = max(draw_range, draw_level);
            }
        }
    }
    for (int i = 0; i < visible_end; i++) {
        int x = wrap(veh_x + TableOffsetX[i]);
        int y = veh_y + TableOffsetY[i];
        if (!on_map(x, y)) {
            continue;
        }
        MAP* sq = mapsq(x, y);
        if ((sq->landmarks & (LM_DISABLE|LM_NEXUS)) == LM_NEXUS) {
            nexus_count++;
        }
        if (i < 9 || triad == TRIAD_AIR || is_ocean(sq) == (triad == TRIAD_SEA)) {
            if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)
            && !map_is_known(faction_id)) {
                if (!sq->is_visible(faction_id)) {
                    sq->landmarks |= LM_UNK_400000;
                    *GameDrawState |= 1;
                }
                if (((sq->items ^ sq->visible_items[faction_id - 1]) & ~(BIT_BASE_IN_TILE|BIT_VEH_IN_TILE))
                || !sq->is_visible(faction_id)) {
                    draw_range = max(draw_range, i >= 9 ? 3 : 2);
                }
            }
            if (faction_id) {
                sq->visible_items[faction_id - 1] = sq->items;
            }
            sq->visibility |= faction_bit;
        }
    }
    for (int i = visible_end; i < scan_end; i++) {
        int x = wrap(veh_x + TableOffsetX[i]);
        int y = veh_y + TableOffsetY[i];
        if (!on_map(x, y)) {
            continue;
        }
        MAP* sq = mapsq(x, y);
        if (sq->is_visible(faction_id)) {
            continue;
        }
        int unseen = 0;
        for (int j = 0; j < 8; j++) {
            int dx = BaseOffsetX[j];
            int dy = BaseOffsetY[j];
            int nx = wrap(x + dx);
            int ny = y + dy;
            if (on_map(nx, ny) && !mapsq(nx, ny)->is_visible(faction_id)) {
                unseen++;
                if (!dx || !dy) {
                    unseen++;
                }
            }
        }
        if (unseen < ((radar || is_ocean(sq)) ? 6 : 4)) {
            if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)
            && !map_is_known(faction_id)) {
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
            if (faction_id) {
                sq->visible_items[faction_id - 1] = sq->items;
            }
            sq->visibility |= faction_bit;
            int base_id = base_at(x, y);
            if (base_id >= 0) {
                spot_base(base_id, faction_id);
            }
            draw_range = max(draw_range, radar + 2);
        }
    }
    for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
        if (Vehs[i].faction_id != faction_id) {
            spot_stack(i, Vehs[i].faction_id);
        }
    }
    for (int i = 0; i < 9; i++) {
        int x = wrap(veh_x + TableOffsetX[i]);
        int y = veh_y + TableOffsetY[i];
        if (!on_map(x, y)) {
            continue;
        }
        MAP* sq = mapsq(x, y);
        spot_sensor(x, y, sq);
        if (!i) {
            continue;
        }
        int base_id = base_at(x, y);
        if (base_id >= 0) {
            int base_faction = Bases[base_id].faction_id;
            if (faction_id != base_faction) {
                if (triad != TRIAD_AIR || !veh->range()) {
                    if (!(on_ocean || stealth) || !(plr->diplo_status[base_faction] & DIPLO_COMMLINK)) {
                        encounter_mask |= 1 << base_faction;
                    }
                }
                if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)) {
                    if (Bases[base_id].visibility & faction_bit) {
                        if (Bases[base_id].factions_pop_size_intel[faction_id] != Bases[base_id].pop_size) {
                            draw_range = max(draw_range, 1);
                        }
                    } else {
                        draw_range = max(draw_range, 2);
                    }
                }
                spot_base(base_id, faction_id);
                if (!stealth || !(plr->diplo_status[base_faction] & DIPLO_COMMLINK)) {
                    spot_stack(veh_id, base_faction);
                }
                if (triad == TRIAD_LAND
                && !(Factions[base_faction].diplo_status[faction_id] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))
                && flag) {
                    add_goal(base_faction, AI_GOAL_ATTACK, 2, x, y, -1);
                    add_goal(base_faction, AI_GOAL_DEFEND, 1, x, y, -1);
                }
            }
        }
        bool target_ocean = is_ocean(sq);
        if (has_abil(veh->unit_id, ABL_DEEP_PRESSURE_HULL) && !target_ocean) {
            continue;
        }
        int other_veh_id = stack_fix(veh_at(x, y));
        if (other_veh_id < 0) {
            continue;
        }
        VEH* other = &Vehs[other_veh_id];
        int other_faction = other->faction_id;
        if (other_faction == faction_id) {
            continue;
        }
        bool skip_contact = stealth && !has_abil(other->unit_id, ABL_DEEP_RADAR)
            && (plr->diplo_status[other_faction] & DIPLO_COMMLINK);
        if (!skip_contact) {
            if (base_id < 0) {
                spot_stack(veh_id, other_faction);
            }
            if (!(plr->diplo_status[other_faction] & DIPLO_PACT) && flag) {
                wake_stack(other_veh_id);
                bool wake_other;
                if (is_human(other_faction)) {
                    wake_other = on_ocean == target_ocean
                        && want_to_wake(faction_id, other_veh_id, veh_id);
                } else {
                    wake_other = !(plr->diplo_status[other_faction] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))
                        && (target_ocean || !on_ocean);
                }
                if (wake_other) {
                    clear_alert(other);
                }
            }
        }
        if ((!has_abil(other->unit_id, ABL_DEEP_PRESSURE_HULL)
        && !has_abil(other->unit_id, ABL_CLOAKED)
        && !(other->flags & VFLAG_INVISIBLE)
        && (!mapsq(other->x, other->y)->is_fungus() || other->unit_id == BSC_FUNGAL_TOWER))
        || radar
        || stack_check(other_veh_id, 1, -1, -1, -1) != 1
        || !(plr->diplo_status[other->faction_id] & DIPLO_COMMLINK)) {
            if (base_id < 0) {
                if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)
                && !other->is_visible(faction_id) && draw_range <= 1) {
                    draw_range = 1;
                }
                spot_stack(other_veh_id, faction_id);
            }
            bool wake_own;
            if (is_human(faction_id)) {
                wake_own = (on_ocean == target_ocean || triad == TRIAD_AIR)
                    && want_to_wake(other_faction, veh_id, other_veh_id);
            } else {
                wake_own = !(plr->diplo_status[other_faction] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))
                    && (on_ocean || !target_ocean || triad == TRIAD_AIR);
            }
            if (wake_own) {
                clear_alert(veh);
            }
            if (target_ocean && other_faction
            && !(plr->diplo_status[other_faction] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
                add_goal(faction_id, AI_GOAL_UNK_1,
                    has_abil(other->unit_id, ABL_DEEP_PRESSURE_HULL|ABL_CLOAKED) ? 3 : 2, x, y, -1);
            }
            if (!(plr->diplo_status[other_faction] & (DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT))) {
                add_goal(faction_id, AI_GOAL_ATTACK, faction_id ? 3 : 1, x, y, -1);
            }
            if (veh->plan() >= PLAN_COLONY
            && !(plr->diplo_status[other_faction] & DIPLO_PACT)) {
                if (whose_territory(other_faction, x, y, 0, 0) == other_faction
                && other->plan() >= PLAN_COLONY) {
                    add_goal(other_faction, AI_GOAL_DEFEND, 2, x, y, -1);
                    add_goal(other_faction, AI_GOAL_ATTACK, 1, x, y, -1);
                    add_goal(other_faction, AI_GOAL_SCOUT, 1, x, y, -1);
                }
            }
            if (triad != TRIAD_AIR || !veh->range()) {
                if ((other->triad() != TRIAD_AIR || !other->range())
                && (on_ocean == target_ocean || (base_id >= 0 && !on_ocean)
                || !(plr->diplo_status[other_faction] & DIPLO_COMMLINK))) {
                    if (other->unit_id == BSC_FUNGAL_TOWER) {
                        has_tower = 1;
                    } else if (other->is_native_unit()) { // remove redundant Spore Launcher reference
                        has_native = 1;
                    }
                    encounter_mask |= 1 << other_faction;
                }
            }
        }
    }
    for (int i = 9; i < 25; i++) {
        int x = wrap(veh_x + TableOffsetX[i]);
        int y = veh_y + TableOffsetY[i];
        if (!on_map(x, y)) {
            continue;
        }
        MAP* sq = mapsq(x, y);
        spot_sensor(x, y, sq);
        int tile_owner = sq->anything_at();
        if (tile_owner < 0 || tile_owner == faction_id) { // Fix: added bounds checking
            continue;
        }
        int other_veh_id = stack_fix(veh_at(x, y));
        int base_id = base_at(x, y);
        if (radar && (triad == TRIAD_AIR || is_ocean(sq) == (triad == TRIAD_SEA))) {
            if (base_id >= 0) {
                if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)) {
                    if (Bases[base_id].visibility & faction_bit) {
                        if (Bases[base_id].factions_pop_size_intel[faction_id] != Bases[base_id].pop_size) {
                            draw_range = max(draw_range, 2);
                        }
                    } else {
                        draw_range = max(draw_range, 3);
                    }
                }
                spot_base(base_id, faction_id);
            }
            if (other_veh_id >= 0 && base_id < 0) {
                VEH* other = &Vehs[other_veh_id];
                if (!has_abil(other->unit_id, ABL_DEEP_PRESSURE_HULL)
                && !has_abil(other->unit_id, ABL_CLOAKED)
                && !(other->flags & VFLAG_INVISIBLE)
                && (!mapsq(other->x, other->y)->is_fungus() || other->unit_id == BSC_FUNGAL_TOWER)) {
                    // Fix: original checked visibility of the spotting unit instead of the spotted one
                    if (faction_id == MapWin->cOwner && !(*GameState & STATE_OMNISCIENT_VIEW)
                    && !other->is_visible(faction_id) && draw_range <= 2) {
                        draw_range = 2;
                    }
                    spot_stack(other_veh_id, faction_id);
                    if (is_ocean(sq) && tile_owner
                    && !(plr->diplo_status[tile_owner] & DIPLO_TREATY)) {
                        add_goal(faction_id, AI_GOAL_UNK_1, 2, x, y, -1);
                    }
                }
            }
        }
        if (other_veh_id >= 0 && !stealth) {
            VEH* other = &Vehs[other_veh_id];
            if (has_abil(other->unit_id, ABL_DEEP_RADAR)) {
                int other_triad = other->triad();
                if (other_triad == TRIAD_AIR || on_ocean == (other_triad == TRIAD_SEA)) {
                    if (!(plr->diplo_status[tile_owner] & DIPLO_PACT) && flag) {
                        wake_stack(other_veh_id);
                    }
                    if (veh_sq->base_who() < 0) {
                        spot_stack(veh_id, tile_owner);
                    }
                    if (!is_ocean(sq) && tile_owner
                    && !(Factions[tile_owner].diplo_status[faction_id] & DIPLO_TREATY)) {
                        add_goal(tile_owner, AI_GOAL_UNK_1, 2, veh_x, veh_y, -1);
                    }
                }
            }
        }
        if (base_id < 0) {
            continue;
        }
        if (!(Factions[tile_owner].player_flags & PFLAG_MAP_REVEALED)
        && !veh_sq->is_visible(tile_owner)) {
            continue;
        }
        if (stealth) {
            continue;
        }
        if (on_ocean != is_ocean(sq) || veh_sq->region == sq->region) {
            spot_stack(veh_id, tile_owner);
            if (triad == TRIAD_LAND && veh_sq->region == sq->region) {
                if (x == Factions[tile_owner].target_x && y == Factions[tile_owner].target_y) {
                    Factions[tile_owner].target_y = -1;
                    Factions[tile_owner].target_x = -1;
                    for (int k = veh_at(x, y); k >= 0; k = Vehs[k].next_veh_id_stack) {
                        if (Vehs[k].state & VSTATE_UNK_40000) {
                            Vehs[k].state |= VSTATE_UNK_40000|VSTATE_UNK_20000;
                        }
                    }
                }
            }
            if (veh->plan() == PLAN_PROBE
            && !(plr->diplo_status[tile_owner] & DIPLO_PACT)) {
                add_goal(tile_owner, AI_GOAL_ATTACK, 2, veh_x, veh_y, -1);
                add_goal(tile_owner, AI_GOAL_DEFEND, 1, veh_x, veh_y, -1);
                add_goal(tile_owner, AI_GOAL_SCOUT, 1, veh_x, veh_y, -1);
            }
        }
    }
    if (*CurrentTurn) {
        draw_radius(veh_x, veh_y, faction_id != MapWin->cOwner ? 0 : draw_range, 2);
        if (faction_id == MapWin->cOwner && draw_range) {
            GraphicWin_redraw(WorldWin);
        }
        if (nexus_count > 8 && faction_id == MapWin->cOwner && MFactions[faction_id].is_alien()) {
            interlude(41, 0, 1, 0);
        }
        if (encounter_mask & 1) {
            encounter(faction_id, 0, veh_x, veh_y, has_tower, has_native);
        }
        if (faction_id && flag) {
            for (int i = 1; i < MaxPlayerNum; i++) {
                if (i != faction_id && (encounter_mask & (1 << i))) {
                    encounter(faction_id, i, veh_x, veh_y, 0, 0);
                }
            }
        }
    }
}

} // namespace engine
} // namespace alpha
