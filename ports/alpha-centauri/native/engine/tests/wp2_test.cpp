/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2 checks (driven by test_engine_wp2.py): base upkeep, production, unit
 * births, growth, riots, secret projects, production queue, stockpile energy,
 * hurry, maintenance, capture, monuments and base names, on synthetic states
 * with synthetic rules set in code (no game data files).
 *
 * The pure modules of the parent directory (cost_factor, base_psych,
 * base_nutrients, base_minerals, base_energy, base_support, base_commerce)
 * are linked in as differential oracles: randomized states are computed by
 * the engine and compared with the module's result for the same facts.
 *
 *   wp2_test unit      run everything, print "<n> wp2 checks passed"
 */
#include "../engine.h"
#include "../base_wp2.h"
#include "../base_deps.h"

#include "../../cost_factor.h"
#include "../../upkeep_base_psych.h"
#include "../../base_nutrients.h"
#include "../../upkeep_base_minerals.h"
#include "../../upkeep_base_energy.h"
#include "../../base_support.h"
#include "../../base_commerce.h"

#include <memory>

using namespace alpha::engine;

static int checks = 0;
static int failures = 0;
static int oracle_cases = 0;

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

/* Small deterministic generator for the randomized oracle sweeps. */
static uint32_t lcg_state = 12345;
static int rnd(int n) {
    lcg_state = lcg_state * 1103515245u + 12345u;
    return n > 0 ? (int)((lcg_state >> 8) % (uint32_t)n) : 0;
}
static int rnd_range(int lo, int hi) {
    return lo + rnd(hi - lo + 1);
}

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    base_compute_state_reset();
    headless_sink()->clear();
    return s;
}

static size_t popups_with(const char* label) {
    size_t n = 0;
    std::string key = std::string("#") + label;
    for (const std::string& line : headless_sink()->log) {
        size_t at = line.find(key);
        if (line.rfind("popup", 0) == 0 && at != std::string::npos) {
            size_t end = at + key.size();
            if (end == line.size() || line[end] == ' ') {
                n++;
            }
        }
    }
    return n;
}

static size_t log_with(const char* text) {
    size_t n = 0;
    for (const std::string& line : headless_sink()->log) {
        if (line.find(text) != std::string::npos) {
            n++;
        }
    }
    return n;
}

static char facility_names[SP_ID_Last + 1][16];
static char unit_names[64][16];

/* Synthetic rules close to the shape of alpha.txt (values chosen for tests,
 * not copied from the game data). */
static void setup_rules() {
    config_vanilla(conf);
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
    Rules->extra_cost_prototype_land = 50;
    Rules->extra_cost_prototype_sea = 50;
    Rules->extra_cost_prototype_air = 50;
    Rules->freq_global_warming_numerator = 1;
    Rules->freq_global_warming_denominator = 1;
    Rules->base_size_subspace_gen = 10;
    Rules->subspace_gen_req = 6;
    Rules->move_rate_roads = 3;
    WorldBuilder->sea_level_rises = 1;
    ResInfo->ocean_sq = {1, 0, 0, 0};
    ResInfo->base_sq = {2, 1, 1, 0};
    ResInfo->bonus_sq = {2, 1, 2, 0};
    ResInfo->forest_sq = {1, 2, 0, 0};
    ResInfo->recycling_tanks = {1, 1, 1, 0};
    ResInfo->improved_land = {1, 1, 0, 0};
    ResInfo->improved_sea = {1, 1, 2, 0};
    ResInfo->monolith_sq = {2, 2, 2, 0};
    ResInfo->borehole_sq = {0, 6, 6, 0};
    // Specialists: 0 technician (econ 3), 1 librarian (labs 3), 2 doctor (psych 2).
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
    // Weapons: 0 = no weapon (non-combat), 1 = gun (offense 1), 2 = psi (native).
    Weapon[0].offense_value = 0;
    Weapon[0].mode = WMODE_TRANSPORT;
    Weapon[1].offense_value = 1;
    Weapon[1].mode = WMODE_COMBAT;
    Weapon[1].cost = 1;
    Weapon[2].offense_value = -1;
    Weapon[2].mode = WMODE_COMBAT;
    Weapon[3].offense_value = 0;
    Weapon[3].mode = WMODE_COLONY;
    Armor[0].defense_value = 1;
    Armor[0].cost = 1;
    Chassis[0].triad = TRIAD_LAND;
    Chassis[0].speed = 1;
    Chassis[0].cost = 1;
    Chassis[1].triad = TRIAD_SEA;
    Chassis[1].speed = 4;
    Chassis[2].triad = TRIAD_AIR;
    Chassis[2].speed = 8;
    for (int i = 0; i < MaxProtoNum; i++) {
        UNIT& u = Units[i];
        memset(&u, 0, sizeof(u));
        u.plan = PLAN_DEFENSE;
        u.preq_tech = TECH_Disable;
        u.cost = 3;
        u.weapon_id = 1;
        u.chassis_id = 0;
        u.unit_flags = UNIT_ACTIVE | UNIT_PROTOTYPED;
        if (i < 64) {
            snprintf(unit_names[i], sizeof(unit_names[i]), "Unit%d", i);
            strcpy_n(u.name, sizeof(u.name), unit_names[i]);
        }
    }
    Units[BSC_COLONY_POD].plan = PLAN_COLONY;
    Units[BSC_COLONY_POD].weapon_id = 3;
    Units[BSC_COLONY_POD].cost = 6;
    Units[BSC_FORMERS].plan = PLAN_TERRAFORM;
    Units[BSC_FORMERS].weapon_id = 0;
    Units[BSC_MIND_WORMS].weapon_id = 2;
    Units[BSC_MIND_WORMS].plan = PLAN_OFFENSE;
}

static void setup_world(int width = 24, int height = 16) {
    setup_rules();
    *MapAreaX = width;
    *MapAreaY = height;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP* sq = &(*MapTiles)[i];
        sq->climate = ((ALT_SHORE_LINE + 1) << 5) | (2 << 3); // rainy land
        sq->val3 = (uint8_t)((sq->val3 & 0x3F) | (1 << 6)); // rolling
        sq->region = 1;
        sq->owner = -1;
    }
    for (int i = 1; i < 4; i++) {
        Factions[i].target_x = -1;
        set_alive(i, true);
        Factions[i].diff_level = 3;
        Factions[i].player_flags |= PFLAG_MAP_REVEALED; // all tiles workable without visibility data
        snprintf(MFactions[i].noun_faction, sizeof(MFactions[i].noun_faction), "Noun%d", i);
        MFactions[i].rule_hurry = 100;
    }
    set_human(1, true);
    *DiffLevel = 3;
    *MapSizePlanet = 2;
    *CurrentPlayerFaction = 1;
    MapWin->cOwner = 1;
    *CurrentTurn = 10;
}

/* A base with explicit worked tiles: governor off so base_yield keeps them. */
static int make_base(int faction_id, int x, int y, int pop) {
    int id = base_init(faction_id, x, y);
    CHECK(id >= 0);
    if (id < 0) {
        return id;
    }
    BASE* b = &Bases[id];
    b->pop_size = pop;
    b->governor_flags = 0;
    b->queue_items[0] = -FAC_STOCKPILE_ENERGY;
    b->production_id_last = b->queue_items[0];
    b->minerals_accumulated = 0;
    b->minerals_accumulated_2 = 0;
    b->worked_tiles = 0; // let base_yield allocate workers
    set_base(id);
    base_compute(1);
    return id;
}

/* ---------------- cost factor vs cost_factor.cpp ---------------- */

