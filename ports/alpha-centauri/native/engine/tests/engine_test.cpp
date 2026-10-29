/* SPDX-License-Identifier: GPL-3.0-or-later
 * Engine core checks (driven by test_engine.py).
 *
 *   engine_test unit
 *       state binding, RNG, map accessors, vehicle/base bookkeeping, stacks,
 *       event sink and in-memory save/load checks; prints "<n> unit checks passed".
 *   engine_test roundtrip IN OUT
 *       game_read_bytes(IN), print a key=value summary, game_write_bytes -> OUT.
 *       Exit 0 on success, 20 when the file is rejected (error=<name> printed).
 *   engine_test daemon IN OUT
 *       load_daemon_bytes(IN) with all load-time fixups, save_daemon_bytes,
 *       then read that output again and require a byte-identical second save.
 */
#include "../engine.h"

#include <memory>

namespace alpha {
namespace engine {
size_t wire_size_counted(const char* name, bool legacy); // savegame.cpp
}
}

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

static bool write_file(const char* path, const std::vector<uint8_t>& data) {
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }
    bool ok = fwrite(data.data(), 1, data.size(), fp) == data.size();
    return (fclose(fp) == 0) && ok;
}

/* Small land map with one unit type, as a base for bookkeeping checks. */
static void setup_world(int width = 8, int height = 6) {
    *MapAreaX = width;
    *MapAreaY = height;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = (ALT_SHORE_LINE + 1) << 5; // land
        (*MapTiles)[i].region = 1;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        Units[i].plan = PLAN_DEFENSE;
        Units[i].preq_tech = TECH_Disable;
        Units[i].cost = 3;
    }
    Factions[1].target_x = -1;
    Factions[2].target_x = -1;
    set_alive(1, true);
    set_alive(2, true);
}

static void test_wire_sizes() {
    const char* names[] = {"VEH", "UNIT", "BASE", "Faction", "MFaction", "MAP", "GamePrefs",
        "Monument", "Continent", "MapMeta", "ReplayEvent"};
    for (const char* n : names) {
        CHECK(wire_size_record(n, false) > 0);
        CHECK_EQ(wire_size_counted(n, false), wire_size_record(n, false));
        CHECK_EQ(wire_size_counted(n, true), wire_size_record(n, true));
    }
    CHECK_EQ(wire_size_record("VEH", false), 0x34);
    CHECK_EQ(wire_size_record("BASE", false), 0x134);
    CHECK_EQ(wire_size_record("BASE", true), 0x130);
    CHECK_EQ(wire_size_record("Faction", false), 0x20CC);
    CHECK_EQ(wire_size_record("Faction", true), 0x20BC);
    CHECK_EQ(wire_size_record("GamePrefs", false), 0x398);
    CHECK_EQ(wire_size_record("GamePrefs", true), 0x31C);
    CHECK_EQ(wire_size_record("MapMeta", false), 0xAA4);
}

static void test_bind() {
    auto a = new_state();
    auto b = new_state();
    engine_bind(a.get());
    *VehCount = 7;
    VEH* va = Vehs;
    engine_bind(b.get());
    CHECK_EQ(*VehCount, 0);
    CHECK(Vehs != va);
    CHECK(Vehs == &b->vehs[1]);
    CHECK(engine_state() == b.get());
    engine_bind(a.get());
    CHECK_EQ(*VehCount, 7);
    CHECK_EQ(*SaveFileVersion, 12);
    CHECK_EQ(SecretProjects[0], -1);
    CHECK(ObjectiveReqVictory == &a->objectives[0]);
}

static void test_random() {
    auto s = new_state();
    engine_bind(s.get());
    game_srand(1);
    // MSVC rand() reference sequence for seed 1.
    const int expected[] = {41, 18467, 6334, 26500, 19169};
    for (int v : expected) {
        CHECK_EQ(game_rand(), v);
    }
    random_reseed(12345);
    int first[16];
    for (int& v : first) {
        v = random(1000);
        CHECK(v >= 0 && v < 1000);
    }
    auto t = new_state();
    engine_bind(t.get());
    random_reseed(12345);
    for (int v : first) {
        CHECK_EQ(random(1000), v);
    }
    // Generator state lives in the bound EngineState.
    engine_bind(s.get());
    uint32_t saved = random_state();
    engine_bind(t.get());
    CHECK_EQ(random_state(), saved);
    random(10);
    CHECK(random_state() != saved);
    engine_bind(s.get());
    CHECK_EQ(random_state(), saved);
    next_rand_seed(1, 2);
    uint32_t n1 = next_rand();
    next_rand_seed(1, 2);
    CHECK_EQ(next_rand(), n1);
}

