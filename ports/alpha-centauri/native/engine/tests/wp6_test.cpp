/* SPDX-License-Identifier: GPL-3.0-or-later
 * Computer player checks (WP6), driven by test_engine_wp6.py.
 *
 *   wp6_test unit
 *       AI helpers on a small fixed state (goals, sites, tech prerequisites,
 *       garrison size, tile search, base tile score); prints
 *       "<n> unit checks passed".
 *   wp6_test turns RULESDIR base|expansion SEED TURNS [keep]
 *       Load rules, replace the component, unit, technology and facility
 *       tables with consistent values (unless "keep"), build a seeded
 *       synthetic world (map, seven computer factions, bases, units, native
 *       life) and play TURNS AI turns. Each turn every faction runs the
 *       Thinker AI in faction upkeep order (plans_upkeep, enemy_strategy,
 *       social AI, research choice, move_upkeep, production choice, one
 *       mod_enemy_move per unit) and then native life. Unit orders are carried
 *       out by a small test executor (go-to steps, built units), since unit
 *       actions belong to other packages. Prints per turn
 *       "turn <t> digest=<hex> <counters> ms=<ms>" and a final
 *       "digest=<hex>" line over the whole run; the digests cover only the
 *       game state, so host and m68k runs must print the same ones.
 */
#include "../engine.h"
#include "../ai.h"

#include <chrono>
#include <memory>

using namespace alpha::engine;

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    checks++; \
    long long va_ = (long long)(a), vb_ = (long long)(b); \
    if (va_ != vb_) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK_EQ failed: %s = %lld, %s = %lld\n", \
            __FILE__, __LINE__, #a, va_, #b, vb_); \
    } \
} while (0)

/* ---- test random numbers (independent of the engine RNG) ---- */
static uint32_t trng_state = 1;
static uint32_t trng() {
    trng_state = trng_state * 1103515245u + 12345u;
    return (trng_state >> 8) & 0xFFFFFF;
}
static int trand(int n) {
    return n > 0 ? (int)(trng() % (uint32_t)n) : 0;
}

/* ---- digest (FNV-1a over integer values, endian independent) ---- */
struct Digest {
    uint64_t h = 1469598103934665603ull;
    void add(long long v) {
        uint64_t u = (uint64_t)v;
        for (int i = 0; i < 8; i++) {
            h ^= (u >> (8 * i)) & 0xFF;
            h *= 1099511628211ull;
        }
    }
};

/* ---- event sink that counts package stubs by name ---- */
class CountSink : public EventSink {
    public:
    std::map<std::string, int> missing;
    int popup(const PopupRequest&) override { return 0; }
    void redraw(const RedrawRequest&) override {}
    void notify(const EngineEvent& e) override {
        if (e.type == EV_NOT_IMPLEMENTED && e.text) {
            missing[e.text]++;
        }
    }
};

/* ---- rules tables ----
 * The synthetic rules of test_engine_wp0b.py check the loader and are not
 * meant to be played (cyclic triads, negative costs, all techs at level 1).
 * These values give a consistent game: a technology tree, increasing
 * component strength and positive costs. They are test values, not the
 * original game data. */
