/* SPDX-License-Identifier: GPL-3.0-or-later
 * Combined engine control: turn scheduler (WP1), base upkeep (WP2) and combat
 * (WP4) on one state, through a saved game.
 *
 *   integration_test make OUT
 *       builds a synthetic world with synthetic rules (no original assets):
 *       three factions (1 human, 2 and 3 computer), bases producing units and
 *       facilities, opposing units next to each other; writes it with
 *       save_daemon_bytes.
 *   integration_test run IN OUT TURNS
 *       sets the same synthetic rules, loads IN with load_daemon_bytes, then
 *       for each of TURNS calls: on even calls the human attacks the first
 *       enemy next to one of its combat units (engine_attack), then End Turn
 *       (engine_end_turn). Prints one line per attack and per call, a digest of
 *       all turn events and per-turn base state, and writes the final game with
 *       save_daemon_bytes. Output and saved bytes depend only on the input, so
 *       host and big-endian m68k runs must agree byte for byte.
 */
#include "engine.h"
#include "turn.h"
#include "base_wp2.h"
#include "combat.h"

#include <memory>
#include <string>
#include <vector>

using namespace alpha::engine;

static const int Human = 1;

static uint32_t fnv1a(const void* data, size_t n, uint32_t h) {
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

static uint32_t mix(uint32_t h, int32_t v) {
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    return fnv1a(b, 4, h);
}

static char empty_str[] = "";
static char morale_names[7][16] = {"Very Green", "Green", "Disciplined", "Hardened", "Veteran", "Commando", "Elite"};
static char facility_names[SP_ID_Last + 1][24];
static char weapon_names[MaxWeaponNum][24];
static char armor_names[MaxArmorNum][24];
static char chassis_names[MaxChassisNum][24];

/* Synthetic rules in the shape of alpha.txt (values chosen for the test, not
 * copied from the game data). Rules tables are not part of a saved game, so
 * both "make" and "run" set them. */
static void set_rules() {
    config_vanilla(conf);
    Rules->normal_start_year = 2100;
    Rules->move_rate_roads = 3;
    Rules->artillery_max_rng = 2;
    Rules->artillery_dmg_numerator = 3;
    Rules->artillery_dmg_denominator = 2;
    Rules->max_dmg_percent_arty_base_bunker = 50;
    Rules->max_dmg_percent_arty_open = 99;
    Rules->max_dmg_percent_arty_sea = 100;
    for (int i = 0; i < 3; i++) {
        Rules->psi_combat_ratio_atk[i] = 3;
        Rules->psi_combat_ratio_def[i] = 2;
    }
    Rules->nutrient_intake_req_citizen = 2;
    Rules->nutrient_cost_multi = 10;
    Rules->mineral_cost_multi = 10;
    Rules->min_base_size_specialists = 5;
    Rules->drones_induced_genejack_factory = 1;
    Rules->pop_limit_wo_hab_complex = 7;
    Rules->pop_limit_wo_hab_dome = 14;
    Rules->retool_strictness = RETOOL_FREE_PROJECT;
    Rules->retool_penalty_prod_change = 50;
    Rules->retool_exemption = 10;
    Rules->limit_mineral_inc_for_mine_wo_road = 1;
    Rules->nutrient_effect_mine_sq = -1;
    Rules->tech_preq_allow_3_nutrients_sq = TECH_Disable;
    Rules->tech_preq_allow_3_minerals_sq = TECH_Disable;
    Rules->tech_preq_allow_3_energy_sq = TECH_Disable;
    Rules->tech_preq_mining_platform_bonus = TECH_Disable;
    Rules->tech_preq_economic_victory = TECH_Disable;
    Rules->extra_cost_prototype_land = 50;
    Rules->extra_cost_prototype_sea = 50;
    Rules->extra_cost_prototype_air = 50;
    Rules->base_size_subspace_gen = 10;
    Rules->subspace_gen_req = 6;
    ResInfo->ocean_sq = {1, 0, 0, 0};
    ResInfo->base_sq = {2, 1, 1, 0};
    ResInfo->bonus_sq = {2, 1, 2, 0};
    ResInfo->forest_sq = {1, 2, 0, 0};
    ResInfo->recycling_tanks = {1, 1, 1, 0};
    ResInfo->improved_land = {1, 1, 0, 0};
    ResInfo->improved_sea = {1, 1, 2, 0};
    ResInfo->monolith_sq = {2, 2, 2, 0};
    ResInfo->borehole_sq = {0, 6, 6, 0};
    const int spc[3][3] = {{3, 0, 0}, {0, 0, 3}, {0, 2, 0}};
    for (int i = 0; i < MaxSpecialistNum; i++) {
        Citizen[i].preq_tech = i < 3 ? TECH_None : TECH_Disable;
        Citizen[i].obsol_tech = TECH_Disable;
        Citizen[i].econ_bonus = i < 3 ? spc[i][0] : 0;
        Citizen[i].psych_bonus = i < 3 ? spc[i][1] : 0;
        Citizen[i].labs_bonus = i < 3 ? spc[i][2] : 0;
    }
    for (int i = 0; i <= SP_ID_Last; i++) {
        snprintf(facility_names[i], sizeof(facility_names[i]), "Fac%d", i);
        Facility[i].name = facility_names[i];
        Facility[i].preq_tech = TECH_None;
        Facility[i].free_tech = TECH_Disable;
        Facility[i].cost = i < SP_ID_First ? 4 + i % 6 : 20;
        Facility[i].maint = i < SP_ID_First ? (i % 3) : 0;
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        Tech[i].preq_tech1 = TECH_Disable;
        Tech[i].preq_tech2 = TECH_Disable;
    }
    for (int i = 0; i < MaxProposalNum; i++) {
        Proposal[i].preq_tech = TECH_Disable;
    }
    const struct { int speed, triad, range, missile; } chs[] = {
        {1, TRIAD_LAND, 0, 0}, {2, TRIAD_LAND, 0, 0}, {3, TRIAD_LAND, 0, 0},
        {4, TRIAD_SEA, 0, 0}, {6, TRIAD_SEA, 0, 0}, {8, TRIAD_AIR, 2, 0},
        {8, TRIAD_AIR, 1, 0}, {8, TRIAD_AIR, 0, 0}, {12, TRIAD_AIR, 1, 1},
    };
    for (int i = 0; i < MaxChassisNum; i++) {
        snprintf(chassis_names[i], sizeof(chassis_names[i]), "Chassis%d", i);
        Chassis[i].offsv1_name = chassis_names[i];
        Chassis[i].speed = chs[i].speed;
        Chassis[i].triad = chs[i].triad;
        Chassis[i].range = chs[i].range;
        Chassis[i].missile = chs[i].missile;
        Chassis[i].cost = 1 + i / 3;
        Chassis[i].preq_tech = TECH_None;
    }
    const int off[] = {1, 2, 4, 5, 6, 8, 10, 12, 13, 16, 20, 24, 6, 12, 30, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const int mode[] = {0, 1, 0, 1, 2, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0,
        WMODE_COLONY, WMODE_TERRAFORM, WMODE_TRANSPORT, WMODE_SUPPLY, WMODE_PROBE, WMODE_ARTIFACT, 2, 2, 2};
    for (int i = 0; i < MaxWeaponNum; i++) {
        snprintf(weapon_names[i], sizeof(weapon_names[i]), "Weapon%d", i);
        Weapon[i].name = weapon_names[i];
        Weapon[i].name_short = weapon_names[i];
        Weapon[i].offense_value = off[i];
        Weapon[i].mode = mode[i];
        Weapon[i].cost = off[i] > 0 ? off[i] : 1;
        Weapon[i].preq_tech = TECH_None;
    }
    const int def[] = {1, 2, 3, 4, 5, 6, 8, 10, 12, -1, 3, 3, 8, 8};
    for (int i = 0; i < MaxArmorNum; i++) {
        snprintf(armor_names[i], sizeof(armor_names[i]), "Armor%d", i);
        Armor[i].name = armor_names[i];
        Armor[i].name_short = armor_names[i];
        Armor[i].defense_value = def[i];
        Armor[i].cost = def[i] > 0 ? def[i] : 1;
        Armor[i].preq_tech = TECH_None;
    }
    for (int i = 0; i < MaxMoraleNum; i++) {
        Morale[i].name = morale_names[i];
        Morale[i].name_lifecycle = morale_names[i];
    }
}

static void make_unit(int unit_id, int chassis, int weapon, int armor, int plan, int cost) {
    UNIT* u = &Units[unit_id];
    memset(u, 0, sizeof(*u));
    snprintf(u->name, sizeof(u->name), "Unit%d", unit_id);
    u->chassis_id = chassis;
    u->weapon_id = weapon;
    u->armor_id = armor;
    u->reactor_id = REC_FISSION;
    u->plan = plan;
    u->preq_tech = TECH_None;
    u->cost = cost;
    u->unit_flags = UNIT_ACTIVE | UNIT_PROTOTYPED;
}

static int make_base(int faction_id, int x, int y, int pop, int item, int minerals) {
    int id = base_init(faction_id, x, y);
    if (id < 0) {
        return id;
    }
    BASE* b = &Bases[id];
    b->pop_size = pop;
    b->governor_flags = 0;
    b->queue_items[0] = item;
    b->queue_size = 0;
    b->production_id_last = item;
    b->minerals_accumulated = minerals; // head start so items complete during the run
    b->minerals_accumulated_2 = minerals;
    b->worked_tiles = 0;
    set_base(id);
    base_compute(1);
    return id;
}

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    base_compute_state_reset();
    headless_sink()->clear();
    set_rules();
    return s;
}

static bool write_file(const char* path, const std::vector<uint8_t>& data) {
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }
    bool ok = fwrite(data.data(), 1, data.size(), fp) == data.size();
    return fclose(fp) == 0 && ok;
}

