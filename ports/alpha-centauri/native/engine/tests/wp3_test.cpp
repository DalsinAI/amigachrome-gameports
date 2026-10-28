/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 checks: unit orders, movement, path search and terraforming
 * (driven by test_engine_wp3.py).
 *
 *   wp3_test unit
 *       synthetic worlds with synthetic rules tables; prints
 *       "<n> unit checks passed".
 *   wp3_test hexcost SEED COUNT
 *       mod_hex_cost on random adjacent tiles, designs and factions against
 *       the terrain_price.h oracle (OpenSMACX hex_cost arithmetic); prints
 *       "<n> hex cost cases matched".
 *   wp3_test budget SEED COUNT
 *       one order_veh step on random terrain, spent moves, retry flag, RNG
 *       seed and owner, against the move_budget oracle (resolve_move_budget):
 *       position, moves_spent, retry flag and RNG state must agree; prints
 *       "<n> move budget cases matched (<m> fractional failures)".
 */
#include "../engine.h"
#include "../veh_action.h"
#include "../wp3_deps.h"

#include "../../terrain_price.h"
#include "../../move_budget.h"

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

static const int RoadRate = 3;
static const int PlayerFaction = 1; // human
static const int AiFaction = 2;
static const int OtherFaction = 3;

/* Faction prototype slots used by the tests (faction 1 designs). */
enum {
    U_ROVER = 64 * 1 + 0,      // speeder, laser
    U_HOVER = 64 * 1 + 1,      // hovertank, laser
    U_JET = 64 * 1 + 2,        // needlejet, laser (range 2)
    U_COPTER = 64 * 1 + 3,     // copter, laser (range 1)
    U_GRAV = 64 * 1 + 4,       // gravship, laser (no range)
    U_CRUISER = 64 * 1 + 5,    // cruiser, laser
    U_PSI = 64 * 1 + 6,        // infantry, psi attack
    U_SUPER_FORMER = 64 * 1 + 7, // infantry formers with super terraformer
    U_ROVER_AI = 64 * 2 + 0,   // faction 2 speeder
    U_AIR_TRANSPORT = 64 * 1 + 8, // needlejet transport
    U_FUNGICIDE = 64 * 1 + 9,  // formers with fungicidal tanks
    U_AMPHI = 64 * 1 + 10,     // infantry, amphibious pods
    U_DROP = 64 * 1 + 11,      // infantry, drop pods
};

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    config_vanilla(conf);
    return s;
}

static void set_unit(int id, const char* name, int chassis, int weapon, int plan,
int carry = 0, uint32_t abilities = 0, int reactor = REC_FISSION) {
    UNIT* u = &Units[id];
    memset(u, 0, sizeof(UNIT));
    snprintf(u->name, sizeof(u->name), "%s", name);
    u->chassis_id = (uint8_t)chassis;
    u->weapon_id = (uint8_t)weapon;
    u->armor_id = ARM_NO_ARMOR;
    u->reactor_id = (uint8_t)reactor;
    u->carry_capacity = (uint8_t)carry;
    u->ability_flags = abilities;
    u->cost = 3;
    u->plan = (int8_t)plan;
    u->unit_flags = UNIT_ACTIVE | UNIT_PROTOTYPED;
    u->preq_tech = TECH_None;
}

/* Synthetic rules: alpha.txt-like values for every table WP3 reads. The
 * rules loader (WP0b) is not needed; only what the tests rely on is set. */