static void set_tables() {
    // speed, triad, range, cargo, cost, missile, preq
    const int chs[MaxChassisNum][7] = {
        {1, TRIAD_LAND, 0, 1, 1, 0, -1}, {2, TRIAD_LAND, 0, 1, 2, 0, 4},
        {3, TRIAD_LAND, 0, 1, 3, 0, 30}, {4, TRIAD_SEA, 0, 4, 4, 0, 2},
        {6, TRIAD_SEA, 0, 8, 6, 0, 33}, {8, TRIAD_AIR, 2, 0, 8, 0, 40},
        {8, TRIAD_AIR, 1, 0, 8, 0, 45}, {8, TRIAD_AIR, 0, 0, 12, 0, 70},
        {12, TRIAD_AIR, 1, 0, 12, 1, 50},
    };
    for (int i = 0; i < MaxChassisNum; i++) {
        Chassis[i].speed = (uint8_t)chs[i][0];
        Chassis[i].triad = (uint8_t)chs[i][1];
        Chassis[i].range = (uint8_t)chs[i][2];
        Chassis[i].cargo = (uint8_t)chs[i][3];
        Chassis[i].cost = (uint8_t)chs[i][4];
        Chassis[i].missile = (uint8_t)chs[i][5];
        Chassis[i].preq_tech = (int16_t)chs[i][6];
    }
    // offense, mode, cost, preq
    const int wpn[MaxWeaponNum][4] = {
        {1, WMODE_PROJECTILE, 1, -1}, {2, WMODE_ENERGY, 2, 1}, {4, WMODE_PROJECTILE, 4, 8},
        {5, WMODE_ENERGY, 5, 14}, {6, WMODE_MISSILE, 6, 20}, {8, WMODE_ENERGY, 8, 28},
        {10, WMODE_ENERGY, 10, 36}, {12, WMODE_PROJECTILE, 12, 44}, {13, WMODE_MISSILE, 13, 52},
        {16, WMODE_ENERGY, 16, 60}, {20, WMODE_MISSILE, 20, 68}, {24, WMODE_ENERGY, 24, 76},
        {6, WMODE_ENERGY, 6, 24}, {12, WMODE_PROJECTILE, 12, 48}, {30, WMODE_PROJECTILE, 30, 84},
        {-1, WMODE_PROJECTILE, 10, 26}, {-12, WMODE_MISSILE, 32, 80},
        {0, WMODE_COLONY, 10, -1}, {0, WMODE_TERRAFORM, 6, -1}, {0, WMODE_TRANSPORT, 4, 2},
        {0, WMODE_SUPPLY, 8, 22}, {0, WMODE_PROBE, 4, 6}, {0, WMODE_ARTIFACT, 36, -2},
        {12, WMODE_MISSILE, 12, 50}, {-1, WMODE_MISSILE, 20, -2}, {-1, WMODE_MISSILE, 20, -2},
    };
    for (int i = 0; i < MaxWeaponNum; i++) {
        Weapon[i].offense_value = (int8_t)wpn[i][0];
        Weapon[i].mode = (uint8_t)wpn[i][1];
        Weapon[i].cost = (uint8_t)wpn[i][2];
        Weapon[i].preq_tech = (int16_t)wpn[i][3];
    }
    // defense, cost, preq
    const int arm[MaxArmorNum][3] = {
        {1, 1, -1}, {2, 2, 3}, {3, 3, 12}, {4, 4, 22}, {5, 5, 32}, {6, 6, 42}, {8, 8, 54},
        {10, 10, 64}, {12, 12, 74}, {-1, 6, 27}, {3, 3, 18}, {3, 3, 25}, {8, 8, 58}, {8, 8, 62},
    };
    for (int i = 0; i < MaxArmorNum; i++) {
        Armor[i].defense_value = (int8_t)arm[i][0];
        Armor[i].mode = 0;
        Armor[i].cost = (uint8_t)arm[i][1];
        Armor[i].preq_tech = (int16_t)arm[i][2];
    }
    for (int i = 0; i < MaxReactorNum; i++) {
        Reactor[i].power = (int16_t)(i + 1);
        Reactor[i].preq_tech = (int16_t)(i ? 30 + 20 * i : -1);
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        Ability[i].cost = 1 + i % 3;
        Ability[i].preq_tech = (int16_t)(i % 4 ? -2 : 10 + 2 * i);
    }
    // Technology tree: every tech above the first eight needs one or two lower techs.
    for (int i = 0; i < MaxTechnologyNum; i++) {
        Tech[i].flags = 0;
        Tech[i].preq_tech1 = i < 8 ? -1 : (i * 13 + 5) % (i - 2);
        Tech[i].preq_tech2 = i < 16 ? -1 : (i * 29 + 3) % (i - 4);
        Tech[i].AI_growth = i % 4;
        Tech[i].AI_tech = (i / 4) % 4;
        Tech[i].AI_wealth = (i / 16) % 4;
        Tech[i].AI_power = (i * 7) % 4;
        if (Tech[i].preq_tech2 == Tech[i].preq_tech1) {
            Tech[i].preq_tech2 = -1;
        }
    }
    for (int i = 1; i <= SP_ID_Last; i++) {
        bool project = i >= SP_ID_First;
        Facility[i].cost = project ? 20 + i % 20 : 4 + i % 12;
        Facility[i].maint = project ? 0 : i % 4;
        Facility[i].preq_tech = (i < 70 && i > 40) || i > 115 ? -2 : (i % 5 ? (i * 7) % 89 : -1);
        Facility[i].free_tech = -1;
    }
}

struct UnitDef {
    int chs, wpn, arm, plan, cost, preq;
};