static void test_map() {
    auto s = new_state();
    engine_bind(s.get());
    *MapAreaX = 8;
    *MapAreaY = 6;
    CHECK_EQ(map_init(), 0);
    CHECK_EQ(*MapAreaTiles, 24);
    CHECK_EQ(*MapHalfX, 4);
    CHECK(mapsq(0, 0) == &(*MapTiles)[0]);
    CHECK(mapsq(2, 0) == &(*MapTiles)[1]);
    CHECK(mapsq(1, 1) == &(*MapTiles)[4]);
    CHECK(mapsq(1, 0) == nullptr);
    CHECK(mapsq(8, 0) == nullptr);
    CHECK(mapsq(0, 6) == nullptr);
    CHECK(mapsq(-1, 1) == nullptr);
    CHECK_EQ(*MapAbstractCount, ((6 + 4) / 5) * ((((8 + 4) / 5) + 1) / 2));
    CHECK_EQ(wrap(-2), 6);
    CHECK_EQ(map_range(0, 0, 6, 0), 1); // cylinder wrap: dx 8-6=2, (dx+dy)/2
    CHECK_EQ(map_range(0, 0, 2, 2), 2);
    *MapAreaX = 7;
    CHECK_EQ(map_init(), 0); // Thinker map_init accepts odd widths; map_read rejects them
    *MapAreaX = 0;
    CHECK_EQ(map_init(), 1);
}

static void test_vehicles() {
    auto s = new_state();
    engine_bind(s.get());
    HeadlessSink* sink = headless_sink();
    sink->clear();
    setup_world();
    int v0 = veh_init(BSC_SCOUT_PATROL, 1, 2, 2);
    int v1 = veh_init(BSC_FORMERS, 1, 2, 2);
    int v2 = veh_init(BSC_SCOUT_PATROL, 2, 4, 2);
    CHECK_EQ(v0, 0);
    CHECK_EQ(v1, 1);
    CHECK_EQ(v2, 2);
    CHECK_EQ(*VehCount, 3);
    CHECK_EQ(Factions[1].units_active[BSC_SCOUT_PATROL], 1);
    CHECK_EQ(Factions[1].units_active[BSC_FORMERS], 1);
    CHECK_EQ(Vehs[v0].x, 2);
    CHECK_EQ(Vehs[v0].faction_id, 1);
    CHECK_EQ(Vehs[v0].home_base_id, -1);
    CHECK(veh_at(2, 2) >= 0);
    CHECK_EQ(veh_top(v0), veh_at(2, 2));
    CHECK_EQ(veh_who(2, 2), 1);
    CHECK_EQ(veh_who(4, 2), 2);
    CHECK_EQ(veh_who(0, 0), -1);
    CHECK((mapsq(2, 2)->items & BIT_VEH_IN_TILE));
    // Stack links: both vehicles on (2,2) are reachable from the top.
    int count = 0;
    for (int id = veh_top(v0); id >= 0; id = Vehs[id].next_veh_id_stack) {
        CHECK_EQ(Vehs[id].x, 2);
        count++;
    }
    CHECK_EQ(count, 2);
    stack_fix(v0);
    CHECK_EQ(veh_at(2, 2), veh_top(v0));
    CHECK_EQ(sink->notifications >= 3, 1);

    // veh_kill compacts like the original: later records shift down by one.
    size_t killed = sink->log.size();
    veh_kill(v0);
    CHECK_EQ(*VehCount, 2);
    CHECK_EQ(Factions[1].units_active[BSC_SCOUT_PATROL], 0);
    bool saw_killed = false;
    for (size_t i = killed; i < sink->log.size(); i++) {
        saw_killed |= sink->log[i].rfind("event 1 0 ", 0) == 0; // EV_VEH_KILLED veh 0
    }
    CHECK(saw_killed);
    CHECK_EQ(Vehs[0].unit_id, BSC_FORMERS);
    CHECK_EQ(Vehs[0].x, 2);
    CHECK_EQ(Vehs[1].faction_id, 2);
    CHECK_EQ(Vehs[1].x, 4);
    CHECK_EQ(veh_at(4, 2), 1);
    CHECK_EQ(veh_at(2, 2), 0);
    CHECK_EQ(Vehs[0].next_veh_id_stack, -1);
    CHECK_EQ(Vehs[0].prev_veh_id_stack, -1);
    // Lift and put back elsewhere.
    veh_put(0, 6, 2);
    CHECK_EQ(Vehs[0].x, 6);
    CHECK_EQ(veh_at(2, 2), -1);
    CHECK(!(mapsq(2, 2)->items & BIT_VEH_IN_TILE));
    CHECK_EQ(veh_at(6, 2), 0);
    // Capacity: MaxVehNum records.
    while (*VehCount < MaxVehNum) {
        int id = veh_init(BSC_SCOUT_PATROL, 1, 0, 0);
        CHECK(id >= 0);
        if (id < 0) {
            break;
        }
    }
    CHECK_EQ(*VehCount, MaxVehNum);
    CHECK_EQ(veh_init(BSC_SCOUT_PATROL, 1, 0, 0), -1);
    // A full stack of 2046 vehicles on one tile stays linked.
    count = 0;
    for (int id = veh_top(veh_at(0, 0)); id >= 0 && count <= MaxVehNum; id = Vehs[id].next_veh_id_stack) {
        count++;
    }
    CHECK_EQ(count, MaxVehNum - 2);
}