static void setup_rules() {
    // Integration: WP4's mod_say_morale2 (replacing the WP3 fallback) reads the
    // morale names, which the rules loader never leaves null.
    static char morale_name[] = "";
    for (int i = 0; i < MaxMoraleNum; i++) {
        Morale[i].name = morale_name;
        Morale[i].name_lifecycle = morale_name;
    }
    Rules->move_rate_roads = RoadRate;
    Rules->max_airdrop_rng_wo_orbital_insert = 8;
    Rules->artillery_max_rng = 2;
    Rules->minerals_harvesting_forest = 5;
    for (int i = 0; i < 3; i++) {
        Rules->psi_combat_ratio_atk[i] = 1;
        Rules->psi_combat_ratio_def[i] = 1;
    }
    struct { int speed, triad, range, cargo, missile; } chs[] = {
        {1, TRIAD_LAND, 0, 1, 0}, {2, TRIAD_LAND, 0, 1, 0}, {3, TRIAD_LAND, 0, 1, 0},
        {4, TRIAD_SEA, 0, 4, 0}, {6, TRIAD_SEA, 0, 8, 0}, {8, TRIAD_AIR, 2, 1, 0},
        {8, TRIAD_AIR, 1, 1, 0}, {8, TRIAD_AIR, 0, 1, 0}, {12, TRIAD_AIR, 1, 1, 1},
    };
    for (int i = 0; i < MaxChassisNum; i++) {
        Chassis[i].speed = (uint8_t)chs[i].speed;
        Chassis[i].triad = (uint8_t)chs[i].triad;
        Chassis[i].range = (uint8_t)chs[i].range;
        Chassis[i].cargo = (uint8_t)chs[i].cargo;
        Chassis[i].missile = (uint8_t)chs[i].missile;
        Chassis[i].cost = (uint8_t)(i + 1);
        Chassis[i].preq_tech = TECH_None;
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        Weapon[i].offense_value = (int8_t)(i + 1);
        Weapon[i].mode = WMODE_PROJECTILE;
        Weapon[i].cost = (uint8_t)(i + 1);
        Weapon[i].preq_tech = TECH_None;
    }
    Weapon[WPN_PSI_ATTACK].offense_value = -1;
    struct { int id, mode; } modes[] = {
        {WPN_COLONY_MODULE, WMODE_COLONY}, {WPN_TERRAFORMING_UNIT, WMODE_TERRAFORM},
        {WPN_TROOP_TRANSPORT, WMODE_TRANSPORT}, {WPN_SUPPLY_TRANSPORT, WMODE_SUPPLY},
        {WPN_PROBE_TEAM, WMODE_PROBE}, {WPN_ALIEN_ARTIFACT, WMODE_ARTIFACT},
    };
    for (auto& m : modes) {
        Weapon[m.id].mode = (uint8_t)m.mode;
        Weapon[m.id].offense_value = 0;
    }
    for (int i = 0; i < MaxArmorNum; i++) {
        Armor[i].defense_value = (int8_t)(i + 1);
        Armor[i].cost = (uint8_t)(i + 1);
        Armor[i].preq_tech = TECH_None;
    }
    for (int i = 0; i < MaxReactorNum; i++) {
        Reactor[i].preq_tech = TECH_None;
        Reactor[i].power = (int16_t)(i + 1);
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        Ability[i].preq_tech = TECH_None;
    }
    // alpha.txt #TERRAFORM rates (turns of work for one former).
    static const int rates[MaxTerrainNum] = {
        4, 8, 8, 4, 4, 1, 3, 5, 10, 4, 6, 6, 12, 12, 24, 18, 12, 12, 8, 5,
    };
    for (int i = 0; i < MaxTerrainNum; i++) {
        Terraform[i].preq_tech = TECH_None;
        Terraform[i].preq_tech_sea = TECH_None;
        Terraform[i].rate = rates[i];
        Terraform[i].bit = TerraformRules[i][0];
        Terraform[i].bit_incompatible = TerraformRules[i][1];
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        memset(&Units[i], 0, sizeof(UNIT));
        Units[i].preq_tech = TECH_Disable;
    }
    set_unit(BSC_COLONY_POD, "Colony Pod", CHS_INFANTRY, WPN_COLONY_MODULE, PLAN_COLONY);
    set_unit(BSC_FORMERS, "Formers", CHS_INFANTRY, WPN_TERRAFORMING_UNIT, PLAN_TERRAFORM);
    set_unit(BSC_SCOUT_PATROL, "Scout Patrol", CHS_INFANTRY, WPN_HAND_WEAPONS, PLAN_DEFENSE);
    set_unit(BSC_TRANSPORT_FOIL, "Transport Foil", CHS_FOIL, WPN_TROOP_TRANSPORT,
        PLAN_NAVAL_TRANSPORT, 4);
    set_unit(BSC_SEA_FORMERS, "Sea Formers", CHS_FOIL, WPN_TERRAFORMING_UNIT, PLAN_TERRAFORM);
    set_unit(BSC_SUPPLY_CRAWLER, "Supply Crawler", CHS_INFANTRY, WPN_SUPPLY_TRANSPORT, PLAN_SUPPLY);
    set_unit(BSC_PROBE_TEAM, "Probe Team", CHS_SPEEDER, WPN_PROBE_TEAM, PLAN_PROBE);
    set_unit(BSC_ALIEN_ARTIFACT, "Alien Artifact", CHS_INFANTRY, WPN_ALIEN_ARTIFACT, PLAN_ARTIFACT);
    set_unit(BSC_MIND_WORMS, "Mind Worms", CHS_INFANTRY, WPN_PSI_ATTACK, PLAN_OFFENSE);
    set_unit(BSC_ISLE_OF_THE_DEEP, "Isle of the Deep", CHS_FOIL, WPN_PSI_ATTACK, PLAN_OFFENSE, 4);
    set_unit(BSC_SEALURK, "Sealurk", CHS_FOIL, WPN_PSI_ATTACK, PLAN_OFFENSE);
    set_unit(BSC_SPORE_LAUNCHER, "Spore Launcher", CHS_INFANTRY, WPN_PSI_ATTACK, PLAN_OFFENSE);
    set_unit(BSC_LOCUSTS_OF_CHIRON, "Locusts", CHS_NEEDLEJET, WPN_PSI_ATTACK, PLAN_OFFENSE);
    set_unit(U_ROVER, "Rover", CHS_SPEEDER, WPN_LASER, PLAN_OFFENSE);
    set_unit(U_HOVER, "Hovertank", CHS_HOVERTANK, WPN_LASER, PLAN_OFFENSE);
    set_unit(U_JET, "Needlejet", CHS_NEEDLEJET, WPN_LASER, PLAN_AIR_SUPERIORITY);
    set_unit(U_COPTER, "Copter", CHS_COPTER, WPN_LASER, PLAN_OFFENSE);
    set_unit(U_GRAV, "Gravship", CHS_GRAVSHIP, WPN_LASER, PLAN_OFFENSE);
    set_unit(U_CRUISER, "Cruiser", CHS_CRUISER, WPN_LASER, PLAN_NAVAL_SUPERIORITY);
    set_unit(U_PSI, "Psi Infantry", CHS_INFANTRY, WPN_PSI_ATTACK, PLAN_OFFENSE);
    set_unit(U_SUPER_FORMER, "Super Formers", CHS_INFANTRY, WPN_TERRAFORMING_UNIT,
        PLAN_TERRAFORM, 0, ABL_SUPER_TERRAFORMER);
    set_unit(U_ROVER_AI, "Rover", CHS_SPEEDER, WPN_LASER, PLAN_OFFENSE);
    set_unit(U_AIR_TRANSPORT, "Air Transport", CHS_NEEDLEJET, WPN_TROOP_TRANSPORT,
        PLAN_NAVAL_TRANSPORT, 2);
    set_unit(U_FUNGICIDE, "Fungicide Formers", CHS_INFANTRY, WPN_TERRAFORMING_UNIT,
        PLAN_TERRAFORM, 0, ABL_FUNGICIDAL);
    set_unit(U_AMPHI, "Amphibious", CHS_INFANTRY, WPN_LASER, PLAN_OFFENSE, 0, ABL_AMPHIBIOUS);
    set_unit(U_DROP, "Drop Infantry", CHS_INFANTRY, WPN_LASER, PLAN_OFFENSE, 0, ABL_DROP_POD);
    for (int i = 0; i < SP_ID_Last - SP_ID_First + 1; i++) {
        SecretProjects[i] = SP_Unbuilt;
    }
}

static void set_tile(int x, int y, int alt, uint32_t items = 0, int rocky = 0) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        fprintf(stderr, "set_tile: (%d,%d) is off the %dx%d map\n", x, y, *MapAreaX, *MapAreaY);
        abort();
    }
    sq->climate = (uint8_t)((alt << 5) | (sq->climate & 0x1F));
    sq->val3 = (uint8_t)((rocky << 6) | (sq->val3 & 0x3F));
    sq->items = (sq->items & (BIT_BASE_IN_TILE | BIT_VEH_IN_TILE)) | items;
    sq->region = (uint8_t)(alt >= ALT_SHORE_LINE ? 1 : 65);
}

/* Land world (alt 3, rolling, moist) with factions 1 (human), 2 and 3 (AI). */
static void setup_world(int width = 24, int height = 16) {
    setup_rules();
    *MapAreaX = width;
    *MapAreaY = height;
    CHECK_EQ(map_init(), 0);
    for (int y = 0; y < height; y++) {
        for (int x = y & 1; x < width; x += 2) {
            MAP* sq = mapsq(x, y);
            sq->climate = (uint8_t)((ALT_SHORE_LINE << 5) | TILE_MOIST);
            sq->val3 = 0;
            sq->items = 0;
            sq->region = 1;
            sq->owner = -1;
        }
    }
    for (int i = 1; i <= 3; i++) {
        Factions[i].target_x = -1;
        set_alive(i, true);
    }
    FactionStatus[0] = 1 << PlayerFaction;
    *CurrentPlayerFaction = PlayerFaction;
    MapWin->cOwner = PlayerFaction;
    *CurrentTurn = 10;
    *DiffLevel = 3;
    headless_sink()->clear();
    headless_sink()->default_choice = 0;
}

static int steps_to(int x1, int y1, int x2, int y2) {
    for (int d = 0; d < 8; d++) {
        if (wrap(x1 + BaseOffsetX[d]) == wrap(x2) && y1 + BaseOffsetY[d] == y2) {
            return d;
        }
    }
    return -1;
}