static void set_units() {
    // Index = VehBasicUnit. Natives use the psi weapon.
    const UnitDef def[] = {
        {CHS_INFANTRY, WPN_COLONY_MODULE, ARM_NO_ARMOR, PLAN_COLONY, 10, -1},
        {CHS_INFANTRY, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, PLAN_TERRAFORM, 6, -1},
        {CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, PLAN_DEFENSE, 2, -1},
        {CHS_FOIL, WPN_TROOP_TRANSPORT, ARM_NO_ARMOR, PLAN_NAVAL_TRANSPORT, 5, 2},
        {CHS_FOIL, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, PLAN_TERRAFORM, 6, 2},
        {CHS_INFANTRY, WPN_SUPPLY_TRANSPORT, ARM_NO_ARMOR, PLAN_SUPPLY, 8, 22},
        {CHS_SPEEDER, WPN_PROBE_TEAM, ARM_NO_ARMOR, PLAN_PROBE, 4, 6},
        {CHS_INFANTRY, WPN_ALIEN_ARTIFACT, ARM_NO_ARMOR, PLAN_ARTIFACT, 36, -2},
        {CHS_INFANTRY, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, PLAN_OFFENSE, 6, -2},
        {CHS_FOIL, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, PLAN_NAVAL_SUPERIORITY, 10, -2},
        {CHS_NEEDLEJET, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, PLAN_AIR_SUPERIORITY, 10, -2},
    };
    const int n = (int)(sizeof(def) / sizeof(def[0]));
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (i < n) {
            const UnitDef& d = def[i];
            u->chassis_id = (uint8_t)d.chs;
            u->weapon_id = (uint8_t)d.wpn;
            u->armor_id = (uint8_t)d.arm;
            u->reactor_id = REC_FISSION;
            u->plan = (int8_t)d.plan;
            u->cost = (uint8_t)d.cost;
            u->preq_tech = (int16_t)d.preq;
            u->carry_capacity = (uint8_t)(d.wpn == WPN_TROOP_TRANSPORT ? 4 : 0);
            u->ability_flags = 0;
            u->unit_flags = 1;
            u->obsolete_factions = 0;
            snprintf(u->name, sizeof(u->name), "Basic%d", i);
        } else {
            memset(u, 0, sizeof(*u));
            u->preq_tech = TECH_Disable; // unused slot
        }
    }
}

/* ---- synthetic world ---- */
static const int MapW = 48;
static const int MapH = 32;

static void fill_regions() {
    std::vector<int> mark((size_t)*MapAreaTiles, 0);
    int next_land = 1, next_sea = 65;
    memset(Continents, 0, sizeof(Continent) * 128);
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (y == 0 || y == *MapAreaY - 1) {
                sq->region = is_ocean(sq) ? 127 : 63; // poles
            }
        }
    }
    for (int y = 1; y < *MapAreaY - 1; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* start = mapsq(x, y);
            size_t idx = (size_t)(x / 2 + y * (*MapAreaX / 2));
            if (mark[idx]) {
                continue;
            }
            bool ocean = is_ocean(start);
            int region = ocean ? next_sea : next_land;
            if (ocean ? next_sea < 126 : next_land < 62) {
                (ocean ? next_sea : next_land)++;
            }
            std::vector<Point> queue;
            queue.push_back({x, y});
            mark[idx] = 1;
            for (size_t head = 0; head < queue.size(); head++) {
                Point p = queue[head];
                MAP* sq = mapsq(p.x, p.y);
                sq->region = (uint8_t)region;
                Continents[region].tile_count++;
                if (!(sq->items & BIT_FUNGUS) && !(sq->val3 & TILE_ROCKY)) {
                    Continents[region].open_terrain++;
                }
                for (int i = 0; i < 8; i++) {
                    int nx = wrap(p.x + BaseOffsetX[i]);
                    int ny = p.y + BaseOffsetY[i];
                    if (ny <= 0 || ny >= *MapAreaY - 1) {
                        continue;
                    }
                    MAP* n2 = mapsq(nx, ny);
                    size_t j = (size_t)(nx / 2 + ny * (*MapAreaX / 2));
                    if (n2 && !mark[j] && is_ocean(n2) == ocean) {
                        mark[j] = 1;
                        queue.push_back({nx, ny});
                    }
                }
            }
        }
    }
}

