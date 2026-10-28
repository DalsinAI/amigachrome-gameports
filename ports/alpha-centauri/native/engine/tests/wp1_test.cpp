/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP1 turn scheduler checks (driven by test_engine_wp1.py).
 *
 *   wp1_test unit
 *       synthetic worlds: turn order, human/AI/hotseat stops, year and turn counter,
 *       movement refill, repair, loans, hurry costs, territory, rankings, social
 *       upheaval, fungal towers, load/resume and same-seed determinism.
 *   wp1_test run IN OUT TURNS [HUMAN]
 *       load_daemon_bytes(IN), set synthetic rules (HUMAN: make that faction a living human), call engine_end_turn() TURNS times
 *       (printing one line per call and a digest of the events), save_daemon_bytes -> OUT.
 */
#include "../engine.h"
#include "../turn.h"

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

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    return s;
}

static uint32_t fnv1a(const uint8_t* p, size_t n, uint32_t h = 2166136261u) {
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t digest_events(const TurnResult& r, uint32_t h = 2166136261u) {
    for (const TurnEvent& e : r.events) {
        int32_t v[11] = {e.kind, e.turn, e.faction_id, e.type, e.a, e.b, e.c, e.d, e.x, e.y,
            (int32_t)e.text.size()};
        for (int32_t x : v) {
            uint8_t b[4] = {(uint8_t)x, (uint8_t)(x >> 8), (uint8_t)(x >> 16), (uint8_t)(x >> 24)};
            h = fnv1a(b, 4, h);
        }
        h = fnv1a((const uint8_t*)e.text.data(), e.text.size(), h);
    }
    return h;
}

/* Synthetic rules values (alpha.txt is loaded by another package). */
static void synthetic_rules() {
    Rules->normal_start_year = 2100;
    for (int i = 0; i < MaxProtoNum; i++) {
        Units[i].plan = PLAN_DEFENSE;
        Units[i].preq_tech = TECH_Disable;
        Units[i].cost = 3;
        Units[i].reactor_id = 1;
        Units[i].chassis_id = CHS_INFANTRY;
        Units[i].weapon_id = (uint8_t)(Units[i].weapon_id % MaxWeaponNum);
        Units[i].armor_id = (uint8_t)(Units[i].armor_id % MaxArmorNum);
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        Weapon[i].offense_value = (int8_t)(i % 8);
    }
    Units[BSC_COLONY_POD].plan = PLAN_COLONY;
    for (int i = 0; i < MaxChassisNum; i++) {
        Chassis[i].triad = TRIAD_LAND;
        Chassis[i].speed = 1;
        Chassis[i].range = 0;
    }
    for (int i = 0; i <= Fac_ID_Last; i++) {
        Facility[i].preq_tech = TECH_Disable;
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        Tech[i].preq_tech1 = TECH_Disable;
        Tech[i].preq_tech2 = TECH_Disable;
    }
    for (int i = 0; i < MaxProposalNum; i++) {
        Proposal[i].preq_tech = TECH_Disable;
    }
    Rules->tech_preq_economic_victory = TECH_Disable;
    // Integration: WP2's base upkeep divides by these (alpha.txt gives 1/1).
    Rules->freq_global_warming_numerator = 1;
    Rules->freq_global_warming_denominator = 1;
    // Integration: WP7's world_climate (turn hook) divides by the
    // #WORLDBUILDER values; these are the alpha.txt defaults.
    CWorldbuilder& w = *WorldBuilder;
    w.land_base = 384; w.land_mod = 256; w.continent_base = 12; w.continent_mod = 24;
    w.hills_base = 1; w.hills_mod = 2; w.plateau_base = 4; w.plateau_mod = 8;
    w.rivers_base = 8; w.rivers_rain_mod = 12; w.solar_energy = 14; w.thermal_band = 14;
    w.thermal_deviance = 8; w.global_warming = 8; w.sea_level_rises = 5;
    w.cloudmass_peaks = 5; w.cloudmass_hills = 3; w.rainfall_coeff = 1;
    w.deep_water = 15; w.shelf = 10; w.plains = 15; w.beach = 10; w.hills = 10; w.peaks = 25;
    w.fungus = 1;
}

/* Integration: WP8's mod_eliminate_player (Thinker rule) removes a faction
 * that has neither a base nor a colony pod, so every alive faction of the
 * turn-order tests gets a colony pod. */
static void add_colony_pods(uint32_t alive) {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if ((alive >> i) & 1) {
            CHECK(veh_init(BSC_COLONY_POD, i, 0, 2 * i) >= 0);
        }
    }
}