/* ---- speed ---- */
static void test_speed() {
    auto s = new_state();
    setup_world();
    CHECK_EQ(proto_speed(BSC_SCOUT_PATROL), 1 * RoadRate);
    CHECK_EQ(proto_speed(U_ROVER), 2 * RoadRate);
    CHECK_EQ(proto_speed(U_HOVER), 3 * RoadRate);
    CHECK_EQ(proto_speed(BSC_TRANSPORT_FOIL), (4 - 1) * RoadRate); // transport -1
    CHECK_EQ(proto_speed(U_CRUISER), 6 * RoadRate);
    CHECK_EQ(proto_speed(U_JET), (8 + 2 * REC_FISSION) * RoadRate); // air: + 2 * reactor
    CHECK_EQ(proto_speed(U_AIR_TRANSPORT), (8 + 2 - 1) / 2 * RoadRate);
    CHECK_EQ(proto_speed(BSC_FUNGAL_TOWER), 0);
    Units[U_ROVER].ability_flags = ABL_ANTIGRAV_STRUTS;
    CHECK_EQ(proto_speed(U_ROVER), 3 * RoadRate);
    Units[U_ROVER].ability_flags = 0;
    int v = veh_init(U_HOVER, PlayerFaction, 4, 4);
    CHECK_EQ(veh_speed(v, 0), 9);
    // Damage: moves scale with remaining hit points (10 per reactor level).
    Vehs[v].damage_taken = 5;
    CHECK_EQ(veh_speed(v, 0), (3 * 5 + 10 - 1) / 10 * RoadRate);
    Vehs[v].damage_taken = 9;
    CHECK_EQ(veh_speed(v, 0), 1 * RoadRate); // at least one move on land
    int ship = veh_init(U_CRUISER, PlayerFaction, 0, 0);
    Vehs[ship].damage_taken = 9;
    CHECK_EQ(veh_speed(ship, 0), 2 * RoadRate); // at least two at sea
    // Maritime Control Center: +2 moves at sea.
    int b = base_init(PlayerFaction, 10, 10);
    SecretProjects[FAC_MARITIME_CONTROL_CENTER - SP_ID_First] = b;
    Vehs[ship].damage_taken = 0;
    CHECK_EQ(veh_speed(ship, 0), 8 * RoadRate);
}

/* ---- hex cost against the OpenSMACX arithmetic ---- */
/* Test case generator (murmur3 finalizer over a Weyl sequence): every bit
 * is well mixed, so small moduli such as direction % 8 are uniform. */