static void test_cost_factor_oracle() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 4, 4, 3);
    for (int iter = 0; iter < 3000; iter++) {
        int faction = rnd(2) ? 1 : 2;
        bool human = is_human(faction);
        int diff = rnd(6);
        *DiffLevel = diff;
        *MapSizePlanet = rnd(4);
        int multi = rnd(3) == 0 ? rnd_range(1, 30) : 10;
        Rules->mineral_cost_multi = multi;
        Rules->nutrient_cost_multi = multi;
        Factions[faction].SE_industry_pending = rnd_range(-5, 7);
        Factions[faction].SE_growth_pending = rnd_range(-4, 7);
        *MultiplayerActive = rnd(4) == 0;
        FactionRankings[7] = rnd(2) ? 1 : 2; // leader human or computer
        bool use_base = rnd(2) && Bases[b].faction_id == faction;
        Bases[b].faction_id = faction;
        set_fac(FAC_CHILDREN_CRECHE, b, rnd(2));
        Bases[b].state_flags = rnd(2) ? (uint32_t)BSTATE_GOLDEN_AGE_ACTIVE : 0u;
        for (int type = 0; type < 2; type++) {
            int got = mod_cost_factor(faction, (BaseResType)type, use_base ? b : -1);
            alpha::CostFactorFacts f = {};
            f.resource = type;
            f.multiplier = multi;
            f.map_size = *MapSizePlanet;
            f.difficulty = diff;
            f.industry_pending = max(Factions[faction].SE_industry_pending, -7);
            f.growth_pending = Factions[faction].SE_growth_pending;
            f.human = human;
            f.leader_threat = great_satan(FactionRankings[7], 0) != 0;
            f.leader_human = is_human(FactionRankings[7]);
            f.multiplayer_net = *MultiplayerActive != 0;
            f.base_present = use_base;
            f.children_creche = has_fac_built(FAC_CHILDREN_CRECHE, b);
            f.golden_age = Bases[b].golden_age_active();
            int32_t want = 0;
            CHECK(alpha::cost_factor(f, want) == alpha::CostFactorError::ok);
            CHECK_EQ(got, max(want, 1));
            oracle_cases++;
        }
    }
}

/* ---------------- psych vs base_psych.cpp ---------------- */

static void test_psych_oracle() {
    auto s = new_state();
    setup_world();
    conf = Config(); // Thinker defaults: base_psych=1, facility_talent_value {1,2,1,1} like the oracle
    int b = make_base(1, 6, 6, 4);
    make_base(1, 14, 6, 2);
    BASE* base = &Bases[b];
    for (int iter = 0; iter < 3000; iter++) {
        int faction = rnd(3) ? 1 : 2;
        base->faction_id = faction;
        Faction* f = &Factions[faction];
        MFaction* m = &MFactions[faction];
        *DiffLevel = rnd(6);
        f->diff_level = rnd(6);
        f->base_count = rnd_range(1, 40);
        f->SE_effic_pending = rnd_range(-4, 4);
        *MapAreaSqRoot = rnd_range(8, 60);
        base->pop_size = rnd_range(1, 25);
        base->specialist_total = rnd(4) ? rnd(base->pop_size + 1) / 2 : base->pop_size;
        base->assimilation_turns_left = rnd(3) ? 0 : rnd(50);
        m->rule_drone = rnd(4) ? 0 : rnd_range(1, 6);
        m->rule_talent = rnd(4) ? 0 : rnd_range(1, 6);
        int nodrone = rnd(4) ? 0 : rnd_range(1, 3);
        m->faction_bonus_count = nodrone ? 1 : 0;
        m->faction_bonus_id[0] = RULE_NODRONE;
        m->faction_bonus_val1[0] = nodrone;
        int talent = rnd_range(-4, 5);
        int police = rnd_range(-6, 4);
        base->psych_total = rnd(4) ? rnd(30) : 0;
        base->nerve_staple_turns_left = rnd(10) ? 0 : 5;
        memset(base->facilities_built, 0, sizeof(base->facilities_built));
        const FacilityId facs[] = {FAC_GENEJACK_FACTORY, FAC_RECREATION_COMMONS, FAC_HOLOGRAM_THEATRE,
            FAC_RESEARCH_HOSPITAL, FAC_NANOHOSPITAL, FAC_PARADISE_GARDEN, FAC_PUNISHMENT_SPHERE};
        for (FacilityId id : facs) {
            set_fac(id, b, rnd(4) == 0);
        }
        for (int i = 0; i <= SP_ID_Last - SP_ID_First; i++) {
            SecretProjects[i] = SP_Unbuilt;
        }
        const FacilityId projs[] = {FAC_PLANETARY_TRANSIT_SYSTEM, FAC_LONGEVITY_VACCINE,
            FAC_HUMAN_GENOME_PROJECT, FAC_CLINICAL_IMMORTALITY, FAC_TELEPATHIC_MATRIX};
        for (FacilityId id : projs) {
            SecretProjects[id - SP_ID_First] = rnd(5) == 0 ? b : SP_Unbuilt;
        }
        f->SE_Economics_pending = rnd(4);
        // Police units at the base (plain land combat units, value 1 or 2).
        while (*VehCount > 0) {
            veh_kill(*VehCount - 1);
        }
        int nunits = rnd(5);
        for (int i = 0; i < nunits; i++) {
            veh_init(BSC_SCOUT_PATROL, faction, base->x, base->y);
        }
        *BaseVehPacifismCount = rnd(3) ? 0 : rnd(4);
        mod_base_psych(b, talent, police);

        alpha::PsychFacts pf = {};
        int32_t content_pop, base_limit;
        psych_check(faction, &content_pop, &base_limit);
        pf.population = base->pop_size;
        pf.specialists = base->specialist_total;
        pf.content_population = content_pop;
        pf.base_limit = base_limit;
        pf.base_index = b;
        pf.faction_base_count = f->base_count;
        pf.difficulty = is_human(faction) ? f->diff_level : 3;
        pf.assimilation_turns = base->assimilation_turns_left;
        pf.rule_drone = m->rule_drone;
        pf.rule_talent = m->rule_talent;
        pf.no_drone_bonus = nodrone;
        pf.pending_talent = talent;
        pf.pending_police = police;
        pf.fresh_psych = base->psych_total;
        pf.genejack_drones = Rules->drones_induced_genejack_factory;
        pf.pacifism_units = *BaseVehPacifismCount;
        int val_police = 1 + (police >= 3);
        for (int i = 0; i < 3; i++) {
            pf.police_effects[i] = i < nunits ? val_police : 0;
        }
        pf.genejack = has_fac_built(FAC_GENEJACK_FACTORY, b);
        pf.recreation_commons = has_fac_built(FAC_RECREATION_COMMONS, b);
        pf.hologram_or_virtual = has_fac_built(FAC_HOLOGRAM_THEATRE, b);
        pf.research_hospital = has_fac_built(FAC_RESEARCH_HOSPITAL, b);
        pf.nanohospital = has_fac_built(FAC_NANOHOSPITAL, b);
        pf.paradise_garden = has_fac_built(FAC_PARADISE_GARDEN, b);
        pf.planetary_transit = has_project(FAC_PLANETARY_TRANSIT_SYSTEM, faction);
        pf.nerve_stapled = base->nerve_staple_turns_left > 0;
        pf.punishment_sphere = has_fac_built(FAC_PUNISHMENT_SPHERE, b);
        pf.telepathic_matrix = has_project(FAC_TELEPATHIC_MATRIX, faction);
        pf.longevity_vaccine = has_project(FAC_LONGEVITY_VACCINE, faction);
        pf.human_genome = has_project(FAC_HUMAN_GENOME_PROJECT, faction);
        pf.clinical_immortality = has_project(FAC_CLINICAL_IMMORTALITY, faction);
        pf.pending_economics = f->SE_Economics_pending;
        alpha::PsychResult pr = {};
        CHECK(alpha::base_psych_policy(pf, pr) == alpha::UpkeepPsychError::ok);
        CHECK_EQ(base->talent_total, pr.talents);
        CHECK_EQ(base->drone_total, pr.drones);
        CHECK_EQ(base->superdrone_total, pr.superdrones);
        if (base->specialist_total < base->pop_size) {
            for (int row = 0; row < 5; row++) {
                CHECK_EQ(BasePsychTalents[row], pr.rows[row][0]);
                CHECK_EQ(BasePsychNDrones[row], pr.rows[row][1]);
                CHECK_EQ(BasePsychSDrones[row], pr.rows[row][2]);
            }
        }
        oracle_cases++;
    }
}

/* ------- nutrients / minerals / energy / support vs the pure modules ------- */