/* width x height land map, all region 1, factions alive as given, humans as given. */
static void setup_world(uint32_t alive, uint32_t human, int width = 16, int height = 16) {
    *MapAreaX = width;
    *MapAreaY = height;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = (ALT_SHORE_LINE + 1) << 5; // land
        (*MapTiles)[i].region = 1;
        (*MapTiles)[i].owner = -1;
    }
    synthetic_rules();
    for (int i = 0; i < MaxPlayerNum; i++) {
        Factions[i].target_x = -1;
        Factions[i].base_id_attack_target = -1;
        Factions[i].tech_research_id = 0;
        Factions[i].diff_level = 3;
        set_alive(i, (alive >> i) & 1);
        set_human(i, (human >> i) & 1);
    }
    *CurrentTurn = 0;
    *CurrentMissionYear = game_year(0);
    MapWin->cOwner = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if ((human >> i) & 1) {
            MapWin->cOwner = i;
            *CurrentPlayerFaction = i;
            break;
        }
    }
    *ControlTurnMove = 0;
    *PbemActive = bit_count(human) > 1;
    *GamePreferences &= ~PREF_BSC_AUTOSAVE_EACH_TURN;
}

static std::vector<int> kinds(const TurnResult& r, std::vector<int>* factions = nullptr) {
    std::vector<int> out;
    for (const TurnEvent& e : r.events) {
        if (e.kind >= TE_TURN_BEGIN) {
            out.push_back(e.kind);
            if (factions) {
                factions->push_back(e.kind == TE_TURN_BEGIN ? e.turn : e.faction_id);
            }
        }
    }
    return out;
}

static void test_year() {
    auto s = new_state();
    engine_bind(s.get());
    Rules->normal_start_year = 2100;
    CHECK_EQ(game_year(0), 2100);
    CHECK_EQ(game_year(1), 2101);
    CHECK_EQ(game_year(250), 2350);
}

static void test_single_human() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x0E, 0x02); // factions 1..3 alive, 1 human
    add_colony_pods(0x0E);
    int v1 = veh_init(BSC_SCOUT_PATROL, 1, 2, 2);
    int v2 = veh_init(BSC_SCOUT_PATROL, 2, 8, 8);
    int v3 = veh_init(BSC_SCOUT_PATROL, 3, 12, 12);
    CHECK(v1 >= 0 && v2 >= 0 && v3 >= 0);
    for (int v : {v1, v2, v3}) {
        Vehs[v].moves_spent = 3;
        Vehs[v].state |= VSTATE_HAS_MOVED;
    }

    // New game: turn upkeep (turn 0 -> 1), then factions in order until the human.
    TurnResult r = engine_end_turn();
    CHECK_EQ(r.status, TURN_HUMAN);
    CHECK_EQ(r.faction_id, 1);
    CHECK_EQ(r.turn, 1);
    CHECK_EQ(r.year, 2101);
    CHECK_EQ(*CurrentTurn, 1);
    CHECK_EQ(*CurrentMissionYear, 2101);
    CHECK_EQ(*ControlTurnMove, 1);
    CHECK_EQ(*CurrentFaction, 1);
    std::vector<int> who;
    std::vector<int> k = kinds(r, &who);
    // Turn 1 is a territory turn ((turn & 3) == 1).
    std::vector<int> want = {TE_TURN_BEGIN, TE_TERRITORY, TE_FACTION_UPKEEP, TE_HUMAN_TURN};
    CHECK(k == want);
    CHECK_EQ(who.size(), 4);
    if (who.size() == 4) {
        CHECK_EQ(who[0], 1);
        CHECK_EQ(who[2], 1);
        CHECK_EQ(who[3], 1);
    }
    // Movement refill happens in the faction's own upkeep at the start of its turn.
    CHECK_EQ(Vehs[v1].moves_spent, 0);
    CHECK_EQ(Vehs[v2].moves_spent, 3);
    CHECK_EQ(Vehs[v3].moves_spent, 3);

    // The human moves, then ends the turn: factions 2 and 3 play, a new turn begins,
    // and the human's turn starts again with full movement.
    Vehs[v1].moves_spent = 2;
    r = engine_end_turn();
    CHECK_EQ(r.status, TURN_HUMAN);
    CHECK_EQ(r.faction_id, 1);
    CHECK_EQ(r.turn, 2);
    CHECK_EQ(r.year, 2102);
    who.clear();
    k = kinds(r, &who);
    want = {TE_HUMAN_TURN_END, TE_FACTION_UPKEEP, TE_AI_TURN, TE_FACTION_UPKEEP, TE_AI_TURN,
        TE_TURN_BEGIN, TE_FACTION_UPKEEP, TE_HUMAN_TURN};
    CHECK(k == want);
    const int want_who[] = {1, 2, 2, 3, 3, 2, 1, 1};
    CHECK_EQ(who.size(), 8);
    for (size_t i = 0; i < who.size() && i < 8; i++) {
        CHECK_EQ(who[i], want_who[i]);
    }
    CHECK_EQ(Vehs[v1].moves_spent, 0);
    CHECK_EQ(Vehs[v2].moves_spent, 0);
    CHECK_EQ(Vehs[v3].moves_spent, 0);

    // Ten more turns: one year per End Turn.
    for (int t = 3; t <= 12; t++) {
        Vehs[v1].moves_spent = 1;
        r = engine_end_turn();
        CHECK_EQ(r.status, TURN_HUMAN);
        CHECK_EQ(r.turn, t);
        CHECK_EQ(r.year, 2100 + t);
        CHECK_EQ(Vehs[v1].moves_spent, 0);
    }
    // Rankings ran each turn and recorded might for the turn.
    CHECK(Factions[1].ranking > 0);
    CHECK(FactionRankings[Factions[1].ranking] == 1);
}