static uint32_t lcg(uint32_t& seed) {
    seed += 0x9E3779B9u;
    uint32_t z = seed;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

static int run_hexcost(uint32_t seed, int count) {
    auto s = new_state();
    setup_world(16, 12);
    const int units[] = {BSC_SCOUT_PATROL, BSC_FORMERS, BSC_COLONY_POD, U_ROVER, U_HOVER,
        BSC_TRANSPORT_FOIL, U_CRUISER, U_JET, BSC_MIND_WORMS, BSC_SPORE_LAUNCHER, BSC_SEALURK,
        BSC_ISLE_OF_THE_DEEP, U_PSI, BSC_ALIEN_ARTIFACT, BSC_SUPPLY_CRAWLER, U_ROVER_AI};
    const uint32_t bits[] = {BIT_ROAD, BIT_MAGTUBE, BIT_BASE_IN_TILE, BIT_FUNGUS, BIT_RIVER,
        BIT_FOREST, BIT_FARM, BIT_MINE};
    int dome_base = base_init(PlayerFaction, 15, 11);
    int matched = 0;
    for (int n = 0; n < count; n++) {
        int x1, y1;
        do {
            y1 = 2 + lcg(seed) % (*MapAreaY - 4);
            x1 = lcg(seed) % *MapAreaX;
        } while ((x1 + y1) & 1);
        int d = lcg(seed) % 8;
        int x2 = wrap(x1 + BaseOffsetX[d]);
        int y2 = y1 + BaseOffsetY[d];
        int xy[2][2] = {{x1, y1}, {x2, y2}};
        for (auto& p : xy) {
            uint32_t items = 0;
            for (uint32_t b : bits) {
                if (lcg(seed) % 4 == 0) {
                    items |= b;
                }
            }
            int alt = lcg(seed) % 8;
            if (lcg(seed) % 2) {
                alt = ALT_SHORE_LINE + lcg(seed) % 3; // mostly land
            }
            set_tile(p[0], p[1], alt, items, lcg(seed) % 3);
        }
        int unit_id = units[lcg(seed) % (sizeof(units) / sizeof(units[0]))];
        int faction = lcg(seed) % 4;
        int planet = (int)(lcg(seed) % 3) - 1;
        bool dome = lcg(seed) % 4 == 0;
        int toggle = lcg(seed) % 4 == 0;
        Factions[faction].SE_planet = planet;
        SecretProjects[FAC_XENOEMPATHY_DOME - SP_ID_First] = dome ? dome_base : SP_Unbuilt;
        Bases[dome_base].faction_id = faction > 0 ? faction : PlayerFaction;
        bool has_dome = has_project(FAC_XENOEMPATHY_DOME, faction);

        MAP* a = mapsq(x1, y1);
        MAP* b = mapsq(x2, y2);
        alpha::MovementTile from{a->items, (uint8_t)(a->climate & 0xE0), (uint8_t)(a->val3 & 0xC0)};
        alpha::MovementTile to{b->items, (uint8_t)(b->climate & 0xE0), (uint8_t)(b->val3 & 0xC0)};
        UNIT* u = &Units[unit_id];
        alpha::MovementDesign design{(uint32_t)unit_id, u->ability_flags, (uint32_t)proto_speed(unit_id),
            u->chassis_id, u->triad(), u->offense_value(), u->plan,
            unit_id == BSC_SEALURK || unit_id == BSC_ISLE_OF_THE_DEEP,
            unit_id == BSC_MIND_WORMS || unit_id == BSC_SPORE_LAUNCHER};
        alpha::MovementContext ctx{faction, planet, RoadRate, has_dome, toggle != 0};
        bool diagonal = x_dist(x1, x2) == 1 && abs(y1 - y2) == 1;
        uint32_t expect = 0;
        alpha::LogicError err = alpha::terrain_step_price(from, to, diagonal, design, ctx, expect);
        CHECK(err == alpha::LogicError::ok);
        int got = mod_hex_cost(unit_id, faction, x1, y1, x2, y2, toggle);
        if ((uint32_t)got != expect) {
            fprintf(stderr, "hexcost case %d: unit %d faction %d (%d,%d)->(%d,%d) items %x->%x "
                "alt %d->%d rocky %d planet %d dome %d toggle %d: got %d expected %u\n",
                n, unit_id, faction, x1, y1, x2, y2, a->items, b->items, a->alt_level(),
                b->alt_level(), b->rocky_level(), planet, has_dome, toggle, got, expect);
            failures++;
        } else {
            matched++;
        }
        checks++;
    }
    printf("%d hex cost cases matched\n", matched);
    return failures ? 1 : 0;
}

/* ---- movement expenditure against move_budget ---- */
static int run_budget(uint32_t seed, int count) {
    auto s = new_state();
    setup_world(16, 12);
    const int units[] = {BSC_SCOUT_PATROL, BSC_FORMERS, U_ROVER, U_HOVER, BSC_MIND_WORMS,
        U_PSI, BSC_ALIEN_ARTIFACT, BSC_SUPPLY_CRAWLER};
    int matched = 0;
    int fractional = 0;
    for (int n = 0; n < count; n++) {
        // Fresh tiles every case: plain land with random terrain.
        for (int i = 0; i < *MapAreaTiles; i++) {
            (*MapTiles)[i].items &= ~(BIT_ROAD | BIT_FUNGUS | BIT_FOREST | BIT_RIVER | BIT_MAGTUBE);
            (*MapTiles)[i].val3 &= 0x3F;
        }
        int x1 = 6, y1 = 6;
        int d = lcg(seed) % 8;
        int x2 = wrap(x1 + BaseOffsetX[d]);
        int y2 = y1 + BaseOffsetY[d];
        uint32_t src_items = (lcg(seed) % 3 == 0 ? (uint32_t)BIT_ROAD : 0u) | (lcg(seed) % 4 == 0 ? (uint32_t)BIT_RIVER : 0u);
        uint32_t dst_items = (lcg(seed) % 3 == 0 ? (uint32_t)BIT_ROAD : 0u) | (lcg(seed) % 3 == 0 ? (uint32_t)BIT_FUNGUS : 0u)
            | (lcg(seed) % 4 == 0 ? (uint32_t)BIT_FOREST : 0u) | (lcg(seed) % 4 == 0 ? (uint32_t)BIT_RIVER : 0u);
        set_tile(x1, y1, ALT_SHORE_LINE, src_items, 0);
        set_tile(x2, y2, ALT_SHORE_LINE + lcg(seed) % 2, dst_items, lcg(seed) % 3);
        int faction = (lcg(seed) % 2) ? PlayerFaction : AiFaction;
        int unit_id = units[lcg(seed) % (sizeof(units) / sizeof(units[0]))];
        Factions[faction].SE_planet = (int)(lcg(seed) % 3) - 1;
        while (*VehCount > 0) {
            veh_kill(0);
        }
        int v = veh_init(unit_id, faction, x1, y1);
        CHECK_EQ(v, 0);
        int speed = veh_speed(v, 0);
        int spent = lcg(seed) % speed;
        Vehs[v].moves_spent = (uint8_t)spent;
        bool retry = lcg(seed) % 2;
        if (retry) {
            Vehs[v].state |= VSTATE_UNK_8000;
        } else {
            Vehs[v].state &= ~VSTATE_UNK_8000;
        }
        uint32_t rng = lcg(seed);
        random_reseed(rng);
        int cost = mod_hex_cost(unit_id, faction, x1, y1, x2, y2, 0);

        alpha::MoveBudget before{(uint32_t)speed, (uint32_t)spent,
            (uint32_t)(retry ? 0x8000u : 0u), rng};
        alpha::MoveBudgetInput in{};
        in.cost = (uint32_t)cost;
        in.road_rate = RoadRate;
        in.land = true;
        in.destination_fungus = mapsq(x2, y2)->is_fungus();
        in.native_unit = Vehs[v].is_native_unit();
        in.former_or_artifact = Units[unit_id].plan == PLAN_TERRAFORM || Units[unit_id].plan == PLAN_ARTIFACT;
        in.destination_occupied = false;
        in.human = is_human(faction);
        in.attack_check = true;
        in.execute = true;
        alpha::MoveBudgetResult out{};
        CHECK(alpha::resolve_move_budget(before, in, out) == alpha::LogicError::ok);

        order_veh(v, d, 3);
        bool ok = true;
        if (out.outcome == alpha::MoveBudgetOutcome::move) {
            ok = Vehs[v].x == x2 && Vehs[v].y == y2
                && Vehs[v].moves_spent == out.budget.spent
                && !(Vehs[v].state & VSTATE_UNK_8000);
        } else if (out.outcome == alpha::MoveBudgetOutcome::fractional_failure) {
            fractional++;
            ok = Vehs[v].x == x1 && Vehs[v].y == y1
                && Vehs[v].moves_spent >= speed
                && (Vehs[v].state & VSTATE_UNK_8000);
        } else {
            ok = false;
        }
        ok = ok && random_state() == out.budget.random_seed;
        if (!ok) {
            fprintf(stderr, "budget case %d: unit %d faction %d dir %d cost %d speed %d spent %d "
                "retry %d fungus %d: oracle outcome %d spent %u seed %u; engine at (%d,%d) spent %d "
                "retry %d seed %u\n", n, unit_id, faction, d, cost, speed, spent, retry,
                in.destination_fungus, (int)out.outcome, out.budget.spent, out.budget.random_seed,
                Vehs[v].x, Vehs[v].y, Vehs[v].moves_spent, !!(Vehs[v].state & VSTATE_UNK_8000),
                random_state());
            failures++;
        } else {
            matched++;
        }
        checks++;
    }
    printf("%d move budget cases matched (%d fractional failures)\n", matched, fractional);
    return failures ? 1 : 0;
}

/* ---- terraforming ---- */
struct TerraCase {
    int item;
    uint32_t start_items;
    int start_rocky;
    int expect_turns; // with one plain former (contribution 2 per turn)
};

static void test_terraform() {
    auto s = new_state();
    setup_world();
    const TerraCase cases[] = {
        {FORMER_FARM, 0, 0, 4},
        {FORMER_MINE, 0, 0, 8},
        {FORMER_SOLAR, 0, 0, 4},
        {FORMER_SOLAR, 0, 1, 4 + 4 * 1 / 2}, // rolling: +rate * rocky / 2
        {FORMER_FOREST, BIT_FARM, 0, 4},
        {FORMER_ROAD, 0, 0, 1},
        {FORMER_ROAD, BIT_RIVER, 2, 1 + 1 + 2}, // river +1, rocky +2
        {FORMER_ROAD, BIT_FOREST, 0, 1 + 2},
        {FORMER_BUNKER, 0, 0, 5},
        {FORMER_AIRBASE, 0, 0, 10},
        {FORMER_SENSOR, 0, 0, 4},
        {FORMER_REMOVE_FUNGUS, BIT_FUNGUS, 0, 6},
        {FORMER_CONDENSER, BIT_FARM, 0, 12},
        {FORMER_THERMAL_BORE, BIT_MINE, 0, 24},
        {FORMER_SOIL_ENR, BIT_FARM, 0, 8},
        {FORMER_MAGTUBE, BIT_ROAD, 0, 3},
        {FORMER_LEVEL_TERRAIN, 0, 2, 8},
    };
    int case_no = 0;
    for (const TerraCase& c : cases) {
        int x = 4 + 2 * (case_no % 9);
        int y = (case_no < 9 ? 2 : 12);
        case_no++;
        set_tile(x, y, ALT_SHORE_LINE, c.start_items, c.start_rocky);
        while (*VehCount > 0) {
            veh_kill(0);
        }
        int v = veh_init(BSC_FORMERS, PlayerFaction, x, y);
        // Ask with the remaining-turns query first (toggle 0).
        int item = c.item;
        if (item == FORMER_SOIL_ENR) {
            item = FORMER_FARM; // farm on a farm upgrades to the soil enricher
        } else if (item == FORMER_MAGTUBE) {
            item = FORMER_ROAD;
        }
        // Turns left after this one: (2 * turns - 2) / 2.
        CHECK_EQ(action_terraform(v, item, 0), c.expect_turns - 1);
        CHECK_EQ(order_terraform(v, item), 1);
        CHECK_EQ(Vehs[v].order, c.expect_turns > 1 ? c.item + VehOrderFormerFirst : ORDER_NONE);
        int turns = 1;
        for (; turns < 60 && Vehs[v].order == c.item + VehOrderFormerFirst; turns++) {
            Vehs[v].moves_spent = 0;
            action(v);
        }
        if (turns != c.expect_turns) {
            fprintf(stderr, "terraform item %d: %d turns, expected %d\n", c.item, turns, c.expect_turns);
        }
        CHECK_EQ(turns, c.expect_turns);
        MAP* sq = mapsq(x, y);
        if (c.item == FORMER_LEVEL_TERRAIN) {
            CHECK_EQ(sq->rocky_level(), c.start_rocky - 1);
        } else {
            CHECK((sq->items & Terraform[c.item].bit) == Terraform[c.item].bit);
            uint32_t removed = Terraform[c.item].bit_incompatible & ~Terraform[c.item].bit;
            CHECK(!(sq->items & removed));
        }
        CHECK_EQ(Vehs[v].order, ORDER_NONE);
    }
    // Remove fungus with fungicidal tanks takes half the time.
    set_tile(4, 8, ALT_SHORE_LINE, BIT_FUNGUS, 0);
    while (*VehCount > 0) {
        veh_kill(0);
    }
    int f = veh_init(U_FUNGICIDE, PlayerFaction, 4, 8);
    CHECK_EQ(order_terraform(f, FORMER_REMOVE_FUNGUS), 1);
    int turns = 1;
    for (; turns < 20 && Vehs[f].order; turns++) {
        action(f);
    }
    CHECK_EQ(turns, 3);
    CHECK(!(mapsq(4, 8)->items & BIT_FUNGUS));

    // Several formers on one tile pool their work; a super former counts double.
    set_tile(6, 8, ALT_SHORE_LINE, 0, 0);
    int f1 = veh_init(BSC_FORMERS, PlayerFaction, 6, 8);
    int f2 = veh_init(U_SUPER_FORMER, PlayerFaction, 6, 8);
    CHECK_EQ(contribution(f1, FORMER_MINE), 2);
    CHECK_EQ(contribution(f2, FORMER_MINE), 4);
    CHECK_EQ(order_terraform(f1, FORMER_MINE), 1); // 2 of 16
    CHECK_EQ(order_terraform(f2, FORMER_MINE), 1); // takes over f1's work: 2 + 4
    CHECK_EQ(Vehs[f1].movement_turns, 0);
    CHECK_EQ(Vehs[f2].movement_turns, 6);
    CHECK_EQ(action_terraform(f2, FORMER_MINE, 0), 2); // (16 - 6 - 6) / 6 rounded up
    action(f1);
    action(f2);
    CHECK(!(mapsq(6, 8)->items & BIT_MINE) || Vehs[f2].order == ORDER_NONE);
    for (int i = 0; i < 4 && (Vehs[f1].order || Vehs[f2].order); i++) {
        if (Vehs[f1].order) {
            action(f1);
        }
        if (Vehs[f2].order) {
            action(f2);
        }
    }
    CHECK(mapsq(6, 8)->items & BIT_MINE);
    CHECK_EQ(Vehs[f1].order, ORDER_NONE);
    CHECK_EQ(Vehs[f2].order, ORDER_NONE);

    // Raise and lower land change the altitude of the tile.
    set_tile(10, 10, ALT_SHORE_LINE + 1, 0, 0);
    int r = veh_init(BSC_FORMERS, PlayerFaction, 10, 10);
    int alt0 = mapsq(10, 10)->alt_level();
    CHECK_EQ(order_terraform(r, FORMER_RAISE_LAND), 1);
    for (int i = 0; i < 30 && Vehs[r].order; i++) {
        action(r);
    }
    CHECK_EQ(mapsq(10, 10)->alt_level(), alt0 + 1);
    CHECK_EQ(order_terraform(r, FORMER_LOWER_LAND), 1);
    for (int i = 0; i < 30 && Vehs[r].order; i++) {
        action(r);
    }
    CHECK_EQ(mapsq(10, 10)->alt_level(), alt0);

    // Illegal orders: land former on ocean, fungus removal without fungus,
    // non-former, missing technology.
    set_tile(14, 10, ALT_OCEAN_SHELF, 0, 0);
    int sea_f = veh_init(BSC_SEA_FORMERS, PlayerFaction, 14, 10);
    CHECK_EQ(order_terraform(sea_f, FORMER_MINE), 1); // sea mine (preq set)
    int land_f = veh_init(BSC_FORMERS, PlayerFaction, 16, 10);
    CHECK_EQ(order_terraform(land_f, FORMER_REMOVE_FUNGUS), 0);
    int scout = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 16, 10);
    CHECK_EQ(order_terraform(scout, FORMER_FARM), 0);
    Terraform[FORMER_BUNKER].preq_tech = TECH_Disable;
    CHECK_EQ(order_terraform(land_f, FORMER_BUNKER), 0);
    Terraform[FORMER_BUNKER].preq_tech = TECH_None;

    // terraform_cost (raise/lower energy price): grows with distance from
    // sea level and doubles on fungus.
    set_tile(18, 10, ALT_SHORE_LINE + 1, 0, 0);
    int c1 = terraform_cost(18, 10, PlayerFaction);
    set_tile(18, 10, ALT_SHORE_LINE + 2, 0, 0);
    int c2 = terraform_cost(18, 10, PlayerFaction);
    set_tile(18, 10, ALT_SHORE_LINE + 2, BIT_FUNGUS, 0);
    int c3 = terraform_cost(18, 10, PlayerFaction);
    CHECK_EQ(c1, (1 + 2) * (1 + 2) * 2 / 2);
    CHECK_EQ(c2, (2 + 2) * (2 + 2) * 2 / 2);
    CHECK_EQ(c3, (2 + 2) * (2 + 2) * 6 / 2);
}

