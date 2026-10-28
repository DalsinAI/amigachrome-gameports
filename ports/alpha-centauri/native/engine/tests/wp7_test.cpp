/* SPDX-License-Identifier: GPL-3.0-or-later
 * World generation and new game checks (WP7), driven by test_engine_wp7.py.
 *
 *   wp7_test unit
 *       FastNoiseLite sample values, Path_continents on a drawn map,
 *       fixup_landmarks, setup_game resets, engine_new_game argument checks;
 *       prints noise bit patterns and "<n> unit checks passed".
 *   wp7_test map RULES_DIR base|expansion SEED SIZE OCEAN EROSION NATIVE OUT
 *       world_generate(SEED) on a WorldSizes[SIZE] map; writes the map tiles
 *       (every saved MAP field, little-endian) and the landmark table to OUT
 *       and prints a summary (tiles, land, regions, landmarks, fnv1a).
 *   wp7_test newgame RULES_DIR base|expansion SEED SIZE PLAYER OUT.SAV
 *       engine_new_game, start position checks, save_daemon_bytes to OUT.SAV,
 *       then load_daemon_bytes into a fresh state and save again: the two
 *       saves must be identical. Prints one line per faction start.
 * Exit status 0 on success, 1 on a failed check.
 */
#include "../engine.h"
#include "../mapgen.h"
#include "../newgame.h"
#include "../setup.h"
#include "../lib/FastNoiseLite.h"

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

static uint32_t fnv1a(const std::vector<uint8_t>& data) {
    uint32_t h = 2166136261u;
    for (uint8_t c : data) {
        h = (h ^ c) * 16777619u;
    }
    return h;
}

static void put8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

static void put32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; i++) {
        out.push_back((uint8_t)(v >> (8 * i)));
    }
}

/* Every MAP field written by the save (save_fields.inc order) plus the
 * landmark table and the map header values world_generate sets. */
static std::vector<uint8_t> map_bytes() {
    std::vector<uint8_t> out;
    put32(out, *MapAreaX);
    put32(out, *MapAreaY);
    put32(out, *MapRandomSeed);
    for (int i = 0; i < 7; i++) {
        put32(out, MapSizePlanet[i]);
    }
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP& m = (*MapTiles)[i];
        put8(out, m.climate);
        put8(out, m.contour);
        put8(out, m.val2);
        put8(out, m.region);
        put8(out, m.visibility);
        put8(out, m.val3);
        put8(out, m.unk_1);
        put8(out, (uint8_t)m.owner);
        put32(out, m.items);
        put32(out, m.landmarks);
        for (size_t k = 0; k < sizeof(m.visible_items) / sizeof(m.visible_items[0]); k++) {
            put32(out, m.visible_items[k]);
        }
    }
    put32(out, *MapLandmarkCount);
    for (int i = 0; i < *MapLandmarkCount; i++) {
        put32(out, Landmarks[i].x);
        put32(out, Landmarks[i].y);
        for (char c : Landmarks[i].name) {
            put8(out, (uint8_t)c);
        }
    }
    for (int i = 0; i < MaxRegionNum; i++) {
        put32(out, Continents[i].tile_count);
    }
    for (int i = 0; i < *MapAbstractCount; i++) {
        put8(out, (*MapAbstract)[i]);
    }
    put32(out, *VehCount);
    for (int i = 0; i < *VehCount; i++) {
        put32(out, Vehs[i].x);
        put32(out, Vehs[i].y);
        put32(out, Vehs[i].unit_id);
    }
    return out;
}

static uint32_t float_bits(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

/* Original #WORLDBUILDER defaults (the allowed ranges and defaults are
 * listed in CWorldbuilder); the synthetic rules files carry test values. */
static void worldbuilder_defaults() {
    CWorldbuilder& w = *WorldBuilder;
    w.land_base = 384; w.land_mod = 256; w.continent_base = 12; w.continent_mod = 24;
    w.hills_base = 1; w.hills_mod = 2; w.plateau_base = 4; w.plateau_mod = 8;
    w.rivers_base = 8; w.rivers_rain_mod = 12; w.solar_energy = 14; w.thermal_band = 14;
    w.thermal_deviance = 8; w.global_warming = 8; w.sea_level_rises = 5;
    w.cloudmass_peaks = 5; w.cloudmass_hills = 3; w.rainfall_coeff = 1;
    w.deep_water = 15; w.shelf = 10; w.plains = 15; w.beach = 10; w.hills = 10; w.peaks = 25;
    w.fungus = 1;
}

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    config_vanilla(conf);
    return s;
}

static int load_rules(const char* dir, const char* variant) {
    game_srand(1);
    random_reseed(1);
    if (engine_load_rules(dir, !strcmp(variant, "expansion"))) {
        printf("error=%s\n", engine_rules_error());
        return 1;
    }
    worldbuilder_defaults();
    return 0;
}