static void randomize_tiles() {
    for (int i = 0; i < *MapAreaTiles; i++) {
        MAP* sq = &(*MapTiles)[i];
        if (sq->is_base()) {
            continue;
        }
        int alt = rnd(6) == 0 ? ALT_OCEAN_SHELF : ALT_SHORE_LINE + rnd(3);
        int rain = rnd(3);
        sq->climate = (uint8_t)((alt << 5) | (rain << 3));
        uint32_t keep = sq->items & (BIT_BASE_IN_TILE | BIT_VEH_IN_TILE | BIT_BASE_RADIUS);
        uint32_t items = 0;
        if (rnd(3) == 0) items |= BIT_FARM;
        if (rnd(4) == 0) items |= BIT_MINE;
        if (rnd(4) == 0) items |= BIT_SOLAR;
        if (rnd(5) == 0) items |= BIT_FOREST;
        if (rnd(4) == 0) items |= BIT_ROAD;
        if (rnd(8) == 0) items |= BIT_FUNGUS;
        if (rnd(10) == 0) items |= BIT_RIVER;
        sq->items = keep | items;
        sq->val3 = (uint8_t)((sq->val3 & 0x3F) | (rnd(3) << 6));
    }
}

static void test_accounting_oracles() {
    auto s = new_state();
    setup_world(32, 20);
    int b0 = make_base(1, 6, 6, 3);
    int b1 = make_base(1, 16, 8, 5);
    int b2 = make_base(1, 24, 12, 2);
    (void)b1; (void)b2;
    for (int iter = 0; iter < 600; iter++) {
        randomize_tiles();
        int b = rnd(3);
        BASE* base = &Bases[b];
        int faction = base->faction_id;
        Faction* f = &Factions[faction];
        *DiffLevel = rnd(6);
        f->diff_level = rnd(6);
        f->SE_growth_pending = rnd_range(-3, 4);
        f->SE_effic_pending = rnd_range(-4, 4);
        f->SE_support_pending = rnd_range(-4, 3);
        f->SE_police_pending = rnd_range(-5, 3);
        f->SE_planet_pending = rnd_range(-3, 3);
        f->SE_economy_pending = rnd_range(-2, 2);
        f->SE_alloc_labs = rnd(6);
        f->SE_alloc_psych = rnd(11 - f->SE_alloc_labs);
        f->tech_ranking = rnd(40);
        f->tech_count_transcendent = rnd(f->tech_ranking + 1) / 4;
        f->satellites_mineral = rnd(3);
        f->major_atrocities = rnd(3) ? 0 : rnd(3);
        f->clean_minerals_modifier = rnd(4);
        *MapNativeLifeForms = rnd(3);
        *MapSeaLevel = rnd(20);
        TectonicDetonationCount[faction] = rnd(4) ? 0 : 1;
        base->pop_size = rnd_range(1, 12);
        base->specialist_total = 0;
        base->worked_tiles = 0;
        base->nutrients_accumulated = rnd_range(-3, 30);
        memset(base->facilities_built, 0, sizeof(base->facilities_built));
        const FacilityId facs[] = {FAC_CHILDREN_CRECHE, FAC_ROBOTIC_ASSEMBLY_PLANT, FAC_GENEJACK_FACTORY,
            FAC_NANOREPLICATOR, FAC_QUANTUM_CONVERTER, FAC_CENTAURI_PRESERVE, FAC_TEMPLE_OF_PLANET,
            FAC_RECYCLING_TANKS, FAC_HEADQUARTERS};
        for (FacilityId id : facs) {
            set_fac(id, b, rnd(4) == 0);
        }
        base->state_flags = rnd(5) == 0 ? (uint32_t)BSTATE_GOLDEN_AGE_ACTIVE : 0u;
        // Supported units homed at the base: some outside territory.
        while (*VehCount > 0) {
            veh_kill(*VehCount - 1);
        }
        int nunits = rnd(7);
        for (int i = 0; i < nunits; i++) {
            int x = rnd(2) ? base->x : wrap(base->x + 2 * rnd_range(1, 5));
            int y = base->y + (x & 1) - (base->x & 1);
            if (!mapsq(x, y)) {
                x = base->x;
                y = base->y;
            }
            int unit_id = rnd(4) == 0 ? BSC_FORMERS : BSC_SCOUT_PATROL;
            int id = veh_init(unit_id, faction, x, y);
            if (id >= 0) {
                Vehs[id].home_base_id = b;
            }
        }
        *BaseUpkeepState = rnd(3);
        int social_effic_before[9];
        memcpy(social_effic_before, f->social_effic, sizeof(social_effic_before));
        int nutrient_total_before = f->nutrient_surplus_total;
        int stored_before = base->nutrients_accumulated;

        set_base(b);
        // Support first (the compute order), checked against base_support.cpp.
        mod_base_support();
        {
            alpha::SupportFacts sf = {f->SE_support_pending, base->SE_police(SE_Pending), base->pop_size};
            std::vector<alpha::SupportUnit> units;
            for (int i = 0; i < *VehCount; i++) {
                VEH* v = &Vehs[i];
                alpha::SupportUnit u = {};
                u.state = 0;
                u.plan = v->plan();
                u.offense = v->offense_value();
                u.triad = v->triad();
                u.homed_here = v->home_base_id == b && v->faction_id == faction;
                u.clean_reactor = false;
                u.native_on_fungus = v->is_native_unit() && mapsq(v->x, v->y)->is_fungus();
                MAP* sq = mapsq(v->x, v->y);
                u.outside_own_territory_and_base = !sq->is_base()
                    && whose_territory(faction, v->x, v->y, 0, 0) != faction;
                units.push_back(u);
            }
            std::vector<uint32_t> states(units.size());
            alpha::SupportResult sr = {};
            CHECK(alpha::base_support_policy(sf, units.data(), units.size(), sr, states.data(), states.size())
                == alpha::SupportError::ok);
            CHECK_EQ(*BaseForcesSupported, sr.supported);
            CHECK_EQ(*BaseForcesMaintCount, sr.paid_units);
            CHECK_EQ(*BaseForcesMaintCost, sr.mineral_cost);
            CHECK_EQ(*BaseVehPacifismCount, sr.pacifism_units);
            for (size_t i = 0; i < units.size(); i++) {
                const uint32_t mask = VSTATE_REQUIRES_SUPPORT | VSTATE_PACIFISM_DRONE | VSTATE_PACIFISM_FREE_SKIP;
                CHECK_EQ(Vehs[i].state & mask, states[i] & mask);
            }
        }
        mod_base_yield();
        // mod_base_yield ran nutrient/minerals/energy for the final allocation
        // inside its plans only when riots were possible; recompute the chain
        // from fresh intake and compare each stage.
        BASE fresh = *base;
        int fresh_nutrient = base->nutrient_intake_2;
        int fresh_mineral = base->mineral_intake_2;
        int fresh_energy = base->energy_intake_2;
        base->nutrients_accumulated = stored_before;
        f->nutrient_surplus_total = nutrient_total_before;
        mod_base_nutrient();
        {
            alpha::NutrientFacts nf = {};
            nf.population = base->pop_size;
            nf.pending_growth = f->SE_growth_pending;
            nf.intake = fresh_nutrient;
            nf.convoy_to = BaseResourceConvoyTo[RSC_NUTRIENT];
            nf.convoy_from = BaseResourceConvoyFrom[RSC_NUTRIENT];
            nf.requirement = Rules->nutrient_intake_req_citizen;
            nf.stored = stored_before;
            nf.faction_surplus = nutrient_total_before;
            nf.upkeep_stage = *BaseUpkeepState;
            nf.flags = (has_fac_built(FAC_CHILDREN_CRECHE, b) ? 1 : 0) | (base->golden_age_active() ? 2 : 0);
            alpha::BaseNutrientResult nr = {};
            CHECK(alpha::base_nutrients(nf, nr) == alpha::NutrientError::ok);
            CHECK_EQ(*BaseGrowthRate, nr.growth);
            CHECK_EQ(base->nutrient_intake_2, nr.intake);
            CHECK_EQ(base->nutrient_consumption, nr.consumption);
            CHECK_EQ(base->nutrient_surplus, nr.surplus);
            CHECK_EQ(base->nutrients_accumulated, nr.stored);
            CHECK_EQ(f->nutrient_surplus_total, nr.faction_surplus);
        }
        int raw_eco = base->eco_damage;
        mod_base_minerals();
        {
            alpha::MineralFacts mf = {};
            mf.fresh_intake = fresh_mineral;
            mf.convoy_to = BaseResourceConvoyTo[RSC_MINERAL];
            mf.forces_maintenance = *BaseForcesMaintCost;
            mf.convoy_from = BaseResourceConvoyFrom[RSC_MINERAL];
            mf.raw_eco_damage = raw_eco;
            mf.planet_ecology = f->clean_minerals_modifier;
            mf.satellites = f->satellites_mineral;
            mf.population = base->pop_size;
            mf.major_atrocities = f->major_atrocities;
            mf.tectonic_detonations = TectonicDetonationCount[faction];
            mf.sea_level = *MapSeaLevel;
            mf.sea_level_rises = WorldBuilder->sea_level_rises;
            mf.faction_difficulty = f->diff_level;
            mf.game_difficulty = *DiffLevel;
            mf.tech_ranking = f->tech_ranking;
            mf.transcendent_techs = f->tech_count_transcendent;
            mf.planet_pending = f->SE_planet_pending;
            mf.native_life = *MapNativeLifeForms;
            mf.quantum_converter = has_fac_built(FAC_QUANTUM_CONVERTER, b);
            mf.robotic_assembly = has_fac_built(FAC_ROBOTIC_ASSEMBLY_PLANT, b);
            mf.genejack = has_fac_built(FAC_GENEJACK_FACTORY, b);
            mf.nanoreplicator = has_fac_built(FAC_NANOREPLICATOR, b);
            mf.centauri_preserve = has_fac_built(FAC_CENTAURI_PRESERVE, b);
            mf.temple = has_fac_built(FAC_TEMPLE_OF_PLANET, b);
            mf.human = is_human(faction);
            alpha::UpkeepMineralResult mr = {};
            CHECK(alpha::base_minerals(mf, mr) == alpha::MineralError::ok);
            // The oracle applies the multiplier to convoyed minerals as the
            // original did; with no convoys both orders agree (Thinker Fix).
            CHECK_EQ(base->mineral_intake_2, mr.intake);
            CHECK_EQ(base->mineral_consumption, mr.consumption);
            CHECK_EQ(base->mineral_surplus, mr.surplus);
            CHECK_EQ(base->mineral_surplus_final, mr.surplus_final);
            CHECK_EQ(base->eco_damage, mr.eco_damage);
        }
        memcpy(f->social_effic, social_effic_before, sizeof(social_effic_before));
        mod_base_energy();
        {
            alpha::EnergyFacts ef = {};
            ef.fresh_intake = fresh_energy;
            ef.convoy_to = BaseResourceConvoyTo[RSC_ENERGY];
            ef.commerce = 0;
            ef.energy_grid = 0;
            ef.convoy_from = BaseResourceConvoyFrom[RSC_ENERGY];
            // Thinker measures the nearest of the faction's HQ bases (the
            // original had at most one); fewer than 36 bases keep the no-HQ
            // distance at the original 16.
            int hq_dist = -1;
            for (int i = 0; i < *BaseCount; i++) {
                if (Bases[i].faction_id == faction && has_fac_built(FAC_HEADQUARTERS, i)) {
                    int d = vector_dist(Bases[i].x, Bases[i].y, base->x, base->y);
                    hq_dist = hq_dist < 0 ? d : min(hq_dist, d);
                }
            }
            CHECK(f->base_count < 36);
            ef.headquarters_distance = hq_dist;
            ef.pending_efficiency = f->SE_effic_pending;
            ef.upkeep_stage = *BaseUpkeepState;
            ef.physical_creche = has_fac_built(FAC_CHILDREN_CRECHE, b);
            memcpy(ef.social_efficiency_before, social_effic_before, sizeof(social_effic_before));
            alpha::EnergyResult er = {};
            CHECK(alpha::base_energy_accounting(ef, er) == alpha::EnergyError::ok);
            CHECK_EQ(base->energy_intake_2, er.intake);
            CHECK_EQ(base->energy_consumption, er.consumption);
            CHECK_EQ(base->energy_inefficiency, er.inefficiency);
            CHECK_EQ(base->energy_surplus, er.surplus);
            for (int j = 0; j < 9; j++) {
                CHECK_EQ(f->social_effic[j], er.social_efficiency_after[j]);
            }
            // Energy split: psych + economy + labs = clamped surplus before multipliers.
            CHECK(base->psych_total >= 0 && base->economy_total >= 0 && base->labs_total >= 0);
        }
        (void)fresh;
        oracle_cases++;
    }
    (void)b0;
}