/* ---- base building ---- */
static void test_build_base() {
    auto s = new_state();
    setup_world();
    int pod = veh_init(BSC_COLONY_POD, PlayerFaction, 8, 6);
    int base_id = order_build_base(pod, "First Landing");
    CHECK_EQ(base_id, 0);
    CHECK_EQ(*BaseCount, 1);
    CHECK_EQ(*VehCount, 0); // pod consumed
    CHECK(mapsq(8, 6)->is_base());
    CHECK_EQ(Bases[0].faction_id, PlayerFaction);
    CHECK(strcmp(Bases[0].name, "First Landing") == 0);
    CHECK_EQ(Factions[PlayerFaction].last_base_turn, *CurrentTurn);
    // Next to a base, on the pole, with a non-colony unit: refused.
    int pod2 = veh_init(BSC_COLONY_POD, PlayerFaction, 9, 7);
    CHECK_EQ(order_build_base(pod2, nullptr), -1);
    veh_put(pod2, 4, 0);
    CHECK_EQ(order_build_base(pod2, nullptr), -1);
    int scout = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 14, 6);
    CHECK_EQ(order_build_base(scout, nullptr), -1);
    // Land pod on ocean refused; rocky tile is flattened to rolling.
    set_tile(14, 10, ALT_OCEAN_SHELF);
    veh_put(pod2, 14, 10);
    CHECK_EQ(order_build_base(pod2, nullptr), -1);
    set_tile(16, 10, ALT_SHORE_LINE + 1, 0, 2);
    veh_put(pod2, 16, 10);
    int b2 = order_build_base(pod2, nullptr);
    CHECK_EQ(b2, 1);
    CHECK_EQ(mapsq(16, 10)->rocky_level(), 1);
}