static int unit_checks() {
    /* FastNoiseLite as world_generate configures it. Bit patterns are printed
     * so the caller can compare host and m68k. */
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(12345);
    uint32_t h = 2166136261u;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            float v = noise.GetNoise(1.5f * 0.4f * x, 0.4f * y)
                + 0.3f * 3 * noise.GetNoise(-6.0f * x, -4.0f * y);
            CHECK(v > -3.0f && v < 3.0f);
            h = (h ^ float_bits(v)) * 16777619u;
        }
    }
    printf("noise=%08x\n", h);

    std::unique_ptr<EngineState> s = new_state();
    *MapAreaX = 20;
    *MapAreaY = 12;
    CHECK_EQ(map_init(), 0);
    /* All ocean except two separate land blocks; Path_continents numbers land
     * from 1 and ocean from 65 and puts the poles into 63/127. */
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            sq->climate = (uint8_t)(ALT_OCEAN_SHELF << 5);
            if (y >= 3 && y <= 8 && ((x >= 2 && x <= 6) || (x >= 12 && x <= 16))) {
                sq->climate = (uint8_t)(ALT_SHORE_LINE << 5);
            }
        }
    }
    Path_continents();
    CHECK_EQ(region_at(4, 4), 1);
    CHECK_EQ(region_at(14, 4), 2);
    CHECK_EQ(region_at(0, 2), 65);
    CHECK_EQ(region_at(0, 0), 127);
    CHECK_EQ(Continents[1].tile_count, Continents[2].tile_count);
    CHECK(Continents[1].tile_count > 0);
    int land = 0, total = 0;
    for (int i = 0; i < MaxRegionNum; i++) {
        total += Continents[i].tile_count;
        if (i < MaxRegionLandNum) {
            land += Continents[i].tile_count;
        }
    }
    // As in the original, the ocean fill also counts the pole rows it touches
    // before they are moved to region 127, so the total exceeds the tiles.
    CHECK_EQ(Continents[1].tile_count, 15);
    CHECK_EQ(Continents[127].tile_count, 20);
    CHECK_EQ(Continents[65].tile_count, *MapAreaTiles - 30 - 20 + 20);
    CHECK_EQ(total, *MapAreaTiles + 20);
    CHECK_EQ(land, 2 * Continents[1].tile_count + Continents[63].tile_count);
    CHECK(Continents[1].sea_coasts[(65 - MaxRegionLandNum) / 8] & (1 << ((65 - MaxRegionLandNum) & 7)));
    /* Running it twice gives the same tables. */
    std::vector<uint8_t> first = map_bytes();
    Path_continents();
    CHECK(map_bytes() == first);

    /* fixup_landmarks keeps a valid table unchanged and drops entries that
     * are off the map. */
    *MapLandmarkCount = 0;
    new_landmark(4, 4, "Alpha");
    new_landmark(14, 6, "Beta");
    std::vector<uint8_t> lm_before(sizeof(Landmark) * 64);
    memcpy(lm_before.data(), Landmarks, lm_before.size());
    fixup_landmarks();
    CHECK_EQ(*MapLandmarkCount, 2);
    CHECK(!memcmp(lm_before.data(), Landmarks, lm_before.size()));
    Landmarks[2].x = 400;
    Landmarks[2].y = 4;
    strcpy(Landmarks[2].name, "Far");
    Landmarks[3] = Landmarks[1];
    *MapLandmarkCount = 4;
    fixup_landmarks();
    CHECK_EQ(*MapLandmarkCount, 3);
    CHECK_EQ(Landmarks[2].x, 14);
    CHECK_EQ(Landmarks[3].x, 0);

    /* world_site rates land only. */
    CHECK_EQ(world_site(0, 2, 0), 0);
    CHECK(world_site(4, 4, 0) >= 0 && world_site(4, 4, 0) <= 15);

    /* engine_new_game rejects invalid settings without touching the state. */
    NewGameSettings bad;
    bad.player_faction = 0;
    CHECK_EQ(engine_new_game(bad), 1);
    bad.player_faction = 3;
    bad.map_size = 7;
    CHECK_EQ(engine_new_game(bad), 1);
    bad.map_size = 2;
    bad.ocean = 3;
    CHECK_EQ(engine_new_game(bad), 1);
    bad.ocean = 1;
    bad.map_width = 8;
    bad.map_height = 40;
    CHECK_EQ(engine_new_game(bad), 1);
    CHECK_EQ(*MapAreaX, 20);

    /* setup_game resets the turn, projects and the climate and switches the
     * save version to 12 (the original clears it with SunspotDuration). */
    *SaveFileVersion = 9;
    *CurrentTurn = 40;
    SecretProjects[3] = 5;
    *ClimateFutureChange = 2;
    *SunspotDuration = 4;
    *DiffLevel = 3;
    FactionStatus[0] = 2;
    setup_game(1);
    CHECK_EQ(*CurrentTurn, 0);
    CHECK_EQ(SecretProjects[3], -1);
    CHECK_EQ(*ClimateFutureChange, 0);
    CHECK_EQ(*SunspotDuration, 0);
    CHECK_EQ(*SaveFileVersion, 12);
    CHECK_EQ(*GovernorFaction, -1);
    CHECK(*GameState & STATE_SCENARIO_EDITOR);

    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}