static void test_bases() {
    auto s = new_state();
    engine_bind(s.get());
    HeadlessSink* sink = headless_sink();
    sink->clear();
    setup_world(16, 10);
    int b0 = base_init(1, 2, 2);
    int b1 = base_init(2, 8, 4);
    int b2 = base_init(1, 12, 6);
    CHECK_EQ(b0, 0);
    CHECK_EQ(b1, 1);
    CHECK_EQ(b2, 2);
    CHECK_EQ(*BaseCount, 3);
    CHECK_EQ(Factions[1].base_count, 2);
    CHECK(mapsq(2, 2)->is_base());
    CHECK_EQ(base_at(8, 4), 1);
    CHECK_EQ(Bases[b0].pop_size, 1);
    CHECK(has_fac_built(FAC_HEADQUARTERS, b0));
    CHECK(!has_fac_built(FAC_HEADQUARTERS, b2));
    CHECK_EQ(Bases[b0].queue_items[0], BSC_COLONY_POD); // diff_level <= 1, turn 0
    CHECK_EQ(base_init(1, 2, 2), -1); // occupied
    // A vehicle created in a friendly base gets it as home.
    int v = veh_init(BSC_SCOUT_PATROL, 1, 12, 6);
    CHECK_EQ(Vehs[v].home_base_id, b2);
    int v_other = veh_init(BSC_SCOUT_PATROL, 2, 12, 6);
    CHECK_EQ(Vehs[v_other].home_base_id, -1);
    set_fac(FAC_RECYCLING_TANKS, b2, true);
    CHECK(has_fac_built(FAC_RECYCLING_TANKS, b2));
    // base_kill shifts later base ids down and fixes home_base_id.
    size_t before = sink->notifications;
    base_kill(b0);
    CHECK_EQ(*BaseCount, 2);
    CHECK_EQ(Factions[1].base_count, 1);
    CHECK(!mapsq(2, 2)->is_base());
    CHECK_EQ(Bases[0].x, 8);
    CHECK_EQ(Bases[1].x, 12);
    CHECK(has_fac_built(FAC_RECYCLING_TANKS, 1));
    CHECK_EQ(Vehs[v].home_base_id, 1);
    CHECK_EQ(base_at(12, 6), 1);
    CHECK(sink->notifications > before);
    // Capacity: MaxBaseNum records.
    auto big = new_state();
    engine_bind(big.get());
    setup_world(64, 32);
    int made = 0;
    for (int y = 0; y < 32 && made < MaxBaseNum; y++) {
        for (int x = y & 1; x < 64 && made < MaxBaseNum; x += 2) {
            CHECK(base_init(1 + (made & 1), x, y) >= 0);
            made++;
        }
    }
    CHECK_EQ(*BaseCount, MaxBaseNum);
    CHECK_EQ(base_init(1, 63, 31), -1);
}