/* ---- movement, ZOC, paths ---- */
static void test_move_and_zoc() {
    auto s = new_state();
    setup_world();
    int v = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 6, 6);
    int d = steps_to(6, 6, 7, 7);
    CHECK_EQ(order_move(v, d), 1);
    CHECK_EQ(Vehs[v].x, 7);
    CHECK_EQ(Vehs[v].y, 7);
    CHECK_EQ(Vehs[v].moves_spent, RoadRate);
    CHECK_EQ(order_move(v, d), 0); // no moves left
    CHECK_EQ(Vehs[v].x, 7);
    // Roads: one third of a move each.
    Vehs[v].moves_spent = 0;
    set_tile(7, 7, ALT_SHORE_LINE, BIT_ROAD);
    set_tile(8, 8, ALT_SHORE_LINE, BIT_ROAD);
    set_tile(9, 9, ALT_SHORE_LINE, BIT_ROAD);
    CHECK_EQ(order_move(v, d), 1);
    CHECK_EQ(order_move(v, d), 1);
    CHECK_EQ(Vehs[v].x, 9);
    CHECK_EQ(Vehs[v].moves_spent, 2);
    // Land unit cannot walk into the ocean.
    set_tile(10, 10, ALT_OCEAN_SHELF);
    CHECK_EQ(order_move(v, d), 0);
    CHECK_EQ(Vehs[v].x, 9);

    // Zone of control: an enemy unit next to both tiles blocks the move.
    auto s2 = new_state();
    setup_world();
    Factions[PlayerFaction].diplo_status[AiFaction] = DIPLO_VENDETTA;
    Factions[AiFaction].diplo_status[PlayerFaction] = DIPLO_VENDETTA;
    int me = veh_init(U_ROVER, PlayerFaction, 6, 6);
    veh_init(U_ROVER_AI, AiFaction, 8, 6);
    CHECK(zoc_any(6, 6, PlayerFaction));
    CHECK(zoc_move(6, 6, PlayerFaction));
    CHECK(zoc_move(7, 7, PlayerFaction));
    CHECK(!zoc_move(4, 6, PlayerFaction));
    CHECK_EQ(order_move(me, steps_to(6, 6, 7, 7)), 0);
    CHECK_EQ(Vehs[me].x, 6); // blocked by ZOC
    CHECK_EQ(Vehs[me].y, 6);
    CHECK_EQ(order_move(me, steps_to(6, 6, 5, 7)), 1);
    CHECK_EQ(Vehs[me].x, 5); // leaving the zone is allowed
    // Path search goes around the zone.
    Vehs[me].moves_spent = 0;
    veh_put(me, 6, 6);
    int first = Path_find(6, 6, 10, 6, U_ROVER, PlayerFaction, 0, -1);
    CHECK(first >= 0);
    int nx = wrap(6 + BaseOffsetX[first]);
    int ny = 6 + BaseOffsetY[first];
    CHECK(!(zoc_move(nx, ny, PlayerFaction) && zoc_move(6, 6, PlayerFaction)));
    CHECK(Path_find(6, 6, 10, 6, U_ROVER, PlayerFaction, PATH_IGNORE_ZOC, -1) >= 0);
    // Land path around a sea channel; no land path to an island.
    auto s3 = new_state();
    setup_world();
    for (int y = 0; y < 14; y++) {
        for (int x = 10 + (y & 1); x <= 12; x += 2) {
            set_tile(x, y, ALT_OCEAN_SHELF);
        }
    }
    int walker = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 4, 4);
    CHECK(Path_find(4, 4, 18, 4, BSC_SCOUT_PATROL, PlayerFaction, 0, -1) >= 0);
    CHECK(Path_find(4, 4, 11, 3, BSC_SCOUT_PATROL, PlayerFaction, 0, -1) < 0);
    // order_go_to walks there one tile a turn. On a round planet the
    // shortest way wraps west around the map edge: 4, 2, 0, 22, 20, 18.
    CHECK_EQ(order_go_to(walker, 18, 4), 1);
    int turns = 1;
    for (; turns < 40 && (Vehs[walker].x != 18 || Vehs[walker].y != 4); turns++) {
        Vehs[walker].moves_spent = 0;
        CHECK(order_continue(walker));
        CHECK(!is_ocean(mapsq(Vehs[walker].x, Vehs[walker].y)));
    }
    CHECK_EQ(Vehs[walker].x, 18);
    CHECK_EQ(Vehs[walker].y, 4);
    CHECK_EQ(Vehs[walker].order, ORDER_NONE);
    CHECK_EQ(turns, 5);
    // On a flat map it must go around the south end of the channel (rows 14, 15).
    *MapToggleFlat |= 1;
    CHECK_EQ(order_go_to(walker, 4, 4), 1);
    int max_y = Vehs[walker].y;
    for (turns = 1; turns < 60 && (Vehs[walker].x != 4 || Vehs[walker].y != 4); turns++) {
        Vehs[walker].moves_spent = 0;
        CHECK(order_continue(walker));
        CHECK(!is_ocean(mapsq(Vehs[walker].x, Vehs[walker].y)));
        max_y = max(max_y, (int)Vehs[walker].y);
    }
    CHECK_EQ(Vehs[walker].x, 4);
    CHECK_EQ(Vehs[walker].y, 4);
    CHECK(max_y >= 14);
    CHECK(turns >= 17 && turns <= 22);
    // An unreachable go-to is refused.
    CHECK_EQ(order_go_to(walker, 11, 3), 0);
}

