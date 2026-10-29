/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3: unit orders, movement, pathfinding, terraforming, supply pods,
 * monoliths and visibility. Declarations beyond the WP3 section of
 * engine_wp.h. Names and signatures follow Thinker Mod src/veh_action.h,
 * src/veh.h, src/path.h and src/map.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE),
 * or the terranx.exe typedefs in Thinker src/engine.h for exe-only routines.
 * Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

/* ---- veh_action.cpp (Thinker veh_action.cpp) ---- */
int __cdecl terrain_avail(FormerItem frm_id, int ocean, int faction_id);
inline bool has_terra(FormerItem frm_id, int ocean, int faction_id) {
    return terrain_avail(frm_id, ocean, faction_id);
}
int __cdecl action(int veh_id);
int __cdecl action_build(int veh_id, const char* name);
void __cdecl action_go_to(int veh_id);
void __cdecl action_road_to(int veh_id);
int __cdecl action_home(int veh_id, int flag);
void __cdecl action_arty(int veh_id, int tx, int ty);
void __cdecl action_destruct(int veh_id);
void __cdecl action_oblit(int veh_id, int base_id);
int __cdecl valid_patrol(int veh_id, int tx, int ty);
int __cdecl action_patrol(int veh_id, int tx, int ty);
int __cdecl shoot_it(int faction_id_atk, int faction_id_def, int tx, int ty, int flag);
void __cdecl action_tectonic(int veh_id, int tx, int ty);
void __cdecl action_fungal(int veh_id, int tx, int ty);
void __cdecl action_give(int veh_id, int faction_id_tgt);
void __cdecl action_gate(int veh_id, int base_id);

/* ---- veh_move.cpp (Thinker veh.cpp movement, pods, monoliths, spotting) ---- */
int __cdecl veh_find(int x, int y, int faction_id, int skip_veh_id);
int __cdecl do_unity_crash(int x, int y, int faction_id);
int __cdecl study_artifact(int veh_id);
int __cdecl mod_veh_skip(int veh_id);
int __cdecl want_to_wake(int faction_id, int veh_id, int veh_id2);

/* ---- terrain.cpp (Thinker map.cpp altitude, movement cost, pods) ---- */
inline int goody_at(int x, int y) { return mod_goody_at(x, y); }
inline int bonus_at(int x, int y) { return mod_bonus_at(x, y); }
inline int minerals_at(int x, int y) { return mod_minerals_at(x, y); }
inline int hex_cost(int unit_id, int faction_id, int x1, int y1, int x2, int y2, int toggle) {
    return mod_hex_cost(unit_id, faction_id, x1, y1, x2, y2, toggle);
}
void __cdecl delete_landmark(int x, int y); // exe-only, new code

/* ---- path.cpp (Thinker path.cpp; Path::find/Path::move are new code) ---- */
const int PathLimit = 160;
const int QueueSize = 8192;
const int MaxTileSearchType = 6;

enum TSType {
    TS_TRIAD_LAND = 0,
    TS_TRIAD_SEA = 1,
    TS_TRIAD_AIR = 2,
    TS_SEA_AND_SHORE = 3,
    TS_TERRITORY_LAND = 4,
    TS_TERRITORY_SHORE = 5,
    TS_TERRITORY_PACT = 6,
};

struct PathNode {
    int x;
    int y;
    int dist;
    int prev;
};

class TileSearch {
    int type;
    int head;
    int tail;
    MAP* sq;
    void reset();
    void add_start(int x, int y);
    public:
    int rx, ry, dist, current, faction_id, y_skip;
    PathNode paths[QueueSize];
    Points oldtiles;
    void init(int x, int y, int ts_type);
    void init(int x, int y, int ts_type, int ts_skip);
    void init(const PointList& points, TSType ts_type, int ts_skip);
    void get_route(PointList& pp);
    bool has_zoc(int plr_id);
    PathNode& get_prev();
    PathNode& get_node();
    MAP* get_next();
};

/* Path::find flag bits understood by the new path search. */
enum PathFlags {
    PATH_IGNORE_ZOC = 0x40,      // AI go-to (ORDER_AI_GO_TO, faction -1)
    PATH_AVOID_FUNGUS = 0x10,    // road-to without the fungus road tech
};

/* Return the BaseOffset direction 0..7 of the first step of the cheapest
 * route from (x1, y1) to (x2, y2) for unit_id of faction_id, or -1. */
int __cdecl Path_find(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int flags, int unk);
/* Next step for the vehicle towards waypoint 0, or -1 (Path::move). */
int __cdecl Path_move(int veh_id, int faction_id);
int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id);
int path_cost(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int max_cost);
int veh_cargo_loaded(int veh_id);
int cargo_capacity(int x, int y, int faction_id);
bool has_transport(int x, int y, int faction_id);

/* ---- orders.cpp: exe-only helpers (new code) and player orders ---- */
int __cdecl supply_options(int veh_id, int base_id);
void __cdecl quick_zoc(int x1, int y1, int faction_id, int x2, int y2, int* zx, int* zy);
int __cdecl base_on_sea(int base_id, int sea_region);
int __cdecl port_to_port(int base_id_1, int base_id_2);
int __cdecl radius_move_2(int x1, int y1, int x2, int y2, int range);
int __cdecl veh_ready(int veh_id);
void __cdecl say_loc(char* dest, int x, int y, int a4, int a5, int a6);

/* Player order entry points for the UI (orders.cpp). Each returns 1 when the
 * order was accepted. Moves spend points through order_veh/mod_hex_cost. */
int order_move(int veh_id, int offset);          // one step, BaseOffset direction 0..7
int order_go_to(int veh_id, int x, int y);       // ORDER_MOVE_TO, then runs action_go_to
int order_road_to(int veh_id, int x, int y, bool magtube);
int order_terraform(int veh_id, int item_id);    // FormerItem
int order_build_base(int veh_id, const char* name); // colony pod
int order_sentry(int veh_id);                    // ORDER_SENTRY_BOARD (also boards a transport)
int order_hold(int veh_id);                      // ORDER_HOLD
int order_wake(int veh_id);
int order_skip(int veh_id);
int order_disband(int veh_id);
int order_set_home(int veh_id);                  // home base = base under the unit
int order_return_home(int veh_id);               // action_home(veh_id, 0)
int order_convoy(int veh_id, int resource);      // supply crawler: 1..3 nutrient/mineral/energy
int order_upgrade(int veh_id, int new_unit_id);  // single unit upgrade, pays credits
int order_airdrop(int veh_id, int x, int y);
int order_patrol(int veh_id, int x, int y);
/* Continue standing orders (terraform, go-to, road-to) for one unit. */
int order_continue(int veh_id);

} // namespace engine
} // namespace alpha