static int map_case(char** argv) {
    std::unique_ptr<EngineState> s = new_state();
    if (load_rules(argv[2], argv[3])) {
        return 20;
    }
    uint32_t seed = (uint32_t)strtoul(argv[4], nullptr, 0);
    int size = atoi(argv[5]);
    if (size < 0 || size >= WorldSizeCount) {
        return 1;
    }
    *MapAreaX = 2 * WorldSizes[size][0];
    *MapAreaY = WorldSizes[size][1];
    *MapSizePlanet = size;
    *MapOceanCoverage = atoi(argv[6]);
    *MapErosiveForces = atoi(argv[7]);
    *MapNativeLifeForms = atoi(argv[8]);
    *MapPlanetaryOrbit = 1;
    *MapCloudCover = 1;
    if (map_init()) {
        return 1;
    }
    world_generate(seed);
    std::vector<uint8_t> bytes = map_bytes();
    FILE* f = fopen(argv[9], "wb");
    if (!f || fwrite(bytes.data(), 1, bytes.size(), f) != bytes.size()) {
        return 1;
    }
    fclose(f);

    int land = 0, rivers = 0, fungus = 0, rocky = 0;
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP* sq = &(*MapTiles)[i];
        land += !is_ocean(sq);
        rivers += (sq->items & BIT_RIVER) != 0;
        fungus += sq->is_fungus();
        rocky += sq->is_rocky();
        CHECK(sq->region != 0);
    }
    int regions = 0;
    for (int i = 0; i < MaxRegionNum; i++) {
        regions += Continents[i].tile_count > 0;
    }
    /* Landmarks are on the map and named. */
    for (int i = 0; i < *MapLandmarkCount; i++) {
        CHECK(mapsq(Landmarks[i].x, Landmarks[i].y) != nullptr);
        CHECK(Landmarks[i].name[0] != 0);
    }
    CHECK(land > 0 && land < *MapAreaTiles);
    CHECK_EQ((int)thinker_vars_get().map_random_value, (int)seed);
    printf("tiles=%d land=%d rivers=%d fungus=%d rocky=%d regions=%d landmarks=%d vehicles=%d fnv=%08x\n",
        *MapAreaTiles, land, rivers, fungus, rocky, regions, *MapLandmarkCount, *VehCount, fnv1a(bytes));
    return failures ? 1 : 0;
}

static bool start_ok(int faction_id, const std::vector<Point>& starts) {
    int x, y;
    if (!landing_site(faction_id, &x, &y)) {
        fprintf(stderr, "faction %d has no start\n", faction_id);
        return false;
    }
    MAP* sq = mapsq(x, y);
    bool aquatic = MFactions[faction_id].is_aquatic();
    bool ok = sq && aquatic == is_ocean(sq) && !sq->is_rocky() && !sq->is_fungus()
        && !(sq->items & BIT_MONOLITH) && y >= 2 && y < *MapAreaY - 2;
    for (auto& p : starts) {
        if (map_range(x, y, p.x, p.y) < 4) {
            ok = false;
        }
    }
    if (!ok) {
        fprintf(stderr, "faction %d start %d,%d is not valid\n", faction_id, x, y);
    }
    return ok;
}