/* ---- transports ---- */
static void test_transport() {
    auto s = new_state();
    setup_world();
    for (int y = 0; y < 16; y++) {
        for (int x = 10 - (y & 1); x < 24; x += 2) {
            set_tile(x, y, ALT_OCEAN_SHELF);
        }
    }
    set_tile(20, 6, ALT_SHORE_LINE); // island
    int foil = veh_init(BSC_TRANSPORT_FOIL, PlayerFaction, 10, 6);
    int scout = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 8, 6);
    CHECK_EQ(veh_cargo(foil), 4);
    CHECK_EQ(cargo_capacity(10, 6, PlayerFaction), 4);
    CHECK_EQ(veh_cargo_loaded(foil), 0);
    // Moving onto the transport boards it.
    CHECK_EQ(order_move(scout, steps_to(8, 6, 10, 6)), 1);
    CHECK_EQ(Vehs[scout].x, 10);
    CHECK_EQ(Vehs[scout].order, ORDER_SENTRY_BOARD); // sleep(): waits for a transport
    CHECK_EQ(Vehs[scout].waypoint_x[0], -1);
    CHECK(has_transport(10, 6, PlayerFaction));
    // The transport takes it aboard (stack_veh) and carries it; 3 moves a turn.
    for (int i = 0; i < 4; i++) {
        if (i == 3) {
            Vehs[foil].moves_spent = 0;
        }
        CHECK_EQ(order_move(foil, steps_to(Vehs[foil].x, 6, Vehs[foil].x + 2, 6)), 1);
        CHECK_EQ(Vehs[scout].waypoint_x[0], foil);
        CHECK_EQ(veh_cargo_loaded(foil), 1);
        CHECK_EQ(cargo_capacity(Vehs[foil].x, 6, PlayerFaction), 3);
    }
    CHECK_EQ(Vehs[foil].x, 18);
    CHECK_EQ(Vehs[scout].x, 18);
    CHECK_EQ(Vehs[scout].y, 6);
    // Unload onto the island.
    Vehs[scout].moves_spent = 0;
    CHECK_EQ(order_move(scout, steps_to(18, 6, 20, 6)), 1);
    CHECK_EQ(Vehs[scout].x, 20);
    CHECK_EQ(veh_cargo_loaded(foil), 0);
    CHECK(Vehs[scout].order != ORDER_SENTRY_BOARD);
    // Sentry inside a stack with a transport boards it.
    int s2 = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 18, 6);
    CHECK_EQ(order_sentry(s2), 1);
    CHECK_EQ(Vehs[s2].order, ORDER_SENTRY_BOARD);
    CHECK_EQ(Vehs[s2].waypoint_x[0], foil);
    CHECK_EQ(veh_cargo_loaded(foil), 1);
}

/* ---- air units and fuel ---- */
static void test_air() {
    auto s = new_state();
    setup_world();
    int b = base_init(PlayerFaction, 4, 8);
    int jet = veh_init(U_JET, PlayerFaction, 4, 8);
    CHECK_EQ(Vehs[jet].home_base_id, b);
    int speed = veh_speed(jet, 0);
    CHECK_EQ(speed, 30);
    // First turn out: fuel turn counter goes up when moves run out.
    for (int i = 0; i < speed / RoadRate; i++) {
        CHECK_EQ(order_move(jet, steps_to(Vehs[jet].x, 8, Vehs[jet].x + 2, 8)), 1);
    }
    CHECK_EQ(Vehs[jet].x, 4 + 2 * 10 - *MapAreaX);
    CHECK_EQ(Vehs[jet].movement_turns, 1);
    CHECK_EQ(*VehCount, 1);
    // Second turn out of an airbase: the needlejet (range 2) crashes.
    Vehs[jet].moves_spent = 0;
    int start_x = Vehs[jet].x;
    for (int i = 0; i < speed / RoadRate && *VehCount == 1; i++) {
        order_move(jet, steps_to(Vehs[jet].x, 8, wrap(Vehs[jet].x + 2), 8));
    }
    CHECK_EQ(*VehCount, 0);
    (void)start_x;
    // An airbase refuels: ending the move there resets the counter.
    int jet2 = veh_init(U_JET, PlayerFaction, 4, 8);
    set_tile(10, 8, ALT_SHORE_LINE, BIT_AIRBASE);
    Vehs[jet2].movement_turns = 1;
    for (int i = 0; i < 3; i++) {
        order_move(jet2, steps_to(Vehs[jet2].x, 8, Vehs[jet2].x + 2, 8));
    }
    CHECK_EQ(Vehs[jet2].x, 10);
    Vehs[jet2].moves_spent = (uint8_t)speed;
    CHECK(Vehs[jet2].movement_turns <= 1);
    // A copter (range 1) ending its turn away from bases and airbases (x = 0)
    // loses 3 * reactor hit points instead of crashing.
    int cop = veh_init(U_COPTER, PlayerFaction, 4, 8);
    for (int i = 0; i < 10 && Vehs[cop].moves_spent < veh_speed(cop, 0); i++) {
        order_move(cop, steps_to(Vehs[cop].x, 8, wrap(Vehs[cop].x + 2), 8));
    }
    CHECK_EQ(Vehs[cop].x, 0);
    CHECK_EQ(Vehs[cop].damage_taken, 3 * REC_FISSION);
    CHECK(Vehs[cop].damage_taken < Vehs[cop].max_hitpoints());
    // Gravships have unlimited range.
    int grav = veh_init(U_GRAV, PlayerFaction, 4, 8);
    for (int i = 0; i < 10; i++) {
        order_move(grav, steps_to(Vehs[grav].x, 8, wrap(Vehs[grav].x + 2), 8));
    }
    CHECK_EQ(Vehs[grav].damage_taken, 0);
    CHECK_EQ(Vehs[grav].movement_turns, 0);
}

/* ---- supply pods, monolith, visibility ---- */
static void test_goody_monolith_spot() {
    auto s = new_state();
    setup_world();
    int b = base_init(PlayerFaction, 4, 4);
    (void)b;
    // A supply pod is opened by moving onto it and then removed.
    set_tile(9, 7, ALT_SHORE_LINE, BIT_SUPPLY_POD);
    CHECK(mod_goody_at(9, 7));
    int v = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 8, 6);
    random_reseed(12345);
    game_srand(777);
    int credits = Factions[PlayerFaction].energy_credits;
    int units = *VehCount;
    int bases = *BaseCount;
    CHECK_EQ(order_move(v, steps_to(8, 6, 9, 7)), 1);
    CHECK(mapsq(9, 7)->items & BIT_SUPPLY_REMOVE); // opened
    CHECK(!mod_goody_at(9, 7));
    // Something happened: credits, units, a base, map or technology change;
    // the pod popup was shown in any case.
    bool popup = false;
    for (auto& line : headless_sink()->log) {
        popup |= line.find("GOODY") != std::string::npos || line.find("POD") != std::string::npos
            || line.find("CREDITS") != std::string::npos || line.find("FREE") != std::string::npos;
    }
    CHECK(popup || Factions[PlayerFaction].energy_credits != credits || *VehCount != units
        || *BaseCount != bases);
    // Many pods over many seeds: every outcome leaves a consistent world.
    for (int i = 0; i < 40; i++) {
        int px = 12 + 2 * (i % 5);
        int py = 2 + 2 * (i / 5) % 12;
        while (*VehCount > 0) {
            veh_kill(0);
        }
        set_tile(px, py, ALT_SHORE_LINE, BIT_SUPPLY_POD);
        set_tile(px - 1, py + 1, ALT_SHORE_LINE, 0);
        int w = veh_init(BSC_SCOUT_PATROL, PlayerFaction, px - 1, py + 1);
        random_reseed(1000 + i);
        game_srand(2000 + i);
        order_move(w, steps_to(px - 1, py + 1, px, py));
        CHECK(!mod_goody_at(px, py));
        for (int k = 0; k < *VehCount; k++) {
            CHECK(Vehs[k].unit_id >= 0 && Vehs[k].unit_id < MaxProtoNum);
            CHECK(mapsq(Vehs[k].x, Vehs[k].y) != nullptr);
        }
    }
    // Monolith: heals and promotes once.
    while (*VehCount > 0) {
        veh_kill(0);
    }
    set_tile(15, 9, ALT_SHORE_LINE, BIT_MONOLITH);
    int m = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 14, 8);
    Vehs[m].damage_taken = 5;
    Vehs[m].morale = MORALE_GREEN;
    CHECK(want_monolith(m));
    headless_sink()->choices["MONOLITH"] = 1;
    CHECK_EQ(order_move(m, steps_to(14, 8, 15, 9)), 1);
    CHECK_EQ(Vehs[m].x, 15);
    CHECK_EQ(Vehs[m].damage_taken, 0);
    CHECK_EQ(Vehs[m].morale, MORALE_GREEN + 1);
    CHECK(Vehs[m].state & VSTATE_MONOLITH_UPGRADED);
    CHECK(!want_monolith(m));
    // Visibility: a unit spots the tiles around it.
    auto s2 = new_state();
    setup_world();
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].visibility = 0;
    }
    int e = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 10, 6);
    spot_all(e, 0);
    CHECK(mapsq(10, 6)->is_visible(PlayerFaction));
    CHECK(mapsq(11, 7)->is_visible(PlayerFaction));
    CHECK(mapsq(12, 6)->is_visible(PlayerFaction));
    CHECK(!mapsq(16, 6)->is_visible(PlayerFaction));
    CHECK(!mapsq(10, 6)->is_visible(AiFaction));
    int ai = veh_init(BSC_SCOUT_PATROL, AiFaction, 14, 6);
    (void)ai;
    spot_loc(14, 6, PlayerFaction);
    CHECK(mapsq(14, 6)->is_visible(PlayerFaction));
}

