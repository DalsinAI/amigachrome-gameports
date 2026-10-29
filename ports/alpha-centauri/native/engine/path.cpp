/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 path search and zones of control. TileSearch, zoc_*, path_get_next,
 * path_cost and the cargo helpers are ported from Thinker Mod src/path.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 *
 * Path_find and Path_move replace terranx Path::find (0x59A530) and
 * Path::move (0x59BC10), which no upstream has decompiled. They are new code:
 * a least-cost search (Dijkstra) over the 8 BaseOffset directions using
 * mod_hex_cost, with the movement rules order_veh enforces (triad, foreign
 * units and bases, zones of control). See ENGINE-WP3.md for how this differs
 * from the original. The AI search helpers of Thinker path.cpp that depend on
 * the AI move tables (defend_tile, safe_path, search_*, route_dist,
 * update_move_path, contiguous, ...) belong to WP6.
 */
#include "engine.h"
#include "veh_action.h"

#include <queue>

namespace alpha {
namespace engine {

void TileSearch::reset() {
    type = 0;
    head = 0;
    tail = 0;
    dist = 0;
    current = 0;
    y_skip = 0;
    faction_id = -1;
    oldtiles.clear();
}

void TileSearch::add_start(int x, int y) {
    assert(type >= 0 && type <= MaxTileSearchType);
    if (tail < QueueSize && mapsq(x, y) && !oldtiles.count({x, y})) {
        paths[tail] = {x, y, 0, -1};
        oldtiles.insert({x, y});
        tail++;
    }
}

void TileSearch::init(int x, int y, int ts_type) {
    reset();
    type = ts_type;
    add_start(x, y);
}

void TileSearch::init(int x, int y, int ts_type, int ts_skip) {
    reset();
    type = ts_type;
    y_skip = ts_skip;
    add_start(x, y);
}

void TileSearch::init(const PointList& points, TSType ts_type, int ts_skip) {
    reset();
    type = ts_type;
    y_skip = ts_skip;
    for (auto& p : points) {
        add_start(p.x, p.y);
    }
}

void TileSearch::get_route(PointList& pp) {
    std::vector<int> route;
    pp.clear();
    for (int p = current, i = 0; p >= 0 && i <= PathLimit; ++i) {
        route.push_back(p);
        p = paths[p].prev;
    }
    for (auto it = route.rbegin(); it != route.rend(); ++it) {
        pp.push_back({paths[*it].x, paths[*it].y});
    }
    assert((int)pp.size() == dist + 1);
}

bool TileSearch::has_zoc(int plr_id) {
    bool prev_zoc = false;
    for (int p = current, i = 0; p >= 0 && i <= PathLimit; ++i) {
        if (zoc_any(paths[p].x, paths[p].y, plr_id)) {
            if (prev_zoc) return true;
            prev_zoc = true;
        } else {
            prev_zoc = false;
        }
        p = paths[p].prev;
    }
    return false;
}

PathNode& TileSearch::get_prev() {
    int p = paths[current].prev;
    assert(p >= 0);
    return paths[p];
}

PathNode& TileSearch::get_node() {
    return paths[current];
}

MAP* TileSearch::get_next() {
    while (head < tail) {
        rx = paths[head].x;
        ry = paths[head].y;
        dist = paths[head].dist;
        current = head;
        head++;
        if (!(sq = mapsq(rx, ry))) {
            continue;
        }
        if (!current && faction_id < 0) {
            faction_id = sq->veh_owner();
            if (faction_id < 0) {
                faction_id = sq->owner;
            }
        }
        bool foreign = sq->is_owned() && sq->owner != faction_id && !has_pact(faction_id, sq->owner);
        bool our_base = sq->is_base() && !foreign;
        bool skip = (sq->is_base() && !our_base)
            || (type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq) && !our_base)
            || (type == TS_SEA_AND_SHORE && !is_ocean(sq) && !our_base)
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_SHORE && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_PACT && (is_ocean(sq) || foreign));
        if (dist > 0 && skip) {
            if ((type == TS_TRIAD_LAND && is_ocean(sq))
            || (type == TS_TRIAD_SEA && !is_ocean(sq))
            || (type == TS_TERRITORY_LAND && (is_ocean(sq) || sq->owner != faction_id))
            || (type == TS_TERRITORY_PACT && foreign)) {
                continue;
            }
            return sq;
        }
        for (const auto& p : NearbyTiles) {
            int x2 = wrap(rx + p[0]);
            int y2 = ry + p[1];
            if (y2 >= y_skip && y2 < *MapAreaY - y_skip
            && x2 >= 0 && x2 < *MapAreaX
            && tail < QueueSize && dist < PathLimit
            && !oldtiles.count({x2, y2})) {
                paths[tail] = {x2, y2, dist+1, current};
                tail++;
                oldtiles.insert({x2, y2});
            }
        }
        if (dist > 0) {
            return sq;
        }
    }
    return NULL;
}