/* ---------------- commerce vs base_commerce.cpp ---------------- */

static void test_commerce_oracle() {
    auto s = new_state();
    setup_world(32, 20);
    make_base(1, 4, 4, 3);
    make_base(1, 10, 4, 4);
    make_base(2, 20, 10, 3);
    make_base(2, 26, 12, 5);
    for (int iter = 0; iter < 400; iter++) {
        for (int i = 0; i < *BaseCount; i++) {
            Bases[i].energy_intake = rnd(30);
        }
        bool treaty = rnd(4) != 0;
        bool pact = treaty && rnd(2);
        uint32_t flags = (treaty ? (uint32_t)DIPLO_TREATY : 0u) | (pact ? (uint32_t)DIPLO_PACT : 0u);
        Factions[1].diplo_status[2] = flags;
        Factions[2].diplo_status[1] = flags;
        Factions[1].tech_commerce_bonus = rnd(3);
        Factions[2].tech_commerce_bonus = rnd(3);
        *TechCommerceCount = rnd(20);
        *GovernorFaction = rnd(3);
        ProposalPassCount[PROP_GLOBAL_TRADE_PACT] = rnd(2);
        ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT] = 0;
        int b = rnd(2);
        set_base(b);
        BASE* base = &Bases[b];
        base->energy_intake_2 = base->energy_intake;
        base->energy_consumption = 0;
        mod_base_energy();
        int our_rank = own_base_rank(b);
        int their = base_rank(2, our_rank);
        alpha::CommercePartnerFacts cf = {};
        cf.fresh_our_intake = base->energy_intake;
        cf.fresh_their_intake = their >= 0 ? Bases[their].energy_intake : 0;
        cf.technology_count = *TechCommerceCount;
        cf.our_bonus = Factions[1].tech_commerce_bonus;
        cf.their_bonus = Factions[2].tech_commerce_bonus;
        cf.global_trade_pact = global_trade_pact();
        cf.pact = pact;
        cf.our_governor = *GovernorFaction == 1;
        cf.their_governor = *GovernorFaction == 2;
        alpha::CommercePartnerResult cr = {};
        CHECK(alpha::commerce_partner(cf, cr) == alpha::CommerceError::ok);
        if (treaty && their >= 0) {
            CHECK_EQ(BaseCommerceImport[2], cr.import_value);
            CHECK_EQ(BaseCommerceExport[2], cr.export_value);
        } else {
            CHECK_EQ(BaseCommerceImport[2], 0);
            CHECK_EQ(BaseCommerceExport[2], 0);
        }
        oracle_cases++;
    }
}

/* ---------------- production, births, queue, projects ---------------- */