static void make_map() {
    *MapAreaX = MapW;
    *MapAreaY = MapH;
    if (map_init()) {
        fprintf(stderr, "map_init failed\n");
        exit(2);
    }
    *MapSeaLevel = 0;
    Point centers[6];
    int radius[6];
    for (int i = 0; i < 6; i++) {
        centers[i] = {trand(MapW), 4 + trand(MapH - 8)};
        radius[i] = 4 + trand(5);
    }
    for (int y = 0; y < MapH; y++) {
        for (int x = y & 1; x < MapW; x += 2) {
            MAP* sq = mapsq(x, y);
            int best = 99;
            for (int i = 0; i < 6; i++) {
                int d = vector_dist(x, y, centers[i].x, centers[i].y) - radius[i];
                best = std::min(best, d);
            }
            best += trand(3) - 1;
            int alt;
            if (y == 0 || y == MapH - 1) {
                alt = ALT_OCEAN;
            } else if (best <= 0) {
                alt = std::min((int)ALT_FOUR_ABOVE_SEA, ALT_SHORE_LINE + (-best) / 2);
            } else {
                alt = best <= 2 ? ALT_OCEAN_SHELF : ALT_OCEAN;
            }
            int rain = trand(3);
            int temp = 1 + (y > 4 && y < MapH - 5);
            sq->climate = (uint8_t)((alt << 5) | (rain << 3) | temp);
            sq->contour = (uint8_t)(alt * 20 + trand(10));
            sq->owner = -1;
            sq->visibility = 0xFF;
            sq->val2 = 0xF;
            sq->items = 0;
            if (alt >= ALT_SHORE_LINE) {
                if (!trand(7)) {
                    sq->items |= BIT_FUNGUS;
                } else if (!trand(9)) {
                    sq->val3 |= TILE_ROCKY;
                }
            }
        }
    }
    fill_regions();
}

static bool free_land(int x, int y) {
    MAP* sq = mapsq(x, y);
    return sq && !is_ocean(sq) && !bad_reg(sq->region) && base_at(x, y) < 0
        && veh_at(x, y) < 0;
}

static int faction_bases[MaxPlayerNum];

static void set_territory() {
    for (int y = 0; y < MapH; y++) {
        for (int x = y & 1; x < MapW; x += 2) {
            MAP* sq = mapsq(x, y);
            int best = 99, owner = -1;
            for (int b = 0; b < *BaseCount; b++) {
                int d = map_range(x, y, Bases[b].x, Bases[b].y);
                if (d < best || (d == best && Bases[b].faction_id < owner)) {
                    best = d;
                    owner = Bases[b].faction_id;
                }
            }
            sq->owner = (int8_t)(best <= 3 ? owner : -1);
        }
    }
}

static void place_unit(int unit_id, int faction_id, int x, int y, int home) {
    int veh_id = veh_init(unit_id, faction_id, x, y);
    if (veh_id >= 0) {
        Vehs[veh_id].home_base_id = (int16_t)home;
        Vehs[veh_id].morale = (uint8_t)(1 + trand(3));
    }
}