static void test_all_ai() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x0F, 0x00); // natives + 1..3, no human
    add_colony_pods(0x0F);
    int v = veh_init(BSC_SCOUT_PATROL, 2, 4, 4);
    for (int t = 1; t <= 6; t++) {
        Vehs[v].moves_spent = 5;
        TurnResult r = engine_end_turn();
        CHECK_EQ(r.status, TURN_ROUND_END);
        CHECK_EQ(r.faction_id, -1);
        CHECK_EQ(r.turn, t);
        CHECK_EQ(r.year, 2100 + t);
        CHECK_EQ(*ControlTurnMove, 0);
        CHECK_EQ(Vehs[v].moves_spent, 0);
        std::vector<int> who;
        std::vector<int> k = kinds(r, &who);
        std::vector<int> want = {TE_TURN_BEGIN,
            TE_FACTION_UPKEEP, TE_AI_TURN, TE_FACTION_UPKEEP, TE_AI_TURN,
            TE_FACTION_UPKEEP, TE_AI_TURN, TE_FACTION_UPKEEP, TE_AI_TURN};
        if ((t & 3) == 1) {
            want.insert(want.begin() + 1, TE_TERRITORY);
        }
        want.push_back(TE_ROUND_END);
        CHECK(k == want);
    }
    CHECK_EQ(turn_order_next(-1), 0);
    CHECK_EQ(turn_order_next(0), 1);
    CHECK_EQ(turn_order_next(3), 0);
}

static void test_hotseat() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x0E, 0x06); // 1 and 2 human, 3 AI
    add_colony_pods(0x0E);
    HeadlessSink* sink = headless_sink();
    sink->clear();
    TurnResult r = engine_end_turn();
    CHECK_EQ(r.status, TURN_HUMAN);
    CHECK_EQ(r.faction_id, 1);
    CHECK_EQ(MapWin->cOwner, 1);
    r = engine_end_turn();
    CHECK_EQ(r.status, TURN_HUMAN);
    CHECK_EQ(r.faction_id, 2);
    CHECK_EQ(r.turn, 1);
    CHECK_EQ(MapWin->cOwner, 2);
    bool switching = false;
    for (const TurnEvent& e : r.events) {
        if (e.kind == TE_POPUP && e.text == "SWITCHING") {
            switching = true;
        }
    }
    CHECK(switching);
    r = engine_end_turn();
    CHECK_EQ(r.status, TURN_HUMAN);
    CHECK_EQ(r.faction_id, 1);
    CHECK_EQ(r.turn, 2);
    CHECK_EQ(MapWin->cOwner, 1);
    // Human eliminated mid-game: the next call stops at the other human.
    set_alive(1, false);
    r = engine_end_turn();
    CHECK_EQ(r.faction_id, 2);
    CHECK_EQ(r.turn, 2);
    set_alive(2, false);
    r = engine_end_turn();
    CHECK_EQ(r.status, TURN_ROUND_END);
    CHECK_EQ(r.turn, 3);
}