static void test_events() {
    auto s = new_state();
    engine_bind(s.get());
    HeadlessSink* sink = headless_sink();
    sink->clear();
    CHECK(event_sink() == sink);
    sink->default_choice = 0;
    sink->choices["PICKME"] = 3;
    CHECK_EQ(popp(ScriptFile, "PICKME", 0, nullptr, nullptr), 3);
    CHECK_EQ(X_pop("OTHER", nullptr), 0);
    CHECK_EQ(sink->popups, 2);
    parse_says(0, "Gaians", 1, 0);
    parse_num(1, 42);
    CHECK(strcmp(ParseStrBuffer[0].str, "Gaians") == 0);
    CHECK_EQ(ParseNumTable[1], 42);
    size_t ni = sink->not_implemented;
    not_implemented("engine_test"); // (was the WP1 control_turn stub, now implemented)
    CHECK_EQ(sink->not_implemented, ni + 1);
    draw_map(1);
    CHECK_EQ(sink->redraws, 1);
    class Counting : public EventSink {
        public:
        int seen = 0;
        int popup(const PopupRequest&) override { return 7; }
        void notify(const EngineEvent&) override { seen++; }
    } counting;
    EventSink* old = set_event_sink(&counting);
    CHECK_EQ(X_pops("ANY", nullptr, nullptr), 7);
    engine_notify(EV_MESSAGE, 0, 0, 0, 0, -1, -1, "hello");
    CHECK_EQ(counting.seen, 1);
    set_event_sink(old);
    CHECK(event_sink() == sink);
}

static void test_save_memory() {
    auto s = new_state();
    engine_bind(s.get());
    setup_world(16, 10);
    *CurrentTurn = 5;
    base_init(1, 2, 2);
    base_init(2, 8, 4);
    veh_init(BSC_SCOUT_PATROL, 1, 2, 2);
    veh_init(BSC_FORMERS, 2, 6, 6);
    Units[3].weapon_id = 15;
    Bases[0].queue_items[1] = -70;
    MapWin->cOwner = 1;
    strcpy(s->scenario_name, "Engine core");
    s->save_tick_seed = 0x12345678;
    std::vector<uint8_t> first;
    CHECK_EQ(game_write_bytes(first), SAVE_OK);
    CHECK(first.size() > 0x31C + 0x398);

    auto t = new_state();
    engine_bind(t.get());
    SaveInfo info;
    CHECK_EQ(game_read_bytes(first.data(), first.size(), &info), SAVE_OK);
    CHECK_EQ(info.consumed, first.size());
    CHECK(info.has_tick_seed);
    CHECK_EQ(*VehCount, 2);
    CHECK_EQ(*BaseCount, 2);
    CHECK_EQ(*MapAreaX, 16);
    CHECK_EQ(*CurrentTurn, 5);
    CHECK_EQ(Bases[1].x, 8);
    CHECK_EQ(MapWin->cOwner, 1);
    CHECK_EQ(t->save_tick_seed, 0x12345678u);
    CHECK(strcmp(t->scenario_name, "Engine core") == 0);
    std::vector<uint8_t> second;
    CHECK_EQ(game_write_bytes(second), SAVE_OK);
    CHECK(first == second);

    // Legacy layout: written in v10 with the inverse conversions.
    engine_bind(s.get());
    *SaveFileVersion = 10;
    uint8_t* marker = s->legacy_prefs + 0x304;
    marker[0] = 10;
    std::vector<uint8_t> legacy;
    CHECK_EQ(game_write_bytes(legacy), SAVE_OK);
    CHECK_EQ(first.size() - legacy.size(),
        0x398 + 8 * (0x20CC - 0x20BC) + 2 * (0x134 - 0x130) + 0x20);
    auto u = new_state();
    engine_bind(u.get());
    CHECK_EQ(game_read_bytes(legacy.data(), legacy.size()), SAVE_OK);
    CHECK_EQ(*SaveFileVersion, 10);
    CHECK_EQ(Units[3].weapon_id, 15);
    CHECK_EQ(Bases[0].queue_items[1], -70);
    std::vector<uint8_t> legacy2;
    CHECK_EQ(game_write_bytes(legacy2), SAVE_OK);
    CHECK(legacy == legacy2);
    upgrade_save_version();
    std::vector<uint8_t> upgraded;
    CHECK_EQ(game_write_bytes(upgraded), SAVE_OK);
    CHECK_EQ(upgraded.size(), first.size());

    // Failed loads leave the bound state untouched.
    auto w = new_state();
    engine_bind(w.get());
    setup_world();
    veh_init(BSC_SCOUT_PATROL, 1, 2, 2);
    for (size_t cut : {(size_t)0, (size_t)11, (size_t)100, first.size() / 2, first.size() - 1}) {
        CHECK(game_read_bytes(first.data(), cut) != SAVE_OK);
        CHECK_EQ(*VehCount, 1);
        CHECK_EQ(*MapAreaX, 8);
    }
    std::vector<uint8_t> trailing = first;
    trailing.push_back(0);
    CHECK_EQ(game_read_bytes(trailing.data(), trailing.size()), SAVE_ERR_TRUNCATED);
    std::vector<uint8_t> bad = first;
    bad[12 + 0x31C + 0x380] = 13;
    CHECK_EQ(game_read_bytes(bad.data(), bad.size()), SAVE_ERR_VERSION);
    bad = first;
    bad[12 + 0x31C + 0x30 + 3] |= 0x10; // STATE_UNK_10000000
    CHECK_EQ(game_read_bytes(bad.data(), bad.size()), SAVE_ERR_ENCRYPTED);
    CHECK_EQ(*VehCount, 1);

    // Daemon path: fixups, then a stable re-save.
    CHECK_EQ(load_daemon_bytes(first.data(), first.size()), SAVE_LOAD_VALID);
    CHECK(is_alive(1));
    CHECK(is_alive(2));
    CHECK_EQ(*ComputeBaseID, -1);
    CHECK_EQ(*ControlTurnMove, 1);
}

