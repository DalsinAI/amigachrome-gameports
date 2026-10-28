/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: private declarations of the AI sources (ai_*.cpp).
 * Ported from Thinker Mod src/move.h, src/plan.h, src/goal.h, src/path.h,
 * src/build.h and src/veh_turn.h at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 *
 * Everything here lives in namespace alpha::engine::ai so that helper copies
 * of functions other packages also port (path search, base and unit queries)
 * never clash with theirs at link time. Inside the namespace, calls resolve to
 * the AI copy first and to alpha::engine (core and engine_wp.h) otherwise.
 * The engine_wp.h entry points of WP6 are defined in alpha::engine itself
 * (never redeclared here, which would make calls ambiguous through ADL).
 */
#pragma once

#include "engine.h"
#include "ai_deps.h"

#include <queue>

#ifdef __GNUC__
/* Thinker release builds compile with these warnings off (src/main.h); the
 * ported bodies keep their debug-only locals. */
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#endif

namespace alpha {
namespace engine {
namespace ai {

/* ---- engine.h (Thinker) AI records ---- */
struct WItem {
    int AI_growth;
    int AI_tech;
    int AI_wealth;
    int AI_power;
    int AI_fight;
};

struct PInfo {
    int former;
    int safety;
    int target;
    int roads;
    int flags;
    int unit_near;
    int unit_path;
    int enemy;
    int enemy_near;
    int enemy_dist;
    int enemy_rank;
    int overlay;

    int get_enemy_dist() const {
        return (enemy_dist > 0 ? enemy_dist : MaxEnemyRange/2);
    }
};

typedef std::unordered_map<Point, PInfo> PMTable;

/* ---- path.h ---- */
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

enum NodesetType {
    NODE_BOREHOLE, // Borehole being built
    NODE_RAISE_LAND, // Land raise action initiated
    NODE_SENSOR_ARRAY, // Sensor being built
    NODE_NEED_FERRY,
    NODE_BASE_SITE, // Skip from search
    NODE_CONVOY_SITE, // Skip from search
    NODE_GOAL_RAISE_LAND, // Former priority only
    NODE_NAVAL_START,
    NODE_NAVAL_BEACH,
    NODE_NAVAL_END,
    NODE_NAVAL_PICK,
    NODE_SCOUT_SITE,
    NODE_PATROL, // Includes probes/transports
    NODE_COMBAT_PATROL, // Only attack-capable units
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
    void adjust_roads(PMTable& tbl, int value);
    void connect_roads(PMTable& tbl, int x, int y, int plr_id);
    bool has_zoc(int plr_id);
    PathNode& get_prev();
    PathNode& get_node();
    MAP* get_next();
};

int __cdecl contiguous(int faction_id_1, int faction_id_2, int region, int flag);
int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction_id);
int path_cost(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int max_cost);
int route_dist(PMTable& tbl, int x1, int y1, int x2, int y2);
void update_move_path(PMTable& tbl, int veh_id, int tx, int ty);
bool defend_tile(VEH* veh, MAP* sq);
bool safe_path(TileSearch& ts, int faction_id, bool skip_owner);
bool has_base_sites(TileSearch& ts, int x, int y, int faction_id, int triad);
int defender_goal(int x, int y, int faction_id, int triad);
int defender_count(int x, int y, int veh_skip_id);
int garrison_count(int x, int y);
int veh_cargo_loaded(int veh_id);
int cargo_capacity(int x, int y, int faction_id);
bool has_transport(int x, int y, int faction_id);
int move_to_base(int veh_id, bool ally);
int escape_move(int veh_id);
int search_escape(TileSearch& ts, int veh_id, int* tx, int* ty);
int search_base(TileSearch& ts, int veh_id, bool ally, int* tx, int* ty);
int search_route(TileSearch& ts, int veh_id, int* tx, int* ty);

/* ---- move.h ---- */
enum UpdateMode {UM_Full, UM_Visual, UM_Player};
enum StackType {ST_NeutralOnly, ST_NonPactOnly, ST_EnemyOnly, ST_EnemyOneUnit};
enum FormerMode {FM_Auto_Full, FM_Auto_Roads, FM_Auto_Tubes, FM_Auto_Sensors,
    FM_Remove_Fungus, FM_Farm_Road, FM_Mine_Road};

const uint32_t BIT_SIMPLE = (BIT_FARM | BIT_MINE | BIT_SOLAR | BIT_FOREST);
const uint32_t BIT_ADVANCED = (BIT_CONDENSER | BIT_THERMAL_BORE);

const int PM_SAFE = -20;
const int PM_NEAR_SAFE = -40;
const int PM_ShoreLine = 0x100;
const int PM_ShoreMask = 0xff;
const int PM_NearOwnBase = 0x10000;
const int PM_PsiGateBase = 0x20000;
const int PM_LandBaseRds = 0x40000;

extern PMTable mapdata;
extern NodeSet mapnodes;

int choose_defender(int x, int y, int veh_id_atk, MAP* sq);
bool ally_near_tile(int x, int y, int faction_id, int skip_veh_id, int max_range);
bool non_ally_in_tile(int x, int y, int faction_id);
bool allow_move(int x, int y, int faction_id, int triad);
bool allow_civ_move(int x, int y, int faction_id, int triad);
bool can_alter_level(int x, int y, int faction_id, bool raise);
bool stack_search(int x, int y, int faction_id, StackType type, VehWeaponMode mode);
int coast_tiles(int x, int y);
int ocean_coast_tiles(int x, int y);
bool near_ocean_coast(int x, int y);
bool near_sea_coast(int x, int y);
void land_raise_plan(int faction_id);
int pick_scout_target(int faction_id);
void invasion_plan(int faction_id);
void update_main_region(int faction_id);
void move_upkeep(int faction_id, UpdateMode mode);
bool can_build_base(int x, int y, int faction_id, int triad);
int base_tile_score(int x, int y, int faction_id, MAP* sq);
int select_item(int x, int y, int faction_id, FormerMode mode, MAP* sq);
int former_tile_score(int x, int y, int faction_id, MAP* sq);
bool allow_scout(int faction_id, MAP* sq);
bool allow_probe(int faction1, int faction2, bool is_enhanced);
bool allow_attack(int faction1, int faction2, bool is_probe, bool is_enhanced);
bool allow_combat(int x, int y, int faction_id, MAP* sq);
bool allow_conv_missile(int veh_id, int enemy_veh_id, MAP* sq);
bool can_airdrop(int veh_id, MAP* sq);
bool allow_airdrop(int x, int y, int faction_id, bool combat, MAP* sq);
bool invasion_unit(int veh_id);
bool near_landing(int veh_id);
int make_landing(int veh_id);

int crawler_move(const int id);
int colony_move(const int id);
int former_move(const int id);
int artifact_move(const int id);
int trans_move(const int id);
int nuclear_move(const int id);
int airdrop_move(const int id);
int combat_move(const int id);

/* ---- plan.h ---- */
struct SItem {
    int item_id;
    int score;