static void make_factions() {
    *GameState = 0;
    *DiffLevel = 3;
    *CurrentTurn = 1;
    FactionStatus[0] = 0; // no human players
    set_alive(0, true);
    for (int i = 1; i < MaxPlayerNum; i++) {
        Faction* f = &Factions[i];
        set_alive(i, true);
        f->diff_level = 3;
        f->energy_credits = 30 + trand(60);
        f->AI_fight = MFactions[i].AI_fight;
        f->AI_growth = MFactions[i].AI_growth;
        f->AI_tech = MFactions[i].AI_tech;
        f->AI_wealth = MFactions[i].AI_wealth;
        f->AI_power = MFactions[i].AI_power;
        f->tech_research_id = -1;
        f->base_id_attack_target = -1;
        f->SE_police = f->SE_police_pending = trand(3) - 1;
        f->SE_support = f->SE_support_pending = trand(3) - 1;
        f->SE_morale = f->SE_morale_pending = trand(3) - 1;
        for (int j = 0; j < MaxGoalsNum; j++) {
            f->goals[j].type = AI_GOAL_UNUSED;
        }
        for (int j = 0; j < MaxSitesNum; j++) {
            f->sites[j].type = AI_GOAL_UNUSED;
        }
        int count = 0;
        for (int t = 0; t < MaxTechnologyNum; t++) {
            if (t < 8 + i || (Tech[t].preq_tech1 >= 0 && Tech[t].preq_tech1 < 8 && trand(3) == 0)) {
                TechOwners[t] |= (uint8_t)(1 << i);
                count++;
            }
        }
        f->tech_ranking = 2 * count;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        for (int j = i + 1; j < MaxPlayerNum; j++) {
            uint32_t s = DIPLO_COMMLINK;
            int r = trand(6);
            if (r == 0) {
                s |= DIPLO_VENDETTA;
            } else if (r == 1) {
                s |= DIPLO_PACT | DIPLO_TREATY;
            } else if (r == 2) {
                s |= DIPLO_TREATY;
            }
            Factions[i].diplo_status[j] = s;
            Factions[j].diplo_status[i] = s;
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        int want = 2 + trand(2), tries = 0;
        faction_bases[i] = 0;
        while (faction_bases[i] < want && tries++ < 2000) {
            int y = 2 + trand(MapH - 4);
            int x = (trand(MapW / 2) * 2) + (y & 1);
            if (!free_land(x, y)) {
                continue;
            }
            bool near = false;
            for (int b = 0; b < *BaseCount; b++) {
                if (map_range(x, y, Bases[b].x, Bases[b].y) < 4) {
                    near = true;
                }
            }
            if (near) {
                continue;
            }
            int base_id = base_init(i, x, y);
            if (base_id < 0) {
                continue;
            }
            BASE* b = &Bases[base_id];
            b->pop_size = (int8_t)(1 + trand(6));
            b->nutrient_surplus = 1 + trand(4);
            b->mineral_surplus = 2 + trand(8);
            b->mineral_intake_2 = b->mineral_surplus + 1;
            b->energy_surplus = 2 + trand(8);
            b->economy_total = b->energy_surplus / 2;
            b->labs_total = b->energy_surplus / 2;
            b->minerals_accumulated = trand(20);
            b->queue_size = 0;
            b->queue_items[0] = BSC_SCOUT_PATROL;
            b->defend_goal = 0;
            faction_bases[i]++;
            place_unit(BSC_SCOUT_PATROL, i, x, y, base_id);
            place_unit(BSC_FORMERS, i, x, y, base_id);
            if (trand(2)) {
                place_unit(BSC_COLONY_POD, i, x, y, base_id);
            }
        }
        Factions[i].base_count = faction_bases[i];
    }
    for (int k = 0; k < 6; k++) {
        for (int tries = 0; tries < 200; tries++) {
            int y = 2 + trand(MapH - 4);
            int x = (trand(MapW / 2) * 2) + (y & 1);
            if (free_land(x, y) && !(mapsq(x, y)->items & BIT_BASE_RADIUS)) {
                place_unit(BSC_MIND_WORMS, 0, x, y, -1);
                break;
            }
        }
    }
    set_territory();
    rebuild_base_bits();
    rebuild_vehicle_bits();
}

/* ---- test executor: carries out what the AI ordered ---- */
static bool can_enter(int veh_id, int x, int y) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return false;
    }
    int triad = veh->triad();
    int base_id = base_at(x, y);
    if (base_id >= 0 && Bases[base_id].faction_id != veh->faction_id) {
        return false;
    }
    int other = veh_at(x, y);
    if (other >= 0 && Vehs[other].faction_id != veh->faction_id) {
        return false;
    }
    if (triad == TRIAD_AIR) {
        return true;
    }
    return base_id >= 0 || (triad == TRIAD_SEA) == is_ocean(sq);
}

static void step_units() {
    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        VEH* veh = &Vehs[veh_id];
        veh->moves_spent = 0;
        if (veh->order != ORDER_MOVE_TO && veh->order != ORDER_ROAD_TO) {
            continue;
        }
        int tx = veh->waypoint_x[0], ty = veh->waypoint_y[0];
        if (!mapsq(tx, ty) || (veh->x == tx && veh->y == ty)) {
            veh->order = ORDER_NONE;
            continue;
        }
        int steps = std::max(1, (int)veh->speed());
        for (int s = 0; s < steps; s++) {
            int best = map_range(veh->x, veh->y, tx, ty), bx = -1, by = -1;
            for (int i = 0; i < 8; i++) {
                int nx = wrap(veh->x + BaseOffsetX[i]);
                int ny = veh->y + BaseOffsetY[i];
                if (!mapsq(nx, ny) || !can_enter(veh_id, nx, ny)) {
                    continue;
                }
                int d = map_range(nx, ny, tx, ty);
                if (d < best) {
                    best = d;
                    bx = nx;
                    by = ny;
                }
            }
            if (bx < 0) {
                veh->order = ORDER_NONE;
                break;
            }
            veh_put(veh_id, bx, by);
            veh = &Vehs[veh_id];
            if (bx == tx && by == ty) {
                veh->order = ORDER_NONE;
                break;
            }
        }
    }
    rebuild_vehicle_bits();
}

static int choices_made = 0;