static void test_unit_birth() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 3);
    BASE* base = &Bases[b];
    base->queue_items[0] = BSC_SCOUT_PATROL;
    base->production_id_last = BSC_SCOUT_PATROL;
    Factions[1].units_queue[BSC_SCOUT_PATROL] = 1;
    int cost = mineral_cost(b, BSC_SCOUT_PATROL);
    CHECK_EQ(cost, Units[BSC_SCOUT_PATROL].cost * mod_cost_factor(1, RSC_MINERAL, -1));
    base->minerals_accumulated = cost - 1;
    int veh_before = *VehCount;
    // Force a positive mineral surplus by working the base tile only.
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK(base->mineral_surplus >= 1);
    CHECK_EQ(*VehCount, veh_before + 1);
    VEH* v = &Vehs[*VehCount - 1];
    CHECK_EQ(v->unit_id, BSC_SCOUT_PATROL);
    CHECK_EQ(v->x, base->x);
    CHECK_EQ(v->y, base->y);
    CHECK_EQ(v->faction_id, 1);
    CHECK_EQ(v->home_base_id, b);
    CHECK_EQ(v->morale, 1); // morale_mod 0 + 1
    CHECK(base->state_flags & BSTATE_PRODUCTION_DONE);
    CHECK(popups_with("PRODUCE") >= 1);
    // Leftover minerals for human players are capped at min(retool_exemption, surplus).
    CHECK(base->minerals_accumulated <= max(0, min(Rules->retool_exemption, base->mineral_surplus)));
    // Command center adds +2 land morale on births.
    set_fac(FAC_COMMAND_CENTER, b, true);
    base->minerals_accumulated = cost;
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK_EQ(Vehs[*VehCount - 1].morale, 3);

    // Colony pod at pop 2 reduces the population; at pop 1 (human, diff > 1)
    // the ABANDONBASE question is asked and the default answer keeps the base.
    base->queue_items[0] = BSC_COLONY_POD;
    base->production_id_last = BSC_COLONY_POD;
    base->pop_size = 2;
    base->minerals_accumulated = mineral_cost(b, BSC_COLONY_POD);
    int vc = *VehCount;
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK_EQ(*VehCount, vc + 1);
    CHECK_EQ(Vehs[*VehCount - 1].unit_id, BSC_COLONY_POD);
    CHECK(Bases[b].pop_size == 1 || Bases[b].pop_size == 2); // may grow back the same turn
    base->pop_size = 1;
    base->nutrients_accumulated = 0;
    base->minerals_accumulated = mineral_cost(b, BSC_COLONY_POD);
    vc = *VehCount;
    headless_sink()->clear();
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK_EQ(*VehCount, vc);
    CHECK(popups_with("ABANDONBASE") + popups_with("ABANDONBASE1") >= 1);
    CHECK_EQ(*BaseCount, 1);
    // Answer "abandon" (2): the pod is built and the base is removed.
    // Integration: WP8's mod_eliminate_player (Thinker rule) removes a faction
    // left without a base together with its units, so faction 1 keeps a
    // second base for the new pod to survive.
    make_base(1, 18, 12, 1);
    headless_sink()->choices["ABANDONBASE"] = 2;
    headless_sink()->choices["ABANDONBASE1"] = 2;
    Bases[b].minerals_accumulated = mineral_cost(b, BSC_COLONY_POD);
    CHECK_EQ(mod_base_upkeep(b), 1);
    CHECK_EQ(*BaseCount, 1);
    CHECK_EQ(Vehs[*VehCount - 1].unit_id, BSC_COLONY_POD);
    CHECK_EQ(Vehs[*VehCount - 1].home_base_id, -1);
}

static void test_prototype_cost() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 3);
    int unit_id = 1 * MaxProtoFactionNum + 2;
    Units[unit_id].unit_flags = UNIT_ACTIVE; // not yet prototyped
    Units[unit_id].cost = 4;
    int base_cost = mod_base_cost(unit_id);
    int32_t has_proto = 0;
    int cost = mod_veh_cost(unit_id, b, &has_proto);
    CHECK_EQ(has_proto, 1);
    CHECK_EQ(cost, 4 + (50 * base_cost + 50) / 100);
    set_fac(FAC_SKUNKWORKS, b, true);
    CHECK_EQ(mod_veh_cost(unit_id, b, &has_proto), 4);
    CHECK_EQ(has_proto, 0);
    // Building it marks the prototype done and adds one morale level.
    set_fac(FAC_SKUNKWORKS, b, false);
    Bases[b].queue_items[0] = unit_id;
    Bases[b].production_id_last = unit_id;
    Bases[b].minerals_accumulated = mineral_cost(b, unit_id);
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK(Units[unit_id].unit_flags & UNIT_PROTOTYPED);
    CHECK_EQ(Vehs[*VehCount - 1].unit_id, unit_id);
    CHECK_EQ(Vehs[*VehCount - 1].morale, 2);
    CHECK_EQ(Monuments[1].events[MON_PROTOTYPE_BUILT].achieved, 1);
}

static void test_facility_queue_and_stockpile() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 4);
    BASE* base = &Bases[b];
    base->queue_items[0] = -FAC_RECYCLING_TANKS;
    base->production_id_last = base->queue_items[0];
    CHECK_EQ(base_queue_count(b), 1);
    CHECK_EQ(base_queue_append(b, -FAC_PERIMETER_DEFENSE), 1);
    CHECK_EQ(base_queue_append(b, -FAC_NETWORK_NODE), 2);
    CHECK_EQ(base_queue_insert(b, 1, -FAC_CHILDREN_CRECHE), 1);
    CHECK_EQ(base_queue_count(b), 4);
    CHECK_EQ(base->queue_items[1], -FAC_CHILDREN_CRECHE);
    CHECK_EQ(base->queue_items[2], -FAC_PERIMETER_DEFENSE);
    CHECK_EQ(base_queue_move(b, 3, 1), 1);
    CHECK_EQ(base->queue_items[1], -FAC_NETWORK_NODE);
    CHECK_EQ(base->queue_items[2], -FAC_CHILDREN_CRECHE);
    CHECK_EQ(base_queue_remove(b, 2), 2);
    CHECK_EQ(base_queue_count(b), 3);
    CHECK_EQ(base->queue_items[2], -FAC_PERIMETER_DEFENSE);
    CHECK(base_queue_valid_item(b, -FAC_CHILDREN_CRECHE));
    CHECK(!base_queue_valid_item(b, -FAC_RECYCLING_TANKS)); // queued at slot 0
    // Complete the current item: it is built and the queue advances.
    base->minerals_accumulated = mineral_cost(b, -FAC_RECYCLING_TANKS);
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK(has_fac_built(FAC_RECYCLING_TANKS, b));
    CHECK_EQ(base->queue_items[0], -FAC_NETWORK_NODE);
    CHECK_EQ(base_queue_count(b), 2);
    CHECK_EQ(base->production_id_last, -FAC_NETWORK_NODE);
    CHECK_EQ(Monuments[1].events[MON_FACILITY_BUILT].achieved, 1);
    // A queued facility that is already built is skipped by base_queue.
    set_fac(FAC_NETWORK_NODE, b, true);
    base->minerals_accumulated = 0;
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK_EQ(base->queue_items[0], -FAC_PERIMETER_DEFENSE);
    // Last item completes: human bases without governor switch to Stockpile Energy.
    base->minerals_accumulated = mineral_cost(b, -FAC_PERIMETER_DEFENSE);
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK(has_fac_built(FAC_PERIMETER_DEFENSE, b));
    CHECK_EQ(base->queue_items[0], -FAC_STOCKPILE_ENERGY);
    // Stockpile energy: credits grow by stockpile_energy, minerals are not stored.
    int credits = Factions[1].energy_credits;
    base->minerals_accumulated = 0;
    set_base(b);
    base_compute(1);
    int expect = stockpile_energy(b);
    CHECK_EQ(expect, base->mineral_surplus > 0 ? (base->mineral_surplus + 1) / 2 : 0);
    int econ = 0;
    CHECK_EQ(mod_base_upkeep(b), 0);
    econ = base->economy_total;
    CHECK_EQ(base->minerals_accumulated, 0);
    CHECK(Factions[1].energy_credits >= credits + expect + econ - 20); // maintenance may apply
    CHECK(expect > 0);
    // Planetary Energy Grid raises stockpile output by a quarter.
    SecretProjects[FAC_PLANETARY_ENERGY_GRID - SP_ID_First] = b;
    CHECK_EQ(stockpile_energy(b), (5 * ((base->mineral_surplus + 1) / 2) + 3) / 4);
    // Clearing the queue keeps the current item.
    base_queue_append(b, -FAC_HAB_COMPLEX);
    base_queue_clear(b);
    CHECK_EQ(base_queue_count(b), 1);
    CHECK_EQ(base->queue_items[0], -FAC_STOCKPILE_ENERGY);
}

static void test_retool_and_change() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 4);
    BASE* base = &Bases[b];
    base->queue_items[0] = BSC_SCOUT_PATROL;
    base->production_id_last = BSC_SCOUT_PATROL;
    base->minerals_accumulated = 30;
    base->minerals_accumulated_2 = 30;
    Factions[1].units_queue[BSC_SCOUT_PATROL] = 1;
    // Unit -> facility under RETOOL_FREE_PROJECT: 50% penalty above the exemption.
    mod_base_change(b, -FAC_RECYCLING_TANKS);
    CHECK_EQ(base->queue_items[0], -FAC_RECYCLING_TANKS);
    CHECK_EQ(Factions[1].units_queue[BSC_SCOUT_PATROL], 0);
    CHECK_EQ(base->minerals_accumulated, 30 - (30 - (100 - 50) * (30 - 10) / 100 - 10));
    CHECK_EQ(mod_base_lose_minerals(b, 0), 10);
    // Changing back costs nothing.
    mod_base_change(b, BSC_SCOUT_PATROL);
    CHECK_EQ(base->minerals_accumulated, 30);
    CHECK_EQ(Factions[1].units_queue[BSC_SCOUT_PATROL], 1);
    // Switching between projects is free with RETOOL_FREE_PROJECT.
    base->queue_items[0] = -FAC_HUMAN_GENOME_PROJECT;
    base->production_id_last = -FAC_HUMAN_GENOME_PROJECT;
    mod_base_change(b, -FAC_WEATHER_PARADIGM);
    CHECK_EQ(base->minerals_accumulated, 30);
    CHECK_EQ(mod_base_making(-FAC_WEATHER_PARADIGM, b), -SP_ID_First);
    Rules->retool_strictness = RETOOL_ALWAYS_FREE;
    CHECK_EQ(mod_base_making(BSC_SCOUT_PATROL, b), 0);
    Rules->retool_strictness = RETOOL_NEVER_FREE;
    CHECK_EQ(mod_base_making(BSC_SCOUT_PATROL, b), BSC_SCOUT_PATROL);
}