    bool operator<(SItem const& obj) const
    {
        return score < obj.score || (score == obj.score && item_id < obj.item_id);
    }
    bool operator>(SItem const& obj) const
    {
        return score > obj.score || (score == obj.score && item_id > obj.item_id);
    }
};

struct MItem {
    int x;
    int y;
    int score;

    bool operator<(MItem const& obj) const
    {
        return score < obj.score || (score == obj.score && (x < obj.x || (x == obj.x && y < obj.y)));
    }
    bool operator>(MItem const& obj) const
    {
        return score > obj.score || (score == obj.score && (x > obj.x || (x == obj.x && y > obj.y)));
    }
};

typedef std::priority_queue<SItem, std::vector<SItem>, std::less<SItem>> score_max_queue_t;
typedef std::priority_queue<SItem, std::vector<SItem>, std::greater<SItem>> score_min_queue_t;
typedef std::priority_queue<MItem, std::vector<MItem>, std::less<MItem>> point_max_queue_t;
typedef std::priority_queue<MItem, std::vector<MItem>, std::greater<MItem>> point_min_queue_t;

extern int plan_upkeep_turn;
extern int move_upkeep_faction;

int facility_score(FacilityId item_id, WItem& Wgov);
void governor_priorities(BASE& base, WItem& Wgov);
void reset_state();
void former_plans(int faction_id);
void plans_upkeep(int faction_id);
bool need_police(int faction_id);
int faction_might(int faction_id);
int compare_might(int faction_id, int faction_id_tgt);
int psi_score(int faction_id);
int satellite_goal(int faction_id, int item_id);
int satellite_count(int faction_id, int item_id);

/* ---- goal.h ---- */
void __cdecl add_goal(int faction_id, int type, int priority, int x, int y, int base_id);
void __cdecl wipe_goals(int faction_id);
void __cdecl clear_goals(int faction_id);
int has_goal(int faction_id, int type, int x, int y);
Goal* find_priority_goal(int faction_id, int type, int* px, int* py);

/* ---- build.h (select_build and helpers; find_proto is WP5, mod_base_hurry WP2) ---- */
int consider_staple(int base_id);
bool redundant_project(int faction_id, int item_id);
int find_satellite(int base_id);
int find_missile(int base_id);
int find_project(int base_id, WItem& Wgov);
int need_scouts(int base_id, Triad triad);
bool unit_is_better(int unit_id1, int unit_id2);
int unit_score(BASE* base, int unit_id, int psi_score, int psi_atk, int psi_def, int flags);
int select_colony(int base_id, int num_colony, bool build_ships);
int select_combat(int base_id, bool sea_base, bool build_ships);

/* ---- veh_turn.h ---- */
int __cdecl mod_alien_base(int veh_id, int x, int y);

/* ---- tech.cpp ---- */
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range);

} // namespace ai
} // namespace engine
} // namespace alpha