static void production(int faction_id) {
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE* base = &Bases[base_id];
        if (base->faction_id != faction_id) {
            continue;
        }
        base->minerals_accumulated += base->mineral_surplus;
        int item = base->queue_items[0];
        int cost = item >= 0 ? Units[item].cost * 2 : Facility[-item].cost * 3;
        if (base->minerals_accumulated >= cost && cost > 0) {
            base->minerals_accumulated -= cost;
            if (item >= 0) {
                if (*VehCount < MaxVehNum - 64) {
                    place_unit(item, faction_id, base->x, base->y, base_id);
                }
            } else if (-item < SP_ID_First) {
                set_fac((FacilityId)(-item), base_id, true);
            } else if (project_base((FacilityId)(-item)) < 0) {
                SecretProjects[-item - SP_ID_First] = base_id;
            }
            base->state_flags |= BSTATE_PRODUCTION_DONE;
        }
        if (base->state_flags & BSTATE_PRODUCTION_DONE || !trand(4)) {
            base->state_flags &= ~BSTATE_PRODUCTION_DONE;
            int choice = select_build(base_id);
            choices_made++;
            if (choice >= -SP_ID_Last && choice < MaxProtoNum) {
                Bases[base_id].queue_items[0] = choice;
            }
        }
    }
}

static void digest_state(Digest& d) {
    d.add(*VehCount);
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        d.add(v->x); d.add(v->y); d.add(v->unit_id); d.add(v->faction_id);
        d.add(v->order); d.add(v->waypoint_x[0]); d.add(v->waypoint_y[0]);
        d.add(v->state); d.add(v->flags); d.add(v->status_icon); d.add(v->home_base_id);
    }
    d.add(*BaseCount);
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        d.add(b->x); d.add(b->y); d.add(b->faction_id); d.add(b->queue_items[0]);
        d.add(b->minerals_accumulated); d.add(b->defend_goal); d.add(b->defend_range);
        for (int j = 0; j < 12; j++) {
            d.add(b->facilities_built[j]);
        }
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        UNIT* u = &Units[i];
        d.add(u->chassis_id); d.add(u->weapon_id); d.add(u->armor_id); d.add(u->reactor_id);
        d.add(u->ability_flags); d.add(u->plan); d.add(u->cost); d.add(u->obsolete_factions);
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        Faction* f = &Factions[i];
        d.add(f->tech_research_id);
        d.add(f->SE_Politics_pending); d.add(f->SE_Economics_pending);
        d.add(f->SE_Values_pending); d.add(f->SE_Future_pending);
        d.add(f->base_id_attack_target);
        for (int j = 0; j < MaxGoalsNum; j++) {
            Goal& g = f->goals[j];
            d.add(g.type); d.add(g.priority); d.add(g.x); d.add(g.y); d.add(g.base_id);
        }
        for (int j = 0; j < 128; j++) {
            d.add(f->region_base_plan[j]); d.add(f->region_flags[j]);
            d.add(f->region_total_combat_units[j]); d.add(f->region_force_rating[j]);
        }
        AIPlans& p = plans[i];
        d.add(p.main_region); d.add(p.main_sea_region); d.add(p.target_land_region);
        d.add(p.prioritize_naval); d.add(p.naval_start_x); d.add(p.naval_end_x);
        d.add(p.naval_beach_x); d.add(p.land_combat_units); d.add(p.sea_combat_units);
        d.add(p.air_combat_units); d.add(p.transport_units); d.add(p.enemy_factions);
        d.add(p.project_limit); d.add(p.median_limit); d.add(p.psi_score);
        d.add(p.keep_fungus); d.add(p.plant_fungus); d.add(p.satellite_goal);
        d.add(p.mil_strength); d.add(p.defense_modifier); d.add(p.enemy_bases);
        // float values, compared at a fixed precision
        d.add((long long)(p.enemy_base_range * 1000.0f));
        d.add((long long)(p.enemy_mil_factor * 1000.0f));
    }
    for (int y = 0; y < MapH; y++) {
        for (int x = y & 1; x < MapW; x += 2) {
            MAP* sq = mapsq(x, y);
            d.add(sq->val2); d.add(sq->items); d.add(sq->owner);
        }
    }
}

static std::unique_ptr<EngineState> state;

static int load_rules(const char* dir, const char* variant, bool keep) {
    state.reset(new EngineState());
    engine_reset(*state);
    engine_bind(state.get());
    int expansion = !strcmp(variant, "expansion");
    if (engine_load_rules(dir, expansion)) {
        printf("error=%s\n", engine_rules_error());
        return 20;
    }
    if (!keep) {
        set_tables();
        set_units();
    }
    config_vanilla(conf);
    ai_reset_state();
    return 0;
}