static void test_repair_and_economy() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x06, 0x02);
    int b = base_init(1, 4, 4);
    CHECK(b >= 0);
    int v = veh_init(BSC_SCOUT_PATROL, 1, 4, 4); // in the base
    int w = veh_init(BSC_SCOUT_PATROL, 1, 10, 10); // in the field, not in territory
    int x = veh_init(BSC_SCOUT_PATROL, 1, 10, 12); // in the field, moved last turn
    Vehs[v].damage_taken = 8;
    Vehs[w].damage_taken = 8;
    Vehs[x].damage_taken = 8;
    Vehs[x].state |= VSTATE_HAS_MOVED;
    *CurrentFaction = 1;
    repair_phase(1);
    // Both tiles are in faction 1's territory (base_init resets the borders).
    CHECK_EQ(mapsq(4, 4)->owner, 1);
    CHECK_EQ(mapsq(10, 10)->owner, 1);
    // Base: repair_minimal 1 + repair_friendly 1 + repair_base 1 = 3 per reactor level.
    CHECK_EQ(Vehs[v].damage_taken, 5);
    // Field: minimal 1 + friendly 1, floor 2 * reactor outside bases.
    CHECK_EQ(Vehs[w].damage_taken, 6);
    // A unit that moved last turn is not repaired.
    CHECK_EQ(Vehs[x].damage_taken, 8);

    // Integration: production_phase now runs WP2's base upkeep, which adds the
    // base's energy income after the loan payments and interest; measure it.
    Factions[1].energy_credits = 100;
    production_phase(1);
    const int income = Factions[1].energy_credits - 100;
    CHECK(income >= 0);

    // Loans: faction 1 pays faction 2 what it can, the rest stays on the balance.
    Factions[1].energy_credits = 3;
    Factions[1].loan_balance[2] = 20;
    Factions[1].loan_payment[2] = 5;
    Factions[2].energy_credits = 0;
    production_phase(1);
    CHECK_EQ(Factions[1].energy_credits, 0 + income);
    CHECK_EQ(Factions[2].energy_credits, 3);
    CHECK_EQ(Factions[1].loan_balance[2], 19);

    // Interest rule: +1 credit per base with rule_interest 0.
    Factions[1].loan_balance[2] = 0;
    MFactions[1].rule_flags |= RFLAG_INTEREST;
    MFactions[1].rule_interest = 0;
    Factions[1].energy_credits = 10;
    production_phase(1);
    CHECK_EQ(Factions[1].energy_credits, 11 + income);
    MFactions[1].rule_interest = 10;
    Factions[1].energy_credits = 10;
    production_phase(1);
    CHECK_EQ(Factions[1].energy_credits, 11 + income);

    // Hurry costs are paid at the end of faction upkeep and credits never go negative.
    Factions[1].hurry_cost_total = 50;
    faction_upkeep(1);
    CHECK_EQ(Factions[1].hurry_cost_total, 0);
    CHECK(Factions[1].energy_credits >= 0);
}

static void test_territory() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x06, 0x02, 40, 40);
    // A second continent (region 2) on rows >= 30.
    for (int y = 30; y < 40; y++) {
        for (int xx = y & 1; xx < 40; xx += 2) {
            mapsq(xx, y)->region = 2;
        }
    }
    int b1 = base_init(1, 10, 10);
    int b2 = base_init(2, 20, 10);
    CHECK(b1 >= 0 && b2 >= 0);
    reset_territory();
    CHECK_EQ(mapsq(10, 10)->owner, 1);
    CHECK_EQ(mapsq(12, 10)->owner, 1);
    CHECK_EQ(mapsq(18, 10)->owner, 2);
    CHECK_EQ(mapsq(16, 10)->owner, 2); // vector_dist 4 vs 6
    CHECK_EQ(mapsq(10, 26)->owner, -1); // more than 7 away
    CHECK_EQ(mapsq(10, 32)->owner, -1); // other continent
    CHECK(Factions[1].region_territory_tiles[1] > 0);
    CHECK_EQ(whose_territory(1, 12, 10, nullptr, 1), 1);
}

