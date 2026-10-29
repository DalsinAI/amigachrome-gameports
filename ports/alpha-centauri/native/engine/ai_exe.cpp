/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: replacements for terranx.exe routines that the
 * ported Thinker AI calls through function pointers.
 * garrison_check, base_territory, planet_buster, abstract_at and
 * Path::territory follow OpenSMACX (2f7f4e24, GPL-3.0-or-later, Copyright (C)
 * 2013-2021 Brendan Casey: src/base.cpp, src/map.cpp, src/veh.cpp,
 * src/path.cpp). world_site, retire_proto, set_course and the route check
 * replacing Path::find in contiguous() are new code: OpenSMACX has no body
 * for them. Every approximation is listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {
namespace ai {

/*
Determine ideal non-offense (defense, combat, recon) unit count for the base.
Original Offset: 00560B30. Return Value: 1..10
*/
int __cdecl garrison_check(int base_id) {
    int x = Bases[base_id].x;
    int y = Bases[base_id].y;
    int faction_id = Bases[base_id].faction_id;
    int sp_count = 0;
    for (int i = SP_ID_First; i <= SP_ID_Last; i++) {
        if (project_base((FacilityId)i) == base_id) {
            sp_count++;
        }
    }
    int garrison = (sp_count + 2) / 3 + (Bases[base_id].pop_size + 1) / 4 + 1;
    bool is_obj = is_objective(base_id);
    if (has_fac_built(FAC_HEADQUARTERS, base_id) || (bit_at(x, y) & BIT_DOUBLE_SEA) || is_obj) {
        garrison++;
    }
    if (is_obj && Factions[faction_id].player_flags & PFLAG_STRAT_DEF_OBJECTIVES) {
        garrison++;
    }
    if (Factions[faction_id].region_base_plan[region_at(x, y)] == PLAN_COLONY) {
        garrison--;
    }
    int faction_id_sea = zoc_sea(x, y, faction_id) - 1;
    if (faction_id_sea > 0
    && (has_treaty(faction_id, faction_id_sea, DIPLO_VENDETTA | DIPLO_WANT_REVENGE)
    || has_treaty(faction_id_sea, faction_id, DIPLO_WANT_REVENGE)
    || Factions[faction_id_sea].integrity_blemishes >= 4)) {
        garrison++;
    }
    int support = Factions[faction_id].SE_support_pending;
    if (support <= -4) {
        garrison--;
    }
    if (support <= -3 && garrison > 2) {
        garrison--;
    }
    return clamp(garrison, 1, 10);
}

/*
Closest base to territory owned by another faction not at war with the faction.
Original Offset: 004E3FA0. Return Value: base id or -1
*/
int __cdecl base_territory(int faction_id, int x, int y) {
    int base_id = -1;
    int owner = whose_territory(faction_id, x, y, &base_id, false);
    if (owner >= 0 && owner != faction_id && (is_human(faction_id) || is_human(owner))
    && !has_treaty(faction_id, owner, DIPLO_VENDETTA)) {
        return base_id;
    }
    return -1;
}

/*
Original Offset: 00500520. Return Value: reactor id if planet buster, otherwise 0
*/
int __cdecl planet_buster(int veh_id) {
    return Units[Vehs[veh_id].unit_id].is_planet_buster();
}

/*
Region of an abstract (5x5) map cell. Original Offset: 00591210
*/
int __cdecl abstract_at(int x, int y) {
    // Fix: bounds check (the original indexes the table unchecked).
    int i = (x >> 1) + y * (*MapAbstractX >> 1);
    if (!*MapAbstract || x < 0 || y < 0 || i < 0 || i >= *MapAbstractCount) {
        return 0;
    }
    return (*MapAbstract)[i];
}

/*
Colonization priority of a tile for the original AI goal assignment
(Original Offset: 005C4FD0, not decompiled anywhere). New code: rates the
tile with Thinker base_tile_score (Thinker controls colony pods with the same
score) and maps it to the 1..15 range that site_set stores in val2.
*/
int __cdecl world_site(int x, int y, int UNUSED(toggle)) {
    MAP* sq = mapsq(x, y);
    if (!sq || is_ocean(sq)) {
        return 0;
    }
    int score = base_tile_score(x, y, 0, sq);
    return clamp(1 + score / 8, 1, 15);
}

/*
Mark a prototype obsolete for the faction (Original Offset: 0057F1D0, not
decompiled anywhere). New code: sets the faction bit in obsolete_factions,
which is what the build and design code checks.
*/
void __cdecl retire_proto(int unit_id, int faction_id) {
    if (unit_id >= 0 && unit_id < MaxProtoNum && faction_id > 0 && faction_id < MaxPlayerNum) {
        Units[unit_id].obsolete_factions |= (1 << faction_id);
    }
}

/*
Native transport course to a target (Original Offset: 00564890, not
decompiled anywhere). New code: a plain go-to order on waypoint 0, which the
caller then inspects exactly like the original result.
*/
void __cdecl set_course(int veh_id, int UNUSED(type), int x, int y) {
    VEH* veh = &Vehs[veh_id];
    if (!on_map(x, y) || (veh->x == x && veh->y == y)) {
        veh->order = ORDER_NONE;
        return;
    }
    veh->waypoint_x[0] = x;
    veh->waypoint_y[0] = y;
    veh->order = ORDER_MOVE_TO;
    veh->status_icon = 'G';
}

/*
Mark (MAP::unk_1) connected land tiles owned by the faction, radiating from
the coordinates. Path::territory, Original Offset: 0059C3C0 (OpenSMACX).
*/
void Path_territory(int x, int y, int UNUSED(region), int faction_id) {
    if (is_ocean(x, y)) {
        return; // skip ocean terrain
    }
    std::vector<Point> queue;
    queue.push_back({x, y});
    size_t head = 0;
    while (head < queue.size()) {
        Point p = queue[head++];
        for (int i = 0; i < 8; i++) {
            int x2 = wrap(p.x + BaseOffsetX[i]);
            int y2 = p.y + BaseOffsetY[i];
            MAP* tile;
            if (on_map(x2, y2) && p.y && p.y != *MapAreaY - 1
            && !is_ocean(x2, y2) && (tile = mapsq(x2, y2)) != NULL
            && !tile->unk_1 && tile->owner == faction_id) {
                tile->unk_1 = 1;
                queue.push_back({x2, y2});
            }
        }
    }
}

/*
Replaces the Path::find call in contiguous() (flags 0x201, or 0xC01 for any
contact). New code: breadth-first search over land tiles from (x1, y1) to
(x2, y2) through tiles marked by Path_territory, or through any land tile of
the same region when any_land is set.
*/
bool territory_route(int x1, int y1, int x2, int y2, bool any_land) {
    MAP* start = mapsq(x1, y1);
    if (!start || !mapsq(x2, y2)) {
        return false;
    }
    int region = start->region;
    std::vector<Point> queue;
    std::vector<uint8_t> seen((size_t)*MapAreaTiles, 0);
    queue.push_back({x1, y1});
    seen[(size_t)(x1 / 2 + y1 * (*MapAreaX / 2))] = 1;
    size_t head = 0;
    while (head < queue.size()) {
        Point p = queue[head++];
        if (p.x == x2 && p.y == y2) {
            return true;
        }
        for (int i = 0; i < 8; i++) {
            int nx = wrap(p.x + BaseOffsetX[i]);
            int ny = p.y + BaseOffsetY[i];
            MAP* sq = mapsq(nx, ny);
            if (!sq || is_ocean(sq) || sq->region != region) {
                continue;
            }
            size_t idx = (size_t)(nx / 2 + ny * (*MapAreaX / 2));
            if (seen[idx]) {
                continue;
            }
            bool target = (nx == x2 && ny == y2);
            if (!target && !any_land && !sq->unk_1) {
                continue;
            }
            seen[idx] = 1;
            queue.push_back({nx, ny});
        }
    }
    return false;
}

void draw_tiles(int x, int y, int flag) {
    draw_tile(x, y, flag); // Original: redraws a 3x3 block; the event sink gets the centre
}

/*
Is the base within one tile of a sea tile of the given region?
Original Offset: 0050DE50 (OpenSMACX src/map.cpp).
*/
int __cdecl base_on_sea(int base_id, int region_sea) {
    region_sea &= 63; // RegionBounds
    if (region_sea >= 63) {
        return false; // skips poles (land or ocean)
    }
    int x = Bases[base_id].x;
    int y = Bases[base_id].y;
    for (int i = 0; i < 8; i++) {
        int x2 = wrap(x + BaseOffsetX[i]);
        int y2 = y + BaseOffsetY[i];
        if (on_map(x2, y2) && is_ocean(x2, y2) && (region_at(x2, y2) & 63) == region_sea) {
            return true;
        }
    }
    return false;
}

int __cdecl POP2(const char* label, const char* pcx_filename, int UNUSED(a3)) {
    return popp(ScriptFile, label, 0, pcx_filename, 0); // Original: exe popup wrapper (0x405140)
}

/* Original: MapWin::focus/Console::focus centre the map view and return
 * whether the tile was already visible. Without a map view nothing is
 * focused, so callers redraw the tile through the event sink instead. */
int MapWin_focus(MapWindow* UNUSED(map_win), int UNUSED(x), int UNUSED(y)) {
    return 0;
}

int Console_focus(Console* UNUSED(console), int UNUSED(x), int UNUSED(y), int UNUSED(faction_id)) {
    return 0;
}

} // namespace ai
} // namespace engine
} // namespace alpha