static int run_unit() {
    auto base = new_state();
    engine_bind(base.get());
    config_vanilla(conf);
    test_wire_sizes();
    test_bind();
    test_random();
    test_map();
    test_vehicles();
    test_bases();
    test_events();
    test_save_memory();
    engine_bind(base.get());
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}

static void print_summary(const SaveInfo& info) {
    printf("version=%d marker=%d player=%d turn=%d year=%d vehicles=%d bases=%d width=%d height=%d"
        " replay=%d tick=%08x scenario=%d consumed=%zu\n",
        *SaveFileVersion, info.legacy_marker, MapWin->cOwner, *CurrentTurn, *CurrentMissionYear,
        *VehCount, *BaseCount, *MapAreaX, *MapAreaY, *ReplayEventSize,
        (unsigned)engine_state()->save_tick_seed, (*GameState & STATE_IS_SCENARIO) ? 1 : 0,
        info.consumed);
}

static int run_roundtrip(const char* in, const char* out) {
    auto s = new_state();
    engine_bind(s.get());
    config_vanilla(conf);
    std::vector<uint8_t> data = read_file(in);
    SaveInfo info;
    int error = game_read_bytes(data.data(), data.size(), &info);
    if (error != SAVE_OK) {
        printf("error=%s\n", save_error_name(error));
        return 20;
    }
    print_summary(info);
    std::vector<uint8_t> written;
    error = game_write_bytes(written);
    if (error != SAVE_OK || !write_file(out, written)) {
        printf("write-error=%s\n", save_error_name(error));
        return 21;
    }
    return 0;
}

static int run_daemon(const char* in, const char* out) {
    auto s = new_state();
    engine_bind(s.get());
    config_vanilla(conf);
    headless_sink()->clear();
    std::vector<uint8_t> data = read_file(in);
    int status = load_daemon_bytes(data.data(), data.size());
    if (status != SAVE_LOAD_VALID) {
        printf("load-status=%d\n", status);
        return 20;
    }
    std::vector<uint8_t> first;
    if (save_daemon_bytes(first) != SAVE_OK || !write_file(out, first)) {
        return 21;
    }
    auto t = new_state();
    engine_bind(t.get());
    if (load_daemon_bytes(first.data(), first.size()) != SAVE_LOAD_VALID) {
        printf("reload failed\n");
        return 22;
    }
    std::vector<uint8_t> second;
    if (save_daemon_bytes(second) != SAVE_OK || second != first) {
        printf("re-save differs\n");
        return 23;
    }
    printf("daemon-stable bytes=%zu not_implemented=%zu\n", first.size(),
        headless_sink()->not_implemented);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "unit")) {
        return run_unit();
    }
    if (argc == 4 && !strcmp(argv[1], "roundtrip")) {
        return run_roundtrip(argv[2], argv[3]);
    }
    if (argc == 4 && !strcmp(argv[1], "daemon")) {
        return run_daemon(argv[2], argv[3]);
    }
    fprintf(stderr, "usage: %s unit | roundtrip IN OUT | daemon IN OUT\n", argv[0]);
    return 2;
}