static int run_turns(const char* dir, const char* variant, uint32_t seed, int turns, bool keep) {
    int r = load_rules(dir, variant, keep);
    if (r) {
        return r;
    }
    CountSink sink;
    set_event_sink(&sink);
    game_srand(seed);
    trng_state = seed * 2654435761u + 1;
    make_map();
    make_factions();
    Digest total;
    double total_ms = 0;
    for (int turn = 1; turn <= turns; turn++) {
        *CurrentTurn = turn;
        ai_stats_reset();
        choices_made = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int f = 1; f < MaxPlayerNum; f++) {
            if (!is_alive(f)) {
                continue;
            }
            *CurrentFaction = f;
            ai_plans_upkeep(f);
            production(f);
            enemy_strategy(f);
            mod_social_ai(f, -1, -1, -1, -1, 0);
            ai_move_upkeep(f, 0);
            Factions[f].tech_research_id = mod_tech_ai(f);
            int count = *VehCount;
            for (int v = 0; v < count && v < *VehCount; v++) {
                if (Vehs[v].faction_id == f) {
                    mod_enemy_move(v);
                }
            }
            step_units();
            set_territory();
        }
        *CurrentFaction = 0;
        alien_fauna();
        int count = *VehCount;
        for (int v = 0; v < count && v < *VehCount; v++) {
            if (Vehs[v].faction_id == 0) {
                mod_alien_move(v);
            }
        }
        step_units();
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        total_ms += ms;
        Digest d;
        digest_state(d);
        total.add((long long)(d.h >> 32));
        total.add((long long)(d.h & 0xFFFFFFFF));
        const AIStats& s = ai_stats();
        printf("turn %d digest=%016llx vehs=%d bases=%d choices=%d enemy_moves=%u move_upkeeps=%u "
            "tile_steps=%u path_finds=%u builds=%u strategy=%u ms=%.1f\n",
            turn, (unsigned long long)d.h, *VehCount, *BaseCount, choices_made, s.veh_moves,
            s.move_upkeeps, s.tile_search_steps, s.path_find_calls, s.build_choices,
            s.strategy_calls, ms);
    }
    printf("digest=%016llx\n", (unsigned long long)total.h);
    printf("total_ms=%.1f\n", total_ms);
    for (auto& m : sink.missing) {
        printf("stub %s %d\n", m.first.c_str(), m.second);
    }
    set_event_sink(nullptr);
    return 0;
}