static void test_secret_project() {
    auto s = new_state();
    setup_world(32, 20);
    for (int i = 0; i <= SP_ID_Last - SP_ID_First; i++) {
        SecretProjects[i] = SP_Unbuilt;
    }
    int b0 = make_base(1, 6, 6, 4);
    int b1 = make_base(2, 20, 10, 4); // computer faction also building it
    int b2 = make_base(2, 26, 12, 4);
    Bases[b0].queue_items[0] = -FAC_HUMAN_GENOME_PROJECT;
    Bases[b0].production_id_last = -FAC_HUMAN_GENOME_PROJECT;
    Bases[b1].queue_items[0] = -FAC_HUMAN_GENOME_PROJECT;
    Bases[b2].queue_items[0] = -FAC_WEATHER_PARADIGM;
    Bases[b1].minerals_accumulated = 5;
    Bases[b2].minerals_accumulated = 1;
    CHECK(facility_avail(FAC_HUMAN_GENOME_PROJECT, 1, b0, 0));
    Bases[b0].minerals_accumulated = mineral_cost(b0, -FAC_HUMAN_GENOME_PROJECT);
    CHECK_EQ(mod_base_upkeep(b0), 0);
    CHECK_EQ(project_base(FAC_HUMAN_GENOME_PROJECT), b0);
    CHECK(has_project(FAC_HUMAN_GENOME_PROJECT, 1));
    CHECK(!facility_avail(FAC_HUMAN_GENOME_PROJECT, 2, b1, 0));
    CHECK_EQ(Monuments[1].events[MON_SECRET_PROJECT].achieved, 1);
    CHECK_EQ(Monuments[1].events[MON_SECRET_PROJECT].param2, FAC_HUMAN_GENOME_PROJECT - SP_ID_First);
    // The computer base that was building it swaps with its own other project
    // base that has a lower score.
    CHECK_EQ(Bases[b1].queue_items[0], -FAC_WEATHER_PARADIGM);
    // b2 then held the completed project; with no cheaper project base left
    // it is reset (mod_base_reset), so it no longer builds it.
    CHECK(Bases[b2].queue_items[0] != -FAC_HUMAN_GENOME_PROJECT);
    // Building an already completed project: the human base gets ALREADYPROJECT.
    headless_sink()->clear();
    Bases[b0].queue_items[0] = -FAC_HUMAN_GENOME_PROJECT;
    Bases[b0].minerals_accumulated = 100;
    CHECK_EQ(mod_base_upkeep(b0), 0);
    CHECK_EQ(popups_with("ALREADYPROJECT1"), 1);
    // Projects count for redundant facilities and survive base removal as destroyed.
    SecretProjects[FAC_COMMAND_NEXUS - SP_ID_First] = b0;
    CHECK(redundant(FAC_COMMAND_CENTER, 1));
    CHECK_EQ(morale_mod(b0, 1, TRIAD_LAND), 2);
    base_kill(b0);
    CHECK_EQ(SecretProjects[FAC_HUMAN_GENOME_PROJECT - SP_ID_First], SP_Destroyed);
}

static void test_growth_and_starvation() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 2);
    BASE* base = &Bases[b];
    set_base(b);
    base_compute(1);
    int cost = (base->pop_size + 1) * mod_cost_factor(1, RSC_NUTRIENT, b);
    CHECK_EQ(cost, 3 * 10);
    // Enough stored nutrients: the base grows and the box empties.
    base->nutrients_accumulated = cost;
    *BaseGrowthRate = 0;
    base->nutrient_surplus = 1;
    CHECK_EQ(mod_base_growth(), 0);
    CHECK_EQ(base->pop_size, 3);
    CHECK_EQ(base->nutrients_accumulated, base->nutrient_surplus);
    // Population limit without a Hab Complex.
    base->pop_size = Rules->pop_limit_wo_hab_complex;
    set_base(b);
    base_compute(1);
    base->nutrients_accumulated = (base->pop_size + 1) * mod_cost_factor(1, RSC_NUTRIENT, b);
    headless_sink()->clear();
    *BaseGrowthRate = 0;
    CHECK_EQ(mod_base_growth(), 0);
    CHECK_EQ(base->pop_size, Rules->pop_limit_wo_hab_complex);
    CHECK_EQ(popups_with("POPULATIONLIMIT"), 1);
    CHECK_EQ(base_unused_space(b), 0);
    set_fac(FAC_HAB_COMPLEX, b, true);
    CHECK_EQ(base_unused_space(b), Rules->pop_limit_wo_hab_dome - Rules->pop_limit_wo_hab_complex);
    // Population boom: grows every turn with surplus >= requirement.
    base->pop_size = 4;
    base->nutrients_accumulated = 3;
    base->nutrient_surplus = Rules->nutrient_intake_req_citizen;
    *BaseGrowthRate = GrowthPopBoom;
    CHECK_EQ(mod_base_growth(), 0);
    CHECK_EQ(base->pop_size, 5);
    // Starvation: negative box removes a citizen.
    base->pop_size = 3;
    base->nutrients_accumulated = -1;
    base->nutrient_surplus = -2;
    headless_sink()->clear();
    CHECK_EQ(mod_base_growth(), 0);
    CHECK_EQ(base->pop_size, 2);
    CHECK_EQ(popups_with("STARVE"), 1);
    CHECK_EQ(base->nutrients_accumulated, -2);
    // A size 1 base starving to death is removed.
    base->pop_size = 1;
    base->nutrients_accumulated = -1;
    CHECK_EQ(mod_base_growth(), 1);
    CHECK_EQ(*BaseCount, 0);
}

static void test_riots() {
    auto s = new_state();
    setup_world();
    conf.content_pop_player[3] = 1;
    int b = make_base(1, 6, 6, 1);
    make_base(1, 14, 6, 1);
    BASE* base = &Bases[b];
    base->pop_size = 6;
    base->queue_items[0] = -FAC_RECYCLING_TANKS;
    base->production_id_last = base->queue_items[0];
    Factions[1].SE_alloc_psych = 0;
    Factions[1].SE_police_pending = -2; // no police
    set_base(b);
    base_compute(1);
    // With no psych and content pop 1 the base riots unless specialists fix it;
    // force workers with the governor off and no specialists.
    CHECK(base->drone_total + base->superdrone_total >= 0);
    base->drone_total = 3;
    base->talent_total = 0;
    headless_sink()->clear();
    mod_base_drones();
    CHECK(base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE);
    CHECK_EQ(popups_with("DRONERIOTS"), 1);
    // Riots stop production: minerals are not accumulated.
    base->minerals_accumulated = 0;
    base->mineral_surplus = 3;
    base->nutrients_accumulated = 0;
    set_base(b);
    mod_base_production();
    CHECK_EQ(base->minerals_accumulated, 0);
    // Riots end when talents >= drones.
    base->drone_total = 0;
    base->talent_total = 1;
    headless_sink()->clear();
    mod_base_drones();
    CHECK(!(base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE));
    CHECK_EQ(popups_with("DRONERIOTSOVER"), 1);
    // Golden age: no drones, pop > 2, talents >= half.
    base->drone_total = 0;
    base->talent_total = 4;
    base->pop_size = 6;
    headless_sink()->clear();
    mod_base_drones();
    CHECK(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE);
    CHECK_EQ(popups_with("GOLDENAGE"), 1);
    base->talent_total = 1;
    mod_base_drones();
    CHECK(!(base->state_flags & BSTATE_GOLDEN_AGE_ACTIVE));
    CHECK_EQ(popups_with("GOLDENAGEOVER"), 1);
    // A second riot turn can destroy a facility (not on the first turn, not HQ).
    set_fac(FAC_HEADQUARTERS, b, false);
    for (int i = 1; i < FAC_SKY_HYDRO_LAB; i++) {
        if (i != FAC_HEADQUARTERS) {
            set_fac((FacilityId)i, b, true);
        }
    }
    Factions[1].SE_police_pending = -6; // police -4 here: a 1 in 3 chance each riot turn
    base->drone_total = 5;
    base->talent_total = 0;
    base->state_flags |= BSTATE_DRONE_RIOTS_ACTIVE;
    int built_before = 0, built_after = 0, riot_turns = 0;
    for (int i = 1; i < FAC_SKY_HYDRO_LAB; i++) built_before += has_fac_built((FacilityId)i, b);
    game_srand(7);
    for (; riot_turns < 50; riot_turns++) {
        headless_sink()->clear();
        mod_base_drones();
        built_after = 0;
        for (int i = 1; i < FAC_SKY_HYDRO_LAB; i++) built_after += has_fac_built((FacilityId)i, b);
        if (built_after != built_before) {
            break;
        }
        CHECK_EQ(popups_with("DRONERIOT"), 0);
    }
    CHECK(riot_turns < 50);
    CHECK_EQ(built_after, built_before - 1);
    CHECK_EQ(popups_with("DRONERIOT"), 1);
    CHECK(!has_fac_built(FAC_HEADQUARTERS, b)); // removed above: riots never destroy HQ bases
    // Nerve staple ends riots and stops them.
    base->nerve_staple_turns_left = 0;
    conf.nerve_staple_turns = 10;
    base->nerve_staple_count = 0;
    action_staple(b);
    CHECK(base->nerve_staple_count == 1);
    CHECK_EQ(base->nerve_staple_turns_left, 10);
    CHECK(!(base->state_flags & BSTATE_DRONE_RIOTS_ACTIVE));
}