static int newgame_case(char** argv) {
    std::unique_ptr<EngineState> s = new_state();
    if (load_rules(argv[2], argv[3])) {
        return 20;
    }
    NewGameSettings ng;
    ng.seed = (uint32_t)strtoul(argv[4], nullptr, 0);
    ng.map_size = atoi(argv[5]);
    ng.player_faction = atoi(argv[6]);
    HeadlessSink* sink = headless_sink();
    sink->clear();
    if (engine_new_game(ng)) {
        printf("error=engine_new_game\n");
        return 1;
    }
    CHECK_EQ(*CurrentTurn, 0);
    CHECK_EQ(MapWin->cOwner, ng.player_faction);
    CHECK(is_human(ng.player_faction));
    CHECK_EQ(*GameHalted, 0);
    std::vector<Point> starts;
    int spawned = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (!is_alive(i)) {
            continue;
        }
        CHECK(start_ok(i, starts));
        int x = -1, y = -1;
        landing_site(i, &x, &y);
        starts.push_back({x, y});
        int units = 0, bases = 0;
        for (int v = 0; v < *VehCount; v++) {
            units += Vehs[v].faction_id == i;
        }
        for (int b = 0; b < *BaseCount; b++) {
            bases += Bases[b].faction_id == i;
        }
        CHECK(units + bases > 0);
        spawned++;
        printf("start faction=%d x=%d y=%d units=%d bases=%d region=%d\n", i, x, y, units, bases, region_at(x, y));
    }
    CHECK(spawned >= 2);
    /* Planetfall was announced for the player. */
    bool planetfall_seen = false;
    for (auto& line : sink->log) {
        if (line.find("PLANETFALL") != std::string::npos) {
            planetfall_seen = true;
        }
    }
    CHECK(planetfall_seen);
    /* crash_landing centres the map on the player's landing site. */
    crash_landing(ng.player_faction);
    int px = -1, py = -1;
    landing_site(ng.player_faction, &px, &py);
    CHECK_EQ(MapWin->TileX, px);
    CHECK_EQ(MapWin->TileY, py);

    std::vector<uint8_t> save1;
    CHECK_EQ(save_daemon_bytes(save1), 0);
    FILE* f = fopen(argv[7], "wb");
    if (!f || fwrite(save1.data(), 1, save1.size(), f) != save1.size()) {
        return 1;
    }
    fclose(f);
    std::vector<uint8_t> before = map_bytes();
    const int vehs_before = *VehCount;
    const int bases_before = *BaseCount;

    std::unique_ptr<EngineState> t = new_state();
    if (load_rules(argv[2], argv[3])) {
        return 20;
    }
    int status = load_daemon_bytes(save1.data(), save1.size());
    CHECK_EQ(status, 0);
    std::vector<uint8_t> save2;
    CHECK_EQ(save_daemon_bytes(save2), 0);
    CHECK(map_bytes() == before);
    CHECK_EQ(*VehCount, vehs_before);
    CHECK_EQ(*BaseCount, bases_before);
    /* load_daemon recomputes PFLAG_MAP_REVEALED (mod_load_daemon); nothing
     * else may change. Apply the same flags to the original state: its save
     * must then equal the reloaded one byte for byte. */
    uint32_t flags[MaxPlayerNum];
    for (int i = 0; i < MaxPlayerNum; i++) {
        flags[i] = Factions[i].player_flags;
    }
    engine_bind(s.get());
    int revealed = 0;
    for (int i = 0; i < MaxPlayerNum; i++) {
        CHECK_EQ((Factions[i].player_flags ^ flags[i]) & ~PFLAG_MAP_REVEALED, 0);
        revealed += Factions[i].player_flags != flags[i];
        Factions[i].player_flags = flags[i];
    }
    std::vector<uint8_t> save1b;
    CHECK_EQ(save_daemon_bytes(save1b), 0);
    CHECK(save1b == save2);
    if (save1b != save2) {
        int shown = 0;
        for (size_t i = 0; i < save1b.size() && i < save2.size() && shown < 20; i++) {
            if (save1b[i] != save2[i]) {
                fprintf(stderr, "save differs at %zu: %02x %02x\n", i, save1b[i], save2[i]);
                shown++;
            }
        }
        if (save1b.size() != save2.size()) {
            fprintf(stderr, "save sizes %zu %zu\n", save1b.size(), save2.size());
        }
    }
    /* A second load and save is stable. */
    std::unique_ptr<EngineState> u = new_state();
    if (load_rules(argv[2], argv[3])) {
        return 20;
    }
    CHECK_EQ(load_daemon_bytes(save2.data(), save2.size()), 0);
    std::vector<uint8_t> save3;
    CHECK_EQ(save_daemon_bytes(save3), 0);
    CHECK(save3 == save2);
    printf("save bytes=%d fnv=%08x vehicles=%d bases=%d map_revealed_flags=%d reload=%s\n",
        (int)save1.size(), fnv1a(save1), *VehCount, *BaseCount, revealed,
        save1b == save2 && save3 == save2 ? "identical" : "different");
    if (failures) {
        fprintf(stderr, "%d of %d checks failed\n", failures, checks);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "unit")) {
        return unit_checks();
    }
    if (argc >= 10 && !strcmp(argv[1], "map")) {
        return map_case(argv);
    }
    if (argc >= 8 && !strcmp(argv[1], "newgame")) {
        return newgame_case(argv);
    }
    fprintf(stderr, "usage: wp7_test unit | map DIR VARIANT SEED SIZE OCEAN EROSION NATIVE OUT"
        " | newgame DIR VARIANT SEED SIZE PLAYER OUT.SAV\n");
    return 2;
}