static void test_social_and_rankings() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x0E, 0x02);
    CSocialCategory c;
    Factions[1].SE_Politics = 1;
    Factions[1].SE_Economics = 0;
    Factions[1].SE_Values = 0;
    Factions[1].SE_Future = 0;
    c.politics = 1;
    c.economics = 0;
    c.values = 0;
    c.future = 0;
    CHECK_EQ(social_upheaval(1, &c), 0);
    c.economics = 2;
    c.values = 1;
    Factions[1].diff_level = 2;
    CHECK_EQ(social_upheaval(1, &c), 3 * 3 * 3 * 2); // two changes -> (2+1)^3 * diff
    CHECK_EQ(social_upheaval(2, &c), 4 * 4 * 4 * DIFF_LIBRARIAN); // AI: three changes, librarian
    Factions[1].SE_Politics_pending = 2;
    Factions[1].SE_upheaval_cost_paid = 7;
    social_upkeep(1);
    CHECK_EQ(Factions[1].SE_Politics, 2);
    CHECK_EQ(Factions[1].SE_upheaval_cost_paid, 0);

    *ObjectiveReqVictory = 9999;
    *ObjectivesSuddenDeathVictory = 9999;
    Factions[1].pop_total = 5;
    Factions[2].pop_total = 9;
    Factions[3].pop_total = 1;
    *CurrentTurn = 7;
    rankings(1);
    CHECK_EQ(FactionTurnMight[7][1], 20);
    CHECK_EQ(FactionTurnMight[7][2], 36);
    CHECK_EQ(FactionRankings[7], 2);
    CHECK_EQ(FactionRankings[6], 1);
    CHECK_EQ(FactionRankings[5], 3);
    CHECK_EQ(Factions[2].ranking, 7);
}

static void test_fungal_towers() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(0x03, 0x02);
    int t = veh_init(BSC_FUNGAL_TOWER, 0, 6, 6);
    CHECK(t >= 0);
    bit_set(6, 6, BIT_ROAD|BIT_FARM, 1);
    game_srand(7);
    do_fungal_towers();
    CHECK(mapsq(6, 6)->items & BIT_FUNGUS);
    CHECK(!(mapsq(6, 6)->items & (BIT_ROAD|BIT_FARM)));
    CHECK(Vehs[t].state & VSTATE_HAS_MOVED);
}

static void run_game(EngineState* s, int turns, std::vector<uint8_t>& save, uint32_t& events) {
    engine_bind(s);
    events = 2166136261u;
    for (int i = 0; i < turns; i++) {
        TurnResult r = engine_end_turn();
        events = digest_events(r, events);
    }
    save.clear();
    CHECK_EQ(save_daemon_bytes(save), 0);
}

static void build_game(EngineState* s) {
    engine_bind(s);
    setup_world(0x0F, 0x02, 24, 24);
    game_srand(12345);
    random_reseed(777);
    base_init(1, 4, 4);
    base_init(2, 14, 14);
    base_init(3, 20, 6);
    veh_init(BSC_FUNGAL_TOWER, 0, 10, 10);
    veh_init(BSC_SCOUT_PATROL, 1, 5, 5);
    veh_init(BSC_SCOUT_PATROL, 2, 15, 15);
    veh_init(BSC_COLONY_POD, 3, 21, 7);
    *ClimateFutureChange = 0;
}