static void test_hurry_and_maint() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 3);
    BASE* base = &Bases[b];
    base->queue_items[0] = -FAC_RECYCLING_TANKS;
    base->minerals_accumulated = 2;
    int mins = mineral_cost(b, -FAC_RECYCLING_TANKS) - 2;
    int expect = 2 * mins * 2; // facility 2x, doubled below retool exemption (vanilla)
    CHECK_EQ(hurry_cost(b, -FAC_RECYCLING_TANKS, mins), expect);
    CHECK_EQ(base_hurry_quote(b, -1), expect);
    Factions[1].energy_credits = expect - 1;
    CHECK_EQ(base_hurry_player(b, -1), HURRY_NO_CREDITS);
    Factions[1].energy_credits = expect + 5;
    CHECK_EQ(base_hurry_player(b, -1), HURRY_DONE);
    CHECK_EQ(Factions[1].energy_credits, 5);
    CHECK_EQ(base->minerals_accumulated, mineral_cost(b, -FAC_RECYCLING_TANKS));
    CHECK(base->state_flags & BSTATE_HURRY_PRODUCTION);
    CHECK_EQ(base_hurry_player(b, -1), HURRY_NOTHING);
    // Units: mins^2/20 + 2*mins, projects 2x or 4x more, rule_hurry percent.
    base->minerals_accumulated = 12;
    CHECK_EQ(hurry_cost(b, BSC_SCOUT_PATROL, 18), (18 * 18 / 20 + 2 * 18));
    MFactions[1].rule_hurry = 50;
    CHECK_EQ(hurry_cost(b, BSC_SCOUT_PATROL, 18), (18 * 18 / 20 + 2 * 18) / 2);
    MFactions[1].rule_hurry = 100;
    // Projects: 2*mins, 4x while fewer than 4 rows are stored: 20 of 188 minerals.
    CHECK_EQ(hurry_cost(b, -FAC_HUMAN_GENOME_PROJECT, 20), 20 * (2 * 188 * 4) / 188);
    // Partial hurry rounds up.
    CHECK_EQ(hurry_cost(b, BSC_SCOUT_PATROL, 18) > 0, 1);
    // Upkeep hurry flag is cleared at the next upkeep.
    base->queue_items[0] = -FAC_STOCKPILE_ENERGY;
    CHECK_EQ(mod_base_upkeep(b), 0);
    CHECK(!(base->state_flags & BSTATE_HURRY_PRODUCTION));
    // Riots allow hurrying facilities only.
    base->state_flags |= BSTATE_DRONE_RIOTS_ACTIVE;
    base->queue_items[0] = BSC_SCOUT_PATROL;
    base->minerals_accumulated = 0;
    Factions[1].energy_credits = 1000;
    CHECK_EQ(base_hurry_player(b, -1), HURRY_NOT_ALLOWED);
    base->state_flags &= ~BSTATE_DRONE_RIOTS_ACTIVE;

    // Maintenance: facility maint is paid each turn after turn 1.
    memset(base->facilities_built, 0, sizeof(base->facilities_built));
    set_fac(FAC_HEADQUARTERS, b, true);
    set_fac(FAC_PERIMETER_DEFENSE, b, true);
    set_fac(FAC_RECYCLING_TANKS, b, true);
    int maint = fac_maint(FAC_HEADQUARTERS, 1) + fac_maint(FAC_PERIMETER_DEFENSE, 1)
        + fac_maint(FAC_RECYCLING_TANKS, 1);
    Factions[1].energy_credits = 100;
    Factions[1].facility_maint_total = 0;
    set_base(b);
    base_compute(1);
    mod_base_maint();
    CHECK_EQ(Factions[1].energy_credits, 100 - maint);
    CHECK_EQ(Factions[1].facility_maint_total, maint);
    // Free faction facilities cost nothing.
    MFactions[1].faction_bonus_count = 1;
    MFactions[1].faction_bonus_id[0] = RULE_FACILITY;
    MFactions[1].faction_bonus_val1[0] = FAC_PERIMETER_DEFENSE;
    CHECK_EQ(fac_maint(FAC_PERIMETER_DEFENSE, 1), 0);
    MFactions[1].faction_bonus_count = 0;
    // Energy shortage sells a facility (difficulty above Specialist).
    int expensive = FAC_PERIMETER_DEFENSE;
    CHECK(Facility[expensive].maint > 0 || Facility[FAC_RECYCLING_TANKS].maint > 0);
    Factions[1].energy_credits = 0;
    Factions[1].diff_level = 3;
    headless_sink()->clear();
    mod_base_maint();
    CHECK(popups_with("POWERSHORT") >= 1);
}

static void test_capture() {
    auto s = new_state();
    setup_world(32, 20);
    int b = make_base(2, 10, 6, 4);
    make_base(2, 20, 6, 3);
    make_base(1, 4, 12, 3);
    BASE* base = &Bases[b];
    base->queue_items[0] = -FAC_RECYCLING_TANKS;
    base->minerals_accumulated = 15;
    set_fac(FAC_PRESSURE_DOME, b, true);
    Factions[2].energy_credits = 100;
    Factions[2].pop_total = 10;
    int pop = base->pop_size;
    conf.base_capture_fix = 0;
    mod_capture_base(b, 1, 0);
    CHECK_EQ(Bases[b].faction_id, 1);
    CHECK_EQ(Bases[b].faction_id_former, 2);
    CHECK_EQ(Bases[b].assimilation_turns_left, 50);
    CHECK_EQ(Bases[b].pop_size, pop - 1);
    // The queue is reset to Stockpile Energy and then base_first picks the
    // new owner's build (select_build is WP6); only the reset is checked.
    CHECK_EQ(Bases[b].queue_size, 0);
    CHECK_EQ(Bases[b].minerals_accumulated, 0);
    CHECK(has_fac_built(FAC_PRESSURE_DOME, b)); // never removed by capture
    CHECK_EQ(Factions[1].base_count, 2);
    CHECK_EQ(Factions[2].base_count, 1);
    int stolen = 100 * 4 / 11; // steal_energy: credits * pop / (pop_total + 1)
    CHECK_EQ(Factions[1].energy_credits, stolen);
    CHECK_EQ(Factions[2].energy_credits, 100 - stolen);
    CHECK_EQ(Monuments[1].events[MON_CONQUER_BASE].achieved, 1);
    CHECK(popups_with("SEIZEBASE") >= 1);
    // Recapture by the former owner liberates without assimilation.
    mod_capture_base(b, 2, 0);
    CHECK_EQ(Bases[b].faction_id, 2);
    CHECK_EQ(Bases[b].assimilation_turns_left, 0);
    CHECK(popups_with("LIBERATEBASE") >= 1);
    // Capturing a size 1 base destroys it.
    Bases[b].pop_size = 1;
    Bases[b].visibility |= FactionStatus[0]; // seen by a human player
    int count = *BaseCount;
    mod_capture_base(b, 1, 0);
    CHECK_EQ(*BaseCount, count - 1);
}