static std::vector<uint8_t> read_file(const char* path) {
    std::vector<uint8_t> data;
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return data;
    }
    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        data.insert(data.end(), buf, buf + n);
    }
    fclose(fp);
    return data;
}

static int run_make(const char* out) {
    auto s = new_state();
    *MapAreaX = 32;
    *MapAreaY = 20;
    if (map_init()) {
        return 10;
    }
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP* sq = &(*MapTiles)[i];
        sq->climate = ((ALT_SHORE_LINE + 1) << 5) | (2 << 3); // rainy land
        sq->val3 = (uint8_t)((sq->val3 & 0x3F) | (1 << 6));  // rolling
        sq->region = 1;
        sq->owner = -1;
    }
    for (int i = 1; i < 4; i++) {
        Factions[i].target_x = -1;
        Factions[i].diff_level = 3;
        Factions[i].base_id_attack_target = -1;
        Factions[i].player_flags |= PFLAG_MAP_REVEALED;
        snprintf(MFactions[i].noun_faction, sizeof(MFactions[i].noun_faction), "Noun%d", i);
        snprintf(MFactions[i].filename, sizeof(MFactions[i].filename), "faction%d", i);
        MFactions[i].rule_hurry = 100;
        set_alive(i, true);
        set_human(i, i == Human);
    }
    for (int a = 0; a < MaxPlayerNum; a++) {
        for (int b = 0; b < MaxPlayerNum; b++) {
            if (a != b) {
                Factions[a].diplo_status[b] = DIPLO_COMMLINK | DIPLO_VENDETTA;
            }
        }
    }
    *DiffLevel = 3;
    *MapSizePlanet = 2;
    *CurrentTurn = 10;
    *CurrentPlayerFaction = Human;
    MapWin->cOwner = Human;
    // Basic units and one design per faction (prototyped, no prototype cost).
    for (int i = 0; i < MaxProtoNum; i++) {
        memset(&Units[i], 0, sizeof(Units[i]));
    }
    make_unit(BSC_COLONY_POD, CHS_INFANTRY, WPN_COLONY_MODULE, ARM_NO_ARMOR, PLAN_COLONY, 6);
    make_unit(BSC_FORMERS, CHS_INFANTRY, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, PLAN_TERRAFORM, 3);
    make_unit(BSC_SCOUT_PATROL, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, PLAN_DEFENSE, 2);
    make_unit(BSC_MIND_WORMS, CHS_INFANTRY, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, PLAN_OFFENSE, 3);
    for (int f = 1; f < 4; f++) {
        // Slot 0: attacker (laser/no armor/speeder), slot 1: defender (hand weapons/armor).
        make_unit(f * MaxProtoFactionNum, CHS_SPEEDER, 2, ARM_NO_ARMOR, PLAN_OFFENSE, 4);
        make_unit(f * MaxProtoFactionNum + 1, CHS_INFANTRY, WPN_HAND_WEAPONS, 2, PLAN_DEFENSE, 4);
    }
    // Bases: two per faction, one building units, one building a facility.
    struct { int f, x, y, pop, item, minerals; } bases[] = {
        {1, 4, 4, 2, MaxProtoFactionNum * 1, 36},
        {1, 4, 12, 1, -FAC_RECYCLING_TANKS, 64},
        {2, 20, 4, 2, MaxProtoFactionNum * 2 + 1, 34},
        {2, 20, 12, 1, -FAC_PERIMETER_DEFENSE, 60},
        {3, 28, 16, 1, BSC_SCOUT_PATROL, 15},
        {3, 26, 2, 1, -FAC_STOCKPILE_ENERGY, 0},
    };
    for (auto& b : bases) {
        if (make_base(b.f, b.x, b.y, b.pop, b.item, b.minerals) < 0) {
            return 11;
        }
    }
    // Front line: human attackers at x = 12, faction 2 defenders diagonally at x = 13.
    for (int i = 0; i < 4; i++) {
        int y = 4 + 2 * i;
        int a = veh_init(MaxProtoFactionNum * 1, 1, 12, y);
        int d = veh_init(MaxProtoFactionNum * 2 + 1, 2, 13, y + 1);
        if (a < 0 || d < 0) {
            return 12;
        }
        Vehs[a].morale = (uint8_t)(1 + i % 3);
        Vehs[d].morale = (uint8_t)(2 + i % 2);
    }
    // A faction 3 unit next to a human base, and a human garrison.
    veh_init(MaxProtoFactionNum * 3, 3, 6, 4);
    veh_init(MaxProtoFactionNum * 1 + 1, 1, 4, 4);
    *ControlTurnMove = 0;
    std::vector<uint8_t> save;
    int status = save_daemon_bytes(save);
    if (status || !write_file(out, save)) {
        return 13;
    }
    printf("made bases=%d vehicles=%d bytes=%u\n", *BaseCount, *VehCount, (unsigned)save.size());
    return 0;
}