static void test_determinism_and_resume() {
    auto a = new_state();
    auto b = new_state();
    build_game(a.get());
    build_game(b.get());
    std::vector<uint8_t> sa, sb;
    uint32_t ea, eb;
    run_game(a.get(), 9, sa, ea);
    run_game(b.get(), 9, sb, eb);
    CHECK(sa == sb);
    CHECK_EQ(ea, eb);
    engine_bind(a.get());
    CHECK_EQ(*CurrentTurn, 9);
    CHECK_EQ(*ControlTurnMove, 1);

    // Save during the human's turn, load it into a fresh state and end the turn there:
    // the loaded game resumes after the human exactly like the original.
    auto c = new_state();
    engine_bind(c.get());
    synthetic_rules();
    CHECK_EQ(load_daemon_bytes(sa.data(), sa.size()), SAVE_LOAD_VALID);
    CHECK_EQ(*ControlTurnMove, 1);
    // Integration: a saved game does not carry the random generators (as in
    // the original), and WP6's computer players draw from them during the
    // turn, so the loaded game continues from the original's generator state.
    c->rng = a->rng;
    TurnResult rc = engine_end_turn();
    engine_bind(a.get());
    TurnResult ra = engine_end_turn();
    CHECK_EQ(rc.status, ra.status);
    CHECK_EQ(rc.turn, ra.turn);
    CHECK_EQ(rc.turn, 10);
    std::vector<uint8_t> s1, s2;
    engine_bind(a.get());
    CHECK_EQ(save_daemon_bytes(s1), 0);
    engine_bind(c.get());
    CHECK_EQ(save_daemon_bytes(s2), 0);
    CHECK(s1 == s2);
}