int __cdecl zoc_any(int x, int y, int faction_id) {
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->anything_at();
            if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner)) {
                return owner + 1;
            }
        }
    }
    return 0;
}

int __cdecl zoc_veh(int x, int y, int faction_id) {
    int value = 0;
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->veh_who();
            if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner)) {
                owner++;
                if (value <= owner) {
                    value = owner;
                }
            }
        }
    }
    return value;
}

int __cdecl zoc_sea(int x, int y, int faction_id) {
    bool sea_tile = is_ocean(mapsq(x, y));
    for (const auto& p : NearbyTiles) {
        int x2 = wrap(x + p[0]);
        int y2 = y + p[1];
        MAP* sq = mapsq(x2, y2);
        if (sq) {
            int owner = sq->veh_who();
            if (owner >= 0 && owner != faction_id && is_ocean(sq) == sea_tile
            && !has_pact(faction_id, owner)) {
                for (int k = veh_at(x2, y2); k >= 0; k = Vehs[k].next_veh_id_stack) {
                    if (Vehs[k].faction_id != faction_id
                    && (Vehs[k].visibility & (1 << faction_id)
                    || (!*MultiplayerActive && !(Vehs[k].flags & VFLAG_INVISIBLE)))) {
                        return owner + 1;
                    }
                }
            }
        }
    }
    return 0;
}

int __cdecl zoc_move(int x, int y, int faction_id) {
    MAP* sq = mapsq(x, y);
    if (sq && !sq->is_base()) {
        return zoc_sea(x, y, faction_id);
    }
    return 0;
}

int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id) {
    return Path_find(x1, y1, x2, y2, unit_id, faction_id, 0, -1);
}

int path_cost(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int max_cost) {
    int px = x1;
    int py = y1;
    int cost = 0;
    int prev_cost = 0;
    int i = 0;

    while (cost + prev_cost <= max_cost) {
        cost += prev_cost;
        if (px == x2 && py == y2) {
            return cost;
        }
        if (++i > PathLimit) {
            return -1;
        }
        int val = Path_find(px, py, x2, y2, unit_id, faction_id, 0, -1);
        if (!(val >= 0 && val < 8)) {
            return -1;
        }
        int rx = px;
        int ry = py;
        px = wrap(px + BaseOffsetX[val]);
        py = py + BaseOffsetY[val];
        prev_cost = mod_hex_cost(unit_id, faction_id, rx, ry, px, py, 0);
    }
    return -1;
}

int veh_cargo_loaded(int veh_id) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->order == ORDER_SENTRY_BOARD && veh->waypoint_x[0] == veh_id
        && veh->x == Vehs[veh_id].x && veh->y == Vehs[veh_id].y) {
            assert(veh_id != i);
            num++;
        }
    }
    return num;
}

int cargo_capacity(int x, int y, int faction_id) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y && veh->is_transport()
        && veh->faction_id == faction_id) {
            num += veh_cargo(i) - veh_cargo_loaded(i);
        }
    }
    return num;
}

bool has_transport(int x, int y, int faction_id) {
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y && veh->is_transport()
        && veh->faction_id == faction_id) {
            return true;
        }
    }
    return false;
}

/*
New code (replaces terranx Path::find). Movement rules for one step from
(x1, y1) to the adjacent tile (x2, y2):
- land units stay on land, except into a base of their own or a pact faction;
- sea units stay on ocean, except into such a base;
- air units go anywhere;
- tiles with units or bases of other factions without a pact are blocked,
  except the destination itself (order_veh then attacks or captures);
- a land step between two tiles that are both in an enemy zone of control is
  blocked (order_veh rule) unless the target holds a friendly unit, or the
  search ignores zones of control (PATH_IGNORE_ZOC, faction_id < 0);
- PATH_AVOID_FUNGUS skips fungus tiles (road building without the fungus
  road technology).
*/
static bool path_step_allowed(int triad, int faction_id, int flags, int x1, int y1,
MAP* sq1, int x2, int y2, MAP* sq2, bool is_target) {
    if (!sq2) {
        return false;
    }
    bool ocean2 = is_ocean(sq2);
    int base_owner = sq2->base_who();
    bool friendly_base = base_owner >= 0 && faction_id >= 0
        && (base_owner == faction_id || has_pact(faction_id, base_owner));
    if (faction_id < 0 && base_owner >= 0) {
        friendly_base = true;
    }
    if (triad == TRIAD_LAND && ocean2 && !friendly_base) {
        return false;
    }
    if (triad == TRIAD_SEA && !ocean2 && !friendly_base) {
        return false;
    }
    if ((flags & PATH_AVOID_FUNGUS) && sq2->is_fungus() && !(sq2->items & BIT_ROAD)) {
        return false;
    }
    if (faction_id < 0) {
        return true;
    }
    int owner = sq2->anything_at();
    if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner) && !is_target) {
        return false;
    }
    if (triad == TRIAD_LAND && !(flags & PATH_IGNORE_ZOC) && sq1
    && !is_ocean(sq1) && !ocean2 && !sq2->is_base()) {
        int stack_owner = sq2->veh_who();
        if (stack_owner != faction_id
        && zoc_move(x1, y1, faction_id) && zoc_move(x2, y2, faction_id)) {
            return false;
        }
    }
    return true;
}

