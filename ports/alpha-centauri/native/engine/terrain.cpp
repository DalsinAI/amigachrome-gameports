/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 terrain: altitude changes (terraform raise/lower, quakes), movement
 * cost, rockiness, resource bonus and supply pod placement, ported from
 * Thinker Mod src/map.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: presentation calls go to veh_action_ui.h; the DEBUG cross-check
 * against the executable's hex_cost is dropped; delete_landmark (exe-only)
 * is new code. Deliberate changes are marked "Fix:".
 */
#include "engine.h"
#include "veh_action.h"
#include "veh_action_ui.h"

namespace alpha {
namespace engine {

using namespace wp3_ui;

/* Thinker map.cpp mapgen_rand (static there; WP7 keeps its own copy). */
static int mapgen_rand(int value) {
    if (*MultiplayerActive) {
        return game_randv(value);
    }
    return (value > 1 ? map_rand.get(value) : 0);
}

/*
New code: terranx delete_landmark (0x5927D0) has no decompiled source. It is
called by alt_set when a tile changes altitude; this version removes a
landmark anchored exactly on that tile, so a raised or lowered named feature
loses its label. Landmarks anchored elsewhere are kept.
*/
void __cdecl delete_landmark(int x, int y) {
    for (int i = 0; i < *MapLandmarkCount; i++) {
        if (Landmarks[i].x == x && Landmarks[i].y == y) {
            if (i < *MapLandmarkCount - 1) {
                memmove(&Landmarks[i], &Landmarks[i + 1],
                    sizeof(Landmark) * (*MapLandmarkCount - i - 1));
            }
            *MapLandmarkCount -= 1;
            return;
        }
    }
}

void __cdecl alt_set(int x, int y, int altitude) {
    MAP* sq = mapsq(x, y);
    assert(sq && altitude >= 0 && altitude <= 7);
    int alt = sq->alt_level();
    int is_sea = altitude < ALT_SHORE_LINE;
    auto set_alt = [&]() {
        sq->climate = (altitude * 32) | (sq->climate & 0x1F);
        *GameDrawState |= 4;
    };
    if (!*CurrentTurn) {
        return set_alt();
    }
    if (altitude != alt) {
        delete_landmark(x, y);
    }
    if ((alt < ALT_SHORE_LINE) != is_sea) {
        sq->items &= ~(BIT_SENSOR|BIT_THERMAL_BORE|BIT_ECH_MIRROR|BIT_CONDENSER|BIT_FOREST|\
            BIT_SOIL_ENRICHER|BIT_AIRBASE|BIT_FARM|BIT_BUNKER|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD);
        for (int i = 1; i < MaxPlayerNum; i++) {
            sq->visible_items[i - 1] = sq->items;
        }
        if (*WorldSkipTerritory) {
            return set_alt();
        }
        int base_id = base_at(x, y);
        if (base_id >= 0) {
            BASE* base = &Bases[base_id];
            const bool is_player = base->faction_id == *CurrentPlayerFaction;
            const bool is_visible = base->visibility & (1 << *CurrentPlayerFaction);
            if (!is_sea || has_fac_built(FAC_PRESSURE_DOME, base_id)) {
                return set_alt();
            }
            // Fix: skip rendering pop_size as negative, zero pop_size deletes the base
            base->pop_size = max(0, base->pop_size
                - clamp(base->pop_size / 2, mapgen_rand(3) + 2, 10));
            set_fac(FAC_PRESSURE_DOME, base_id, 1);
            Factions[base->faction_id].player_flags |= PFLAG_UNK_80000;
            MapBaseSubmergedCount[base->faction_id]++;
            if (is_objective(base_id) && base->pop_size < 1) {
                base->pop_size = 1;
            }
            if (base->pop_size > 0) {
                return set_alt();
            }
            if (Factions[base->faction_id].base_count <= 1 || (*MultiplayerActive && !*ControlTurnC)) {
                base->pop_size = 1;
                if (!*VehBattleState && (is_player || is_visible)) {
                    Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                    parse_says(0, &base->name[0], -1, -1);
                    if (is_player) {
                        X_pop_2("BASESUBMERGED", "subbase_sm.pcx", 0);
                    } else {
                        parse_says(1, &MFactions[base->faction_id].adj_name_faction[0], -1, -1);
                        X_pop_2("BASESUBMERGED2", "subbase_sm.pcx", 0);
                    }
                }
                return set_alt();
            }
            if (!*VehBattleState && (is_player || is_visible)) {
                Console_focus(MapWin, base->x, base->y, *CurrentPlayerFaction);
                parse_says(0, &base->name[0], -1, -1);
                if (is_player) {
                    popp(ScriptFile, "BASESUBMERGED1", 0, "subbase_sm.pcx", 0);
                } else {
                    parse_says(1, &MFactions[base->faction_id].adj_name_faction[0], -1, -1);
                    popp(ScriptFile, "BASESUBMERGED3", 0, "subbase_sm.pcx", 0);
                }
            }
            mod_base_kill(base_id);
        }
        int veh_id = stack_fix(veh_at(x, y));
        while (veh_id >= 0) {
            VEH* veh = &Vehs[veh_id];
            int next_veh_id = veh->next_veh_id_stack;
            if (veh->triad() != TRIAD_AIR && (veh->triad() == TRIAD_SEA) != is_sea
            && !(veh->flags & VFLAG_IS_OBJECTIVE)) {
                MapBaseIdClosestSubmergedVeh[veh->faction_id] = base_find_2(x, y, veh->faction_id);
                veh_kill(veh_id);
                if (next_veh_id > veh_id) {
                    --next_veh_id;
                }
            }
            veh_id = next_veh_id;
        }
    } else if (altitude >= ALT_SHORE_LINE) {
        int val = sq->val3 >> 6;
        if (altitude <= alt) {
            if (altitude < alt && val > altitude - 3 && mapgen_rand(val + 1)) {
                sq->val3 = (sq->val3 & 0x3F) | ((val + 3) << 6);
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
        } else if (val < 2 && val + 1 <= altitude - 3) {
            if (!mapgen_rand(val + 4)) {
                sq->val3 = (sq->val3 & 0x3F) | ((val + 1) << 6);
                sq->landmarks |= LM_UNK_400000;
                *GameDrawState |= 1;
            }
        }
        assert((sq->val3 & 0xC0) != 0xC0); // Both rocky bits must not be set
    }
    set_alt(); // Always set altitude level
}

void __cdecl alt_set_both(int x, int y, int altitude) {
    alt_set(x, y, altitude);
    if (alt_natural(x, y) != altitude) {
        int offset = AltNatural[altitude + 1] - AltNatural[altitude];
        alt_put_detail(x, y,
            (uint8_t)(*MapSeaLevel + AltNatural[altitude] + mapgen_rand(offset)));
    }
}

void __cdecl world_alt_set(int x, int y, int altitude, int toggle) {
    assert(altitude >= 0 && altitude <= 7);
    memset(MapBaseSubmergedCount, 0, MaxPlayerNum * sizeof(int));
    memset(MapBaseIdClosestSubmergedVeh, 0xFF, MaxPlayerNum * sizeof(int));
    if (on_map(x, y)) {
        if (toggle) {
            alt_set_both(x, y, altitude);
        } else {
            alt_set(x, y, altitude);
        }
    }
    MAP* sq;
    int tx = -1;
    int ty = -1;
    if (altitude > 1) {
        int alt = altitude - 1;
        for (int i = 0; i < altitude - 1; ++i, --alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() < alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
    if (altitude <= conf.altitude_limit) {
        int alt = altitude + 1;
        for (int i = 0; i <= conf.altitude_limit - altitude; ++i, ++alt) {
            bool found = false;
            for (int j = TableRange[i]; j < TableRange[i+1]; ++j) {
                if ((sq = next_tile(x, y, j, &tx, &ty)) != NULL && sq->alt_level() > alt) {
                    if (toggle) {
                        alt_set_both(tx, ty, alt);
                        if (sq->anything_at() < 0 && !bonus_at(tx, ty)) {
                            owner_set(tx, ty, -1);
                        }
                    } else {
                        alt_set(tx, ty, alt);
                    }
                    found = true;
                }
            }
            if (!found) {
                break;
            }
        }
    }
}

void __cdecl world_raise_alt(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        int alt = sq->alt_level();
        if (alt < conf.altitude_limit) {
            world_alt_set(x, y, alt + 1, 1);
        }
    } else {
        assert(sq);
    }
}

void __cdecl world_lower_alt(int x, int y) {
    MAP* sq = mapsq(x, y);
    if (sq) {
        int alt = sq->alt_level();
        if (alt > 0) {
            world_alt_set(x, y, alt - 1, 1);
        }
    } else {
        assert(sq);
    }
}

int __cdecl mod_hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle) {
    if (!(unit_id >= 0 && unit_id < MaxProtoNum && faction_id < MaxPlayerNum)) {
        assert(0);
        return 0;
    }
    UNIT* unit = &Units[unit_id];
    MAP* sq_src = mapsq(x1, y1);
    MAP* sq_dst = mapsq(x2, y2);
    uint32_t bit_src = (sq_src ? sq_src->items : 0);
    uint32_t bit_dst = (sq_dst ? sq_dst->items : 0);
    int road_move_rate = (conf.magtube_movement_rate > 0 ? conf.road_movement_rate : 1);
    int cost;
    if (is_ocean(sq_dst)) {
        if (bit_dst & BIT_FUNGUS
        && sq_dst->alt_level() == ALT_OCEAN_SHELF
        && unit->triad() == TRIAD_SEA
        && unit_id != BSC_SEALURK // Fix Sealurk movement
        && unit_id != BSC_ISLE_OF_THE_DEEP
        && !has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
            cost = Rules->move_rate_roads * 3;
        } else {
            cost = Rules->move_rate_roads;
        }
    } else if (is_ocean(sq_src)) {
        cost = Rules->move_rate_roads;
    } else if (unit->triad() != TRIAD_LAND) {
        cost = Rules->move_rate_roads;
    // Land only conditions
    } else if (bit_src & (BIT_MAGTUBE | BIT_BASE_IN_TILE)
    && bit_dst & (BIT_MAGTUBE | BIT_BASE_IN_TILE) && faction_id) {
        cost = (conf.magtube_movement_rate > 0 ? 1 : 0);
    } else if ((bit_src & (BIT_ROAD | BIT_BASE_IN_TILE) || (bit_src & BIT_FUNGUS && faction_id > 0
    && has_project(FAC_XENOEMPATHY_DOME, faction_id))) && bit_dst & (BIT_ROAD | BIT_BASE_IN_TILE)
    && faction_id) {
        cost = road_move_rate;
    } else if (faction_id >= 0 && (has_project(FAC_XENOEMPATHY_DOME, faction_id) || !faction_id
    || unit_id == BSC_MIND_WORMS || unit_id == BSC_SPORE_LAUNCHER) && bit_dst & BIT_FUNGUS) {
        cost = road_move_rate;
    } else if (bit_src & BIT_RIVER && bit_dst & BIT_RIVER && x_dist(x1, x2) == 1
    && abs(y1 - y2) == 1 && faction_id) {
        cost = road_move_rate;
    } else if (unit->chassis_id == CHS_HOVERTANK || has_abil(unit_id, ABL_ANTIGRAV_STRUTS)) {
        cost = Rules->move_rate_roads;
    } else {
        cost = Rules->move_rate_roads;
        if (sq_dst->is_rocky() && !toggle) {
            cost += Rules->move_rate_roads;
        }
        if (bit_dst & BIT_FOREST && !toggle) {
            cost += Rules->move_rate_roads;
        }
        if (faction_id && bit_dst & BIT_FUNGUS && (unit_id >= MaxProtoFactionNum
        || unit->offense_value() >= 0)) {
            int speed_val = proto_speed(unit_id);
            // Fix faction_id check when it is negative
            if (unit->plan != PLAN_TERRAFORM && unit->plan != PLAN_ARTIFACT
            && (faction_id < 0 || Factions[faction_id].SE_planet <= 0)) {
                cost += Rules->move_rate_roads * 2;
            } else {
                cost = max(cost, speed_val);
            }
            if (conf.fast_fungus_movement > 0) {
                cost = min(max(speed_val, Rules->move_rate_roads), cost);
            }
        }
    }
    return cost;
}

int __cdecl mod_minerals_at(int x, int y) {
    if (!y || y == (*MapAreaY - 1)) {
        return 2;
    }
    MAP* sq = mapsq(x, y);
    int alt = sq->alt_level();
    int avg = (x + y) / 2;
    int val1 = (x - avg) / 2 + 2 * ((x - avg) / 2) + 2 * (avg / 2) + *MapRandomSeed;
    int val2 = ((byte)val1 - 2 * (((byte)x - (byte)avg) & 1) - (avg & 1)) & 3;
    int val3 = (alt - 3 >= 0 ? alt - 3 : 3 - alt) - (alt < 3);
    if (val3 == 0) {
        if (val2 == 0) {
            return 1;
        }
        if (val2 == 1 || val2 == 3) {
            return 0;
        }
        if (val2 == 2) {
            return (val1 & 4 ? 2 : 1);
        }
        return ~val2 & 1;
    } else if (val3 == 1) {
        if (!val2) {
            return 0;
        }
        if (!(val2 - 1)) {
            return 1;
        }
        if ((val2 - 1) != 1) {
            return 2;
        }
        return 1;
    } else if (val3 == 2) {
        if (!val2) {
            return 0;
        }
        if (val2 == 1) {
            return 1;
        }
        return 2;
    } else if (val3 == 3) {
        if (val2 < 0 || val2 > 1) {
            return 2;
        }
        return 1;
    } else {
        return ~val2 & 1;
    }
}

int __cdecl mod_bonus_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    bool has_rsc_bonus = bit & BIT_BONUS_RES;
    if (!has_rsc_bonus && (!*MapRandomSeed || (alt >= ALT_SHORE_LINE
    && !conf.rare_supply_pods && !(*GameRules & RULES_NO_UNITY_SCATTERING)))) {
        return 0;
    }
    int avg = (x + y) >> 1;
    x -= avg;
    int chk = (avg & 3) + 4 * (x & 3);
    if (!has_rsc_bonus && chk != ((*MapRandomSeed + (-5 * (avg >> 2)) - 3 * (x >> 2)) & 0xF)) {
        return 0;
    }
    if (alt < ALT_OCEAN_SHELF) {
        return 0;
    }
    int ret = (alt < ALT_SHORE_LINE) ? chk % 3 + 1 : (chk % 5) & 3;
    if (!ret || bit & BIT_NUTRIENT_RES) {
        if (bit & BIT_ENERGY_RES) {
            return 3; // energy
        }
        return ((bit & BIT_MINERAL_RES) != 0) + 1; // nutrient or mineral
    }
    return ret;
}

int __cdecl mod_goody_at(int x, int y) {
    MAP* sq = mapsq(x, y);
    uint32_t bit = sq->items;
    uint32_t alt = sq->alt_level();
    if (bit & (BIT_SUPPLY_REMOVE | BIT_MONOLITH)) {
        return 0; // nothing, supply pod already opened or monolith
    }
    if (*GameRules & RULES_NO_UNITY_SCATTERING) {
        return (bit & (BIT_UNK_4000000 | BIT_UNK_8000000)) ? 2 : 0; // near landing site
    }
    if (bit & BIT_SUPPLY_POD) {
        return 1; // supply pod
    }
    if (!*MapRandomSeed) {
        return 0; // nothing
    }
    int avg = (x + y) >> 1;
    int x_diff = x - avg;
    int cmp = (avg & 3) + 4 * (x_diff & 3);
    if (!conf.rare_supply_pods && alt >= ALT_SHORE_LINE) {
        if (cmp == ((-5 * (avg >> 2) - 3 * (x_diff >> 2) + *MapRandomSeed) & 0xF)) {
            return 2;
        }
    }
    return cmp == ((11 * (avg / 4) + 61 * (x_diff / 4) + *MapRandomSeed + 8) & 0x1F); // 0 or 1
}

} // namespace engine
} // namespace alpha