static int run_unit() {
    auto base = new_state();
    engine_bind(base.get());
    config_vanilla(conf);
    test_year();
    test_single_human();
    test_all_ai();
    test_hotseat();
    test_repair_and_economy();
    test_territory();
    test_social_and_rankings();
    test_fungal_towers();
    test_determinism_and_resume();
    engine_bind(base.get());
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d wp1 unit checks passed\n", checks);
    return 0;
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

/* Load a synthetic save, make its patterned records valid for the turn code, then
 * run TURNS End Turn calls. */
static int run_turns(const char* in, const char* out, int turns, int human) {
    auto s = new_state();
    engine_bind(s.get());
    config_vanilla(conf);
    std::vector<uint8_t> data = read_file(in);
    int status = load_daemon_bytes(data.data(), data.size());
    if (status != SAVE_LOAD_VALID) {
        printf("error=%s\n", save_error_name(status));
        return 20;
    }
    synthetic_rules();
    // Integration: unit speeds (WP3 veh_speed, called by the WP6 unit moves)
    // divide by the road movement rate; alpha.txt gives 3.
    Rules->move_rate_roads = 3;
    // Integration: unit messages print the morale names (WP4), which the rules
    // loader never leaves null.
    static char morale_name[] = "";
    for (int i = 0; i < MaxMoraleNum; i++) {
        Morale[i].name = morale_name;
        Morale[i].name_lifecycle = morale_name;
    }
    // Integration: WP7's world_temperature divides the map height by these
    // #WORLDBUILDER values (as the original does); the synthetic maps are
    // smaller than any real one, so scale them down to the map height.
    for (int32_t* v : {&WorldBuilder->solar_energy, &WorldBuilder->thermal_band,
            &WorldBuilder->thermal_deviance, &WorldBuilder->global_warming}) {
        *v = clamp(*v, 1, max(1, *MapAreaY));
    }
    // Patterned bytes: keep every record inside the ranges the turn code indexes.
    for (int i = 0; i < *VehCount; i++) {
        VEH* v = &Vehs[i];
        v->faction_id = (uint8_t)(v->faction_id & 7);
        if (v->unit_id < 0 || v->unit_id >= MaxProtoNum) {
            v->unit_id = BSC_SCOUT_PATROL;
        }
        // Integration: WP2's unit support reads the tile of every supported unit.
        if (!mapsq(v->x, v->y) && *MapAreaX >= 2 && *MapAreaY >= 1) {
            v->y = (int16_t)(i % *MapAreaY);
            v->x = (int16_t)((2 * i) % (*MapAreaX & ~1) + (v->y & 1));
        }
        if (v->home_base_id >= *BaseCount) {
            v->home_base_id = -1;
        }
    }
    // Integration: patterned stack links can form cycles, which WP3's real
    // spot_all would walk forever; rebuild the stacks from the positions.
    {
        std::vector<std::pair<int, int>> pos;
        for (int i = 0; i < *VehCount; i++) {
            pos.push_back({Vehs[i].x, Vehs[i].y});
            Vehs[i].x = -1;
            Vehs[i].y = -1;
            Vehs[i].next_veh_id_stack = -1;
            Vehs[i].prev_veh_id_stack = -1;
        }
        for (int i = 0; i < *VehCount; i++) {
            veh_drop(i, pos[i].first, pos[i].second);
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        b->faction_id = b->faction_id & 7;
        // Integration: WP6 plans_upkeep indexes diplomacy by the former owner.
        b->faction_id_former = b->faction_id_former & 7;
        b->pop_size = b->pop_size & 15;
        // Integration: WP2's base messages print the base name; the patterned
        // names have no terminator, and reading on past them shows the
        // following fields in host byte order.
        b->name[sizeof(b->name) - 1] = '\0';
        // Integration: production_phase now runs WP2's base upkeep, which needs the
        // base on a map tile (its worked radius starts at the base tile).
        if (!mapsq(b->x, b->y) && *MapAreaX >= 2 && *MapAreaY >= 1) {
            b->y = i % *MapAreaY;
            b->x = (2 * i) % (*MapAreaX & ~1) + (b->y & 1);
        }
        b->specialist_adjust = b->specialist_adjust & 7;
        b->drone_total = b->drone_total & 7;
        b->talent_total = b->talent_total & 7;
        b->energy_surplus = b->energy_surplus & 255;
        b->unk_total = b->unk_total & 255;
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        Faction* f = &Factions[i];
        f->base_id_attack_target = -1;
        f->AI_fight = f->AI_fight & 1;
        f->AI_growth = f->AI_growth & 1;
        f->AI_tech = f->AI_tech & 1;
        f->AI_wealth = f->AI_wealth & 1;
        f->AI_power = f->AI_power & 1;
        f->SE_police_pending = clamp(f->SE_police_pending, -5, 5);
        f->SE_effic_2 = clamp(f->SE_effic_2, -5, 5);
        f->SE_alloc_psych = f->SE_alloc_psych & 3;
        f->SE_alloc_labs = f->SE_alloc_labs & 7;
        f->energy_credits = f->energy_credits & 0xFFFF;
        f->hurry_cost_total = f->hurry_cost_total & 0xFF;
        f->pop_total = f->pop_total & 0xFFF;
        f->tech_count_transcendent = f->tech_count_transcendent & 0xFF;
        f->unk_101 = f->unk_101 & 0xFF;
        f->integrity_blemishes = f->integrity_blemishes & 0xFF;
        for (int j = 0; j < MaxPlayerNum; j++) {
            f->loan_balance[j] = f->loan_balance[j] & 0xFFFF;
            f->loan_payment[j] = f->loan_payment[j] & 0xFF;
            f->diplo_gifts[j] = f->diplo_gifts[j] & 0xFF;
        }
        MFactions[i].faction_bonus_count = 0;
        MFactions[i].rule_techcost = MFactions[i].rule_techcost & 0xFF;
        MFactions[i].rule_interest = MFactions[i].rule_interest % 100;
    }
    *ObjectiveAchievePts = *ObjectiveAchievePts & 0xFF;
    // Integration: values WP2's base upkeep indexes or multiplies.
    *DiffLevel = clamp(*DiffLevel, 0, 5);
    *ClimateFutureChange = *ClimateFutureChange & 0xFF;
    for (int i = 0; i < MaxPlayerNum; i++) {
        FactionRankings[i] = FactionRankings[i] & 7;
    }
    *MapNativeLifeForms = *MapNativeLifeForms & 3;
    // Integration: the WP6 route scores add up the continent counters.
    for (int r = 0; r < MaxRegionNum; r++) {
        Continent& c = Continents[r];
        c.tile_count &= 0xFFFF;
        c.open_terrain &= 0xFFFF;
        c.world_site &= 0xF;
        c.pods &= 0xFF;
    }
    for (int i = 0; i < MaxPlayerNum; i++) {
        Faction* f = &Factions[i];
        f->major_atrocities = f->major_atrocities & 0xF;
        // Integration: WP6 enemy_strategy scales turn thresholds by it.
        f->diff_level = clamp(f->diff_level, 0, 6);
        // Integration: WP3 supply pods (opened by WP6 unit moves) double these.
        for (int32_t* p = &f->goody_opened; p <= &f->goody_tech; p++) {
            *p = *p & 0xFF;
        }
        // Integration: WP6 mod_social_ai indexes the social models by the
        // faction's priority choice (-1 = none, 0..3 as in the faction file).
        MFactions[i].soc_priority_category = clamp(MFactions[i].soc_priority_category, -1, 3);
        MFactions[i].soc_priority_model = clamp(MFactions[i].soc_priority_model, -1, 3);
        f->tech_ranking = f->tech_ranking & 0xFF;
        f->clean_minerals_modifier = f->clean_minerals_modifier & 0xF;
        f->tech_commerce_bonus = f->tech_commerce_bonus & 0xF;
        f->satellites_nutrient = f->satellites_nutrient & 0xF;
        f->satellites_mineral = f->satellites_mineral & 0xF;
        f->satellites_energy = f->satellites_energy & 0xF;
        f->satellites_ODP = f->satellites_ODP & 0xF;
        f->tech_fungus_nutrient = f->tech_fungus_nutrient & 3;
        f->tech_fungus_mineral = f->tech_fungus_mineral & 3;
        f->tech_fungus_energy = f->tech_fungus_energy & 3;
        // Social choices and effects (contiguous int32 fields).
        for (int32_t* p = &f->SE_Politics_pending; p <= &f->SE_Future; p++) {
            *p = *p & 3;
        }
        for (int32_t* p = &f->SE_economy_pending; p <= &f->SE_research_base; p++) {
            *p = clamp(*p, -3, 3);
        }
        TectonicDetonationCount[i] = TectonicDetonationCount[i] & 0xF;
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        b->eco_damage = b->eco_damage & 0xFF;
        b->queue_size = clamp(b->queue_size, 0, 9);
        for (int& item : b->queue_items) {
            if (item >= MaxProtoNum || item < -SP_ID_Last) {
                item = -FAC_STOCKPILE_ENERGY;
            }
        }
        if (b->production_id_last >= MaxProtoNum || b->production_id_last < -SP_ID_Last) {
            b->production_id_last = b->queue_items[0];
        }
        b->minerals_accumulated = b->minerals_accumulated & 0xFFF;
        b->nutrients_accumulated = b->nutrients_accumulated & 0xFFF;
        // Last turn's yields; base_compute recomputes them, but other bases read them first.
        for (int32_t* p = &b->nutrient_intake; p <= &b->unk_total; p++) {
            *p = *p & 0xFF;
        }
    }
    *ClimateValueA = *ClimateValueA & 0xFFFF;
    *ClimateValueB = *ClimateValueB & 0xFFFF;
    *ClimateValueC = *ClimateValueC & 0xFFFF;
    for (int i = 0; i < MaxSecretProjectNum; i++) {
        if (SecretProjects[i] >= *BaseCount) {
            SecretProjects[i] = -1;
        }
    }
    *GamePreferences &= ~PREF_BSC_AUTOSAVE_EACH_TURN;
    if (human > 0 && human < MaxPlayerNum) {
        set_alive(human, true);
        set_human(human, true);
        MapWin->cOwner = human;
        *CurrentPlayerFaction = human;
        *ControlTurnMove = 0;
    }
    game_srand(1);
    random_reseed(1);
    uint32_t h = 2166136261u;
    for (int i = 0; i < turns; i++) {
        TurnResult r = engine_end_turn();
        h = digest_events(r, h);
        printf("call=%d status=%d faction=%d turn=%d year=%d events=%u\n",
            i, r.status, r.faction_id, r.turn, r.year, (unsigned)r.events.size());
    }
    std::vector<uint8_t> save;
    status = save_daemon_bytes(save);
    if (status) {
        printf("save_error=%d\n", status);
        return 21;
    }
    FILE* fp = fopen(out, "wb");
    if (!fp || fwrite(save.data(), 1, save.size(), fp) != save.size()) {
        return 22;
    }
    fclose(fp);
    printf("events_digest=%08x save_bytes=%u\n", (unsigned)h, (unsigned)save.size());
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "unit")) {
        return run_unit();
    }
    if ((argc == 5 || argc == 6) && !strcmp(argv[1], "run")) {
        return run_turns(argv[2], argv[3], atoi(argv[4]), argc == 6 ? atoi(argv[5]) : 0);
    }
    fprintf(stderr, "usage: %s unit | run IN OUT TURNS [HUMAN]\n", argv[0]);
    return 2;
}