/* ---- player orders: sentry, hold, wake, disband, home, upgrade, convoy ---- */
static void test_orders() {
    auto s = new_state();
    setup_world();
    int b0 = base_init(PlayerFaction, 4, 4);
    int b1 = base_init(PlayerFaction, 14, 4);
    int v = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 4, 4);
    CHECK_EQ(Vehs[v].home_base_id, b0);
    CHECK_EQ(order_hold(v), 1);
    CHECK_EQ(Vehs[v].order, ORDER_HOLD);
    CHECK_EQ(order_wake(v), 1);
    CHECK_EQ(Vehs[v].order, ORDER_NONE);
    CHECK_EQ(order_sentry(v), 1);
    CHECK_EQ(Vehs[v].order, ORDER_SENTRY_BOARD);
    CHECK_EQ(order_wake(v), 1);
    CHECK_EQ(Vehs[v].order, ORDER_NONE);
    // Set home / return home.
    veh_put(v, 14, 4);
    CHECK_EQ(order_set_home(v), 1);
    CHECK_EQ(Vehs[v].home_base_id, b1);
    CHECK_EQ(order_set_home(v), 0); // already home
    veh_put(v, 18, 4);
    Vehs[v].moves_spent = 0;
    CHECK_EQ(order_return_home(v), 1);
    CHECK_EQ(Vehs[v].x, 16);
    CHECK_EQ(Vehs[v].waypoint_x[0], 14);
    Vehs[v].moves_spent = 0;
    CHECK(order_continue(v));
    CHECK_EQ(Vehs[v].x, 14);
    // Disband in a base: half the mineral value goes into production.
    int minerals = Bases[b1].minerals_accumulated;
    int cost = mod_veh_cost(BSC_SCOUT_PATROL, b1, nullptr);
    int row = mod_cost_factor(PlayerFaction, RSC_MINERAL, b1);
    if (row <= 0) {
        row = 10;
    }
    int count = *VehCount;
    CHECK_EQ(order_disband(v), 1);
    CHECK_EQ(*VehCount, count - 1);
    CHECK_EQ(Bases[b1].minerals_accumulated, minerals + cost * row / 2);
    // Disband in the field: just removed.
    int w = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 10, 10);
    (void)w;
    count = *VehCount;
    minerals = Bases[b1].minerals_accumulated;
    CHECK_EQ(order_disband(*VehCount - 1), 1);
    CHECK_EQ(*VehCount, count - 1);
    CHECK_EQ(Bases[b1].minerals_accumulated, minerals);
    // Upgrade in a base for energy credits.
    int u = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 4, 4);
    Factions[PlayerFaction].energy_credits = 0;
    CHECK_EQ(order_upgrade(u, U_HOVER), 0); // no credits
    Factions[PlayerFaction].energy_credits = 1000;
    int price = 10 * mod_upgrade_cost(PlayerFaction, U_HOVER, BSC_SCOUT_PATROL);
    CHECK_EQ(order_upgrade(u, U_JET), 0); // other triad
    CHECK_EQ(order_upgrade(u, U_HOVER), 1);
    CHECK_EQ(Vehs[u].unit_id, U_HOVER);
    CHECK_EQ(Factions[PlayerFaction].energy_credits, 1000 - price);
    veh_put(u, 8, 8);
    CHECK_EQ(order_upgrade(u, U_ROVER), 0); // outside a base
    // Supply crawler convoy next to a base.
    int crawler = veh_init(BSC_SUPPLY_CRAWLER, PlayerFaction, 5, 5);
    CHECK_EQ(order_convoy(crawler, RES_MINERAL), 1);
    CHECK_EQ(Vehs[crawler].order, ORDER_CONVOY);
    CHECK_EQ(Vehs[crawler].order_auto_type, RES_MINERAL - 1);
    CHECK_EQ(order_convoy(v = veh_init(BSC_SCOUT_PATROL, PlayerFaction, 5, 5), RES_MINERAL), 0);
    // Supply crawler entering an own base: human answer 1 helps production.
    int crawler2 = veh_init(BSC_SUPPLY_CRAWLER, PlayerFaction, 13, 5);
    headless_sink()->choices["SUPPLYOPTIONS"] = 1;
    minerals = Bases[b1].minerals_accumulated;
    count = *VehCount;
    order_move(crawler2, steps_to(13, 5, 14, 4));
    CHECK_EQ(*VehCount, count - 1);
    CHECK(Bases[b1].minerals_accumulated > minerals);
    headless_sink()->choices.erase("SUPPLYOPTIONS");
    // ...answer 0 just moves in.
    int crawler3 = veh_init(BSC_SUPPLY_CRAWLER, PlayerFaction, 13, 5);
    count = *VehCount;
    order_move(crawler3, steps_to(13, 5, 14, 4));
    CHECK_EQ(*VehCount, count);
    CHECK_EQ(Vehs[*VehCount - 1].x, 14);
}

static int run_unit() {
    test_speed();
    test_terraform();
    test_build_base();
    test_move_and_zoc();
    test_transport();
    test_air();
    test_goody_monolith_spot();
    test_orders();
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "unit")) {
        return run_unit();
    }
    if (argc >= 4 && !strcmp(argv[1], "hexcost")) {
        return run_hexcost((uint32_t)strtoul(argv[2], nullptr, 0), atoi(argv[3]));
    }
    if (argc >= 4 && !strcmp(argv[1], "budget")) {
        return run_budget((uint32_t)strtoul(argv[2], nullptr, 0), atoi(argv[3]));
    }
    fprintf(stderr, "usage: wp3_test unit | hexcost SEED COUNT | budget SEED COUNT\n");
    return 2;
}