static void test_support_and_yield() {
    auto s = new_state();
    setup_world();
    int b = make_base(1, 6, 6, 3);
    BASE* base = &Bases[b];
    // Workers: pop + 1 = worked tiles (incl. base tile) + specialists.
    set_base(b);
    base_compute(1);
    CHECK_EQ(__builtin_popcount(base->worked_tiles) + base->specialist_total, base->pop_size + 1);
    CHECK(base->worked_tiles & 1);
    // Base tile yields from ResInfo.
    CHECK_EQ(mod_crop_yield(1, b, base->x, base->y, 0), ResInfo->base_sq.nutrient);
    CHECK_EQ(mod_mine_yield(1, b, base->x, base->y, 0), ResInfo->base_sq.mineral);
    set_fac(FAC_RECYCLING_TANKS, b, true);
    CHECK_EQ(mod_crop_yield(1, b, base->x, base->y, 0),
        ResInfo->base_sq.nutrient + ResInfo->recycling_tanks.nutrient);
    // Farm on rainy land: 2 + improved_land, limited to 2 without tech.
    int tx = base->x + 2, ty = base->y;
    MAP* sq = mapsq(tx, ty);
    sq->climate = (uint8_t)(((ALT_SHORE_LINE + 1) << 5) | (2 << 3));
    sq->items |= BIT_FARM;
    CHECK(sq->is_rainy());
    CHECK_EQ(mod_crop_yield(1, b, tx, ty, 0), 2);
    // Units over the free support cost minerals.
    Factions[1].SE_support_pending = 0; // 2 free
    for (int i = 0; i < 4; i++) {
        int v = veh_init(BSC_SCOUT_PATROL, 1, base->x, base->y);
        Vehs[v].home_base_id = b;
    }
    set_base(b);
    base_compute(1);
    CHECK_EQ(*BaseForcesSupported, 4);
    CHECK_EQ(*BaseForcesMaintCost, 2);
    CHECK_EQ(base->mineral_consumption, 2);
    // Specialists: pop below min_base_size_specialists picks the psych specialist.
    CHECK_EQ(pick_specialist(base, 4, 2, 2), base->pop_size >= 5 ? 0 : 2);
}

static void test_monuments_and_names() {
    auto s = new_state();
    setup_world();
    clear_monuments();
    char name[] = "Alpha";
    mon_colony_founded(1, name);
    CHECK_EQ(Monuments[1].events[MON_COLONY_FOUNDED].achieved, 1);
    CHECK_EQ(Monuments[1].events[MON_COLONY_FOUNDED].faction_id, 1);
    CHECK_EQ(Monuments[1].events[MON_COLONY_FOUNDED].prev_count, 1);
    CHECK(!strcmp(Monuments[1].base_name, "Alpha"));
    CHECK_EQ(Monuments[1].latest_event_id, MON_COLONY_FOUNDED);
    CHECK(log_with("ui:monument") >= 1);
    mon_colony_founded(2, name);
    CHECK_EQ(Monuments[2].events[MON_COLONY_FOUNDED].faction_id, 1); // first faction wins
    CHECK_EQ(Monuments[2].events[MON_COLONY_FOUNDED].prev_count, 0);
    mon_killed_faction(1, 3);
    mon_killed_faction(1, 2);
    CHECK_EQ(Monuments[1].events[MON_KILLED_FACTION_1].param2, 3);
    CHECK_EQ(Monuments[1].events[MON_KILLED_FACTION_1 + 1].param2, 2);
    // Base names: faction lists, scrambled order, fallback "<noun> <n>".
    char buf[32];
    Factions[1].base_name_offset = 0;
    mod_name_base(1, buf, true, false);
    CHECK(!strcmp(buf, "Noun1 1"));
    base_name_lists.land[1] = {"Landing", "Hill", "Lake", "Ridge"};
    base_name_lists.sea[1] = {"Reef"};
    Factions[1].base_name_offset = 0;
    mod_name_base(1, buf, true, false);
    CHECK(!strcmp(buf, "Landing")); // first name is always the first entry
    CHECK_EQ(Factions[1].base_name_offset, 1);
    std::set<std::string> seen = {buf};
    for (int i = 0; i < 3; i++) {
        mod_name_base(1, buf, true, false);
        CHECK(seen.insert(buf).second);
    }
    CHECK_EQ(seen.size(), 4u);
    base_name_lists.generic = {"Generic A"};
    mod_name_base(1, buf, true, false);
    CHECK(!strcmp(buf, "Generic A"));
    mod_name_base(1, buf, true, false);
    CHECK(!strcmp(buf, "Noun1 6"));
    Factions[1].base_sea_name_offset = 0;
    mod_name_base(1, buf, true, true);
    CHECK(!strcmp(buf, "Reef"));
    mod_name_base(1, buf, true, true); // sea names exhausted -> land rules
    CHECK(!strcmp(buf, "Noun1 7"));
    base_name_lists = BaseNameLists();
    // make_base_unique appends -2, -3.
    int b0 = make_base(1, 4, 4, 1);
    int b1 = make_base(1, 12, 4, 1);
    int b2 = make_base(1, 18, 4, 1);
    strcpy_n(Bases[b0].name, 25, "Same");
    strcpy_n(Bases[b1].name, 25, "Same");
    strcpy_n(Bases[b2].name, 25, "Same");
    make_base_unique(b1);
    CHECK(!strcmp(Bases[b1].name, "Same-2"));
    make_base_unique(b2);
    CHECK(!strcmp(Bases[b2].name, "Same-3"));
}

static void test_full_turns() {
    // Several upkeep turns on a few bases: invariants hold and nothing crashes.
    auto s = new_state();
    setup_world(40, 24);
    lcg_state = 99;
    randomize_tiles();
    for (int i = 0; i < 8; i++) {
        int x = 4 + (i % 4) * 8, y = 4 + (i / 4) * 10;
        int b = make_base(1 + (i & 1), x + (y & 1), y, 1 + i % 4);
        if (b >= 0) {
            Bases[b].queue_items[0] = (i % 3) ? BSC_SCOUT_PATROL : -FAC_RECYCLING_TANKS;
            Bases[b].production_id_last = Bases[b].queue_items[0];
            Bases[b].governor_flags = GOV_ACTIVE | GOV_MANAGE_CITIZENS;
        }
    }
    int start_vehicles = *VehCount;
    for (int turn = 0; turn < 30; turn++) {
        *CurrentTurn = 10 + turn;
        for (int i = 0; i < *BaseCount; i++) {
            if (mod_base_upkeep(i)) {
                i--;
                continue;
            }
            BASE* b = &Bases[i];
            CHECK(b->pop_size >= 1);
            CHECK_EQ(__builtin_popcount(b->worked_tiles) + b->specialist_total, b->pop_size + 1);
            CHECK(b->talent_total + b->drone_total + b->specialist_total <= b->pop_size);
            CHECK(b->queue_size >= 0 && b->queue_size <= 9);
        }
    }
    CHECK(*VehCount > start_vehicles);
    CHECK(*BaseCount >= 1);
}

int main(int argc, char** argv) {
    if (argc < 2 || strcmp(argv[1], "unit")) {
        fprintf(stderr, "usage: %s unit\n", argv[0]);
        return 2;
    }
    test_cost_factor_oracle();
    test_psych_oracle();
    test_accounting_oracles();
    test_commerce_oracle();
    test_unit_birth();
    test_prototype_cost();
    test_facility_queue_and_stockpile();
    test_retool_and_change();
    test_secret_project();
    test_growth_and_starvation();
    test_riots();
    test_hurry_and_maint();
    test_capture();
    test_support_and_yield();
    test_monuments_and_names();
    test_full_turns();
    if (failures) {
        fprintf(stderr, "%d of %d wp2 checks failed\n", failures, checks);
        return 1;
    }
    printf("%d wp2 checks passed (%d oracle cases)\n", checks, oracle_cases);
    return 0;
}