/* ---- unit checks ---- */
static int unit_checks(const char* dir, const char* variant) {
    if (load_rules(dir, variant, false)) {
        return 20;
    }
    CountSink sink;
    set_event_sink(&sink);
    trng_state = 7;
    make_map();
    make_factions();

    // Technology prerequisites follow the tree set_tables built.
    int t = 40, p1 = Tech[t].preq_tech1;
    CHECK(ai::tech_is_preq(p1, t, 1));
    CHECK(ai::tech_is_preq(t, t, 0));
    CHECK(!ai::tech_is_preq(t, p1, 5)); // a lower tech never needs a higher one
    CHECK(!ai::tech_is_preq(-1, t, 3));
    CHECK(ai::tech_is_preq(Tech[p1].preq_tech1 >= 0 ? Tech[p1].preq_tech1 : p1, t, 2));

    // Goals: Thinker goal types are stored, merged by tile and cleared.
    int x = Bases[0].x, y = Bases[0].y, f = Bases[0].faction_id;
    ai::add_goal(f, AI_GOAL_RAISE_LAND, 3, x, y, 0);
    CHECK(ai::has_goal(f, AI_GOAL_RAISE_LAND, x, y) == 3);
    ai::add_goal(f, AI_GOAL_RAISE_LAND, 7, x, y, 0);
    CHECK(ai::has_goal(f, AI_GOAL_RAISE_LAND, x, y) == 7);
    ai::add_goal(f, AI_GOAL_ATTACK, 5, x, y, 0); // original goal types are left to the exe AI
    CHECK(!ai::has_goal(f, AI_GOAL_ATTACK, x, y));
    ai::wipe_goals(f);
    ai::clear_goals(f);
    CHECK(!ai::has_goal(f, AI_GOAL_RAISE_LAND, x, y));
    // Sites are kept only for factions the exe AI controls.
    add_site(f, AI_GOAL_COLONIZE, 4, x, y);
    bool found = false;
    for (int i = 0; i < MaxSitesNum; i++) {
        found |= Factions[f].sites[i].type == AI_GOAL_COLONIZE;
    }
    CHECK(!found);
    conf.factions_enabled = 0;
    add_site(f, AI_GOAL_COLONIZE, 4, x, y);
    found = false;
    for (int i = 0; i < MaxSitesNum; i++) {
        found |= Factions[f].sites[i].type == AI_GOAL_COLONIZE && Factions[f].sites[i].x == x;
    }
    CHECK(found);
    del_site(f, AI_GOAL_COLONIZE, x, y, 0);
    found = false;
    for (int i = 0; i < MaxSitesNum; i++) {
        found |= Factions[f].sites[i].type == AI_GOAL_COLONIZE;
    }
    CHECK(!found);
    conf.factions_enabled = 7;

    // Garrison size stays in the original 1..10 range and grows with population.
    for (int b = 0; b < *BaseCount; b++) {
        int g = ai::garrison_check(b);
        CHECK(g >= 1 && g <= 10);
    }
    Bases[0].pop_size = 1;
    int small = ai::garrison_check(0);
    Bases[0].pop_size = 20;
    CHECK(ai::garrison_check(0) > small);

    // Tile search from a base visits each reachable land tile once, nearest first.
    ai::TileSearch ts;
    ts.init(x, y, ai::TS_TRIAD_LAND);
    int visited = 0, last_dist = 0;
    bool ordered = true;
    std::set<std::pair<int, int>> seen;
    while (ts.get_next() != NULL) {
        visited++;
        ordered &= ts.dist >= last_dist;
        last_dist = ts.dist;
        seen.insert({ts.rx, ts.ry});
        CHECK(!is_ocean(ts.rx, ts.ry));
    }
    CHECK(ordered);
    CHECK_EQ((int)seen.size(), visited);
    CHECK(visited >= 1 && visited <= Continents[region_at(x, y)].tile_count);

    // Base tile scores are 0 on ocean and bounded on land.
    for (int yy = 2; yy < MapH - 2; yy += 3) {
        for (int xx = yy & 1; xx < MapW; xx += 6) {
            MAP* sq = mapsq(xx, yy);
            int s = ai::base_tile_score(xx, yy, f, sq);
            CHECK(s >= -100 && s <= 1000);
        }
    }
    CHECK(ai::world_site(x, y, 0) >= 1 && ai::world_site(x, y, 0) <= 15);

    // Region helpers from ai_exe.cpp.
    int sea = -1;
    for (int i = 0; i < 8 && sea < 0; i++) {
        int nx = wrap(x + BaseOffsetX[i]), ny = y + BaseOffsetY[i];
        if (mapsq(nx, ny) && is_ocean(nx, ny)) {
            sea = region_at(nx, ny);
        }
    }
    if (sea >= 0) {
        CHECK(ai::base_on_sea(0, sea));
    }
    CHECK(!ai::base_on_sea(0, 127));
    CHECK(ai::territory_route(x, y, x, y, false));
    CHECK_EQ(ai::abstract_at(-2, -2), 0);
    ai::retire_proto(BSC_SCOUT_PATROL, f);
    CHECK(Units[BSC_SCOUT_PATROL].obsolete_factions & (1 << f));
    Units[BSC_SCOUT_PATROL].obsolete_factions = 0;

    // Technology values are positive for unknown techs the faction can research.
    int any = 0;
    for (int i = 0; i < MaxTechnologyNum; i++) {
        if (!has_tech(i, f)) {
            int v = mod_tech_val(i, f, 0);
            CHECK(v >= 0);
            any += v > 0;
        }
    }
    CHECK(any > 0);

    // Planning entry points run on the fresh world without changing the map.
    ai_plans_upkeep(f);
    CHECK(plans[f].main_region > 0 && plans[f].main_region < 63);
    ai_move_upkeep(f, 0);
    CHECK(ai_stats().move_upkeeps >= 1);
    int choice = select_build(0);
    CHECK(choice >= -SP_ID_Last && choice < MaxProtoNum); // 0 is the colony pod
    CHECK_EQ(ai_probe_upkeep(f), 0);

    set_event_sink(nullptr);
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 4 && !strcmp(argv[1], "unit")) {
        return unit_checks(argv[2], argv[3]);
    }
    if (argc >= 6 && !strcmp(argv[1], "turns")) {
        bool keep = argc >= 7 && !strcmp(argv[6], "keep");
        return run_turns(argv[2], argv[3], (uint32_t)strtoul(argv[4], NULL, 10), atoi(argv[5]), keep);
    }
    fprintf(stderr, "usage: wp6_test unit RULESDIR VARIANT | turns RULESDIR VARIANT SEED TURNS [keep]\n");
    return 2;
}