int __cdecl Path_find(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int flags, int UNUSED(unk)) {
    MAP* src = mapsq(x1, y1);
    MAP* dst = mapsq(x2, y2);
    if (!src || !dst || (x1 == x2 && y1 == y2) || !*MapTiles) {
        return -1;
    }
    if (unit_id < 0 || unit_id >= MaxProtoNum) {
        unit_id = BSC_COLONY_POD;
    }
    const int triad = Units[unit_id].triad();
    const int tiles = *MapAreaTiles;
    const int cost_faction = (faction_id < 0 ? 0 : faction_id);
    // Costs are scaled by 16 plus one per step, so equal-cost routes prefer
    // fewer tiles and zero-cost mag tube steps still make progress.
    std::vector<int> best(tiles, INT_MAX);
    std::vector<int> first(tiles, -1);
    typedef std::pair<int, int> Item; // (cost, tile index)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
    MAP* base = *MapTiles;
    auto tile_xy = [&](int i, int* x, int* y) {
        *y = (2 * i) / *MapAreaX;
        *x = 2 * i - *y * *MapAreaX;
        if ((*x + *y) & 1) {
            (*x)++;
        }
    };
    int src_i = (int)(src - base);
    int dst_i = (int)(dst - base);
    best[src_i] = 0;
    queue.push({0, src_i});
    int expanded = 0;
    const int limit = min(tiles, 64 * 1024);
    while (!queue.empty()) {
        Item cur = queue.top();
        queue.pop();
        int i = cur.second;
        if (cur.first != best[i]) {
            continue;
        }
        if (i == dst_i) {
            return first[i];
        }
        if (++expanded > limit) {
            break;
        }
        int x, y;
        tile_xy(i, &x, &y);
        MAP* sq = base + i;
        // Do not route through the destination of an attack or other blocked tiles.
        if (i != src_i && sq->anything_at() >= 0 && faction_id >= 0
        && sq->anything_at() != faction_id && !has_pact(faction_id, sq->anything_at())) {
            continue;
        }
        for (int d = 0; d < 8; d++) {
            int nx = wrap(x + BaseOffsetX[d]);
            int ny = y + BaseOffsetY[d];
            MAP* nsq = mapsq(nx, ny);
            if (!nsq) {
                continue;
            }
            int ni = (int)(nsq - base);
            if (!path_step_allowed(triad, faction_id, flags, x, y, sq, nx, ny, nsq, ni == dst_i)) {
                continue;
            }
            int step = mod_hex_cost(unit_id, cost_faction, x, y, nx, ny, 0);
            int cost = cur.first + 16 * max(0, step) + 1;
            if (cost < best[ni]) {
                best[ni] = cost;
                first[ni] = (i == src_i ? d : first[i]);
                queue.push({cost, ni});
            }
        }
    }
    return -1;
}

/*
New code (replaces terranx Path::move): next step of a vehicle with a
go-to order towards waypoint 0. faction_id < 0 (ORDER_AI_GO_TO) ignores zones
of control like the original's AI go-to.
*/
int __cdecl Path_move(int veh_id, int faction_id) {
    VEH* veh = &Vehs[veh_id];
    int tx = veh->waypoint_x[0];
    int ty = veh->waypoint_y[0];
    if (!on_map(tx, ty) || !mapsq(tx, ty) || (veh->x == tx && veh->y == ty)) {
        return -1;
    }
    int flags = (faction_id < 0 ? PATH_IGNORE_ZOC : 0);
    return Path_find(veh->x, veh->y, tx, ty, veh->unit_id, veh->faction_id, flags, -1);
}

} // namespace engine
} // namespace alpha