static uint32_t digest_turn(const TurnResult& r, uint32_t h) {
    for (const TurnEvent& e : r.events) {
        int32_t v[11] = {e.kind, e.turn, e.faction_id, e.type, e.a, e.b, e.c, e.d, e.x, e.y,
            (int32_t)e.text.size()};
        for (int32_t x : v) {
            h = mix(h, x);
        }
        h = fnv1a(e.text.data(), e.text.size(), h);
    }
    return h;
}

/* First human combat unit with an adjacent enemy vehicle: its id and target. */
static int pick_attack(int* tx, int* ty) {
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        if (v->faction_id != Human || v->unit_id != MaxProtoFactionNum * Human) {
            continue;
        }
        for (int j = 0; j < *VehCount; j++) {
            VEH* t = &Vehs[j];
            if (t->faction_id != Human && t->faction_id != 0
            && map_range(v->x, v->y, t->x, t->y) == 1) {
                *tx = t->x;
                *ty = t->y;
                return i;
            }
        }
    }
    return -1;
}

static int run_turns(const char* in, const char* out, int turns) {
    auto s = new_state();
    std::vector<uint8_t> data = read_file(in);
    int status = load_daemon_bytes(data.data(), data.size());
    if (status != SAVE_LOAD_VALID) {
        printf("load_error=%d\n", status);
        return 20;
    }
    if (!is_human(Human) || *BaseCount != 6) {
        printf("bad_world human=%d bases=%d\n", is_human(Human), *BaseCount);
        return 21;
    }
    game_srand(1);
    random_reseed(1);
    uint32_t h = 2166136261u;
    int battles = 0;
    int vehicles_start = *VehCount;
    std::unique_ptr<BattleResult> br(new BattleResult());
    for (int call = 0; call < turns; call++) {
        if (call % 2 == 0) {
            int tx = -1, ty = -1;
            int veh_id = pick_attack(&tx, &ty);
            if (veh_id >= 0) {
                int outcome = engine_attack(veh_id, tx, ty, br.get());
                battles += outcome >= BATTLE_ATTACKER_WON;
                printf("attack call=%d veh=%d target=%d,%d outcome=%s rounds=%d hp=%d/%d killed=%d\n",
                    call, veh_id, tx, ty, battle_outcome_name(outcome), br->rounds,
                    br->hp_atk_after, br->hp_def_after, br->num_killed);
                h = mix(mix(mix(h, outcome), br->rounds), br->attacker_hits);
            }
        }
        TurnResult r = engine_end_turn();
        h = digest_turn(r, h);
        printf("call=%d status=%d faction=%d turn=%d year=%d events=%u bases=%d vehicles=%d\n",
            call, r.status, r.faction_id, r.turn, r.year, (unsigned)r.events.size(),
            *BaseCount, *VehCount);
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            h = mix(mix(mix(mix(mix(h, b->pop_size), b->minerals_accumulated),
                b->nutrients_accumulated), b->queue_items[0]), b->facilities_built[0]);
            printf("  base=%d faction=%d pop=%d nutrients=%d minerals=%d item=%d facilities=%02x\n",
                i, b->faction_id, b->pop_size, b->nutrients_accumulated,
                b->minerals_accumulated, b->queue_items[0], b->facilities_built[0]);
        }
    }
    std::vector<uint8_t> save;
    status = save_daemon_bytes(save);
    if (status || !write_file(out, save)) {
        printf("save_error=%d\n", status);
        return 22;
    }
    printf("battles=%d vehicles=%d->%d digest=%08x save_bytes=%u save_digest=%08x\n",
        battles, vehicles_start, *VehCount, (unsigned)h, (unsigned)save.size(),
        (unsigned)fnv1a(save.data(), save.size(), 2166136261u));
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 3 && !strcmp(argv[1], "make")) {
        return run_make(argv[2]);
    }
    if (argc == 5 && !strcmp(argv[1], "run")) {
        return run_turns(argv[2], argv[3], atoi(argv[4]));
    }
    fprintf(stderr, "usage: %s make OUT | run IN OUT TURNS\n", argv[0]);
    return 2;
}
