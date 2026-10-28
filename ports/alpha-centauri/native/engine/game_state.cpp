/* SPDX-License-Identifier: GPL-3.0-or-later
 * EngineState storage, global binding and map allocation.
 * map_init()/map_shutdown() follow Thinker Mod src/savegame.cpp and
 * quick_root() follows OpenSMACX general.cpp (GPL-3.0); Thinker pin
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT, see engine/THINKER-LICENSE.
 */
#include "engine.h"

namespace alpha {
namespace engine {

Config conf;
AIPlans plans[MaxPlayerNum];
FILE* debug_log = nullptr;

static EngineState* bound = nullptr;
static NetMessage net_message_dummy = {0};
static Sprite* faction_portraits[MaxPlayerNum] = {};
static MapWindow* map_win_ptrs[8] = {};
static MAP* map_tiles_ptr = nullptr;
static uint8_t* map_abstract_ptr = nullptr;

NetMessage* NetMsg = &net_message_dummy;
Sprite** FactionPortraits = faction_portraits;

GamePrefs* cs = nullptr;
int* GamePreferences = nullptr;
int* GameMorePreferences = nullptr;
int* GameWarnings = nullptr;
int* GameRules = nullptr;
int* GameInterludeState = nullptr;
int* TutWinMapState = nullptr;
int* GameState = nullptr;
int* DiffLevel = nullptr;
int* VehCount = nullptr;
int* BaseCount = nullptr;
int* CurrentTurn = nullptr;
int* CurrentMissionYear = nullptr;
int* CurrentVehID = nullptr;
int* GameTimeControl = nullptr;
uint8_t* FactionStatus = nullptr;
int* FactionRankings = nullptr;
int* RankingFactionIDUnk1 = nullptr;
int* RankingFactionIDUnk2 = nullptr;
int* SecretProjects = nullptr;
int* GovernorFaction = nullptr;
int* ProposalPassCount = nullptr;
int* ProposalCallTurn = nullptr;
uint8_t* TechOwners = nullptr;
char* LastSavePath = nullptr;
int* ClimateValueA = nullptr;
int* ClimateValueB = nullptr;
int* ClimateValueC = nullptr;
int* ClimateFutureChange = nullptr;
int* ScoreBonusPts = nullptr;
int* ScorePopTotal = nullptr;
int* SunspotDuration = nullptr;
int* MountPlanetX = nullptr;
int* MountPlanetY = nullptr;
int* DustCloudDuration = nullptr;
int* SaveFileVersion = nullptr;
int* ScnVictFacilityObj = nullptr;
int* SolarFlaresEvent = nullptr;
int* GameMoreRules = nullptr;
int* CurrentFaction = nullptr;
int* ClimateLevel = nullptr;

int* MapAreaX = nullptr;
int* MapAreaY = nullptr;
int* MapRandomSeed = nullptr;
int* MapSeaLevel = nullptr;
int* MapSeaLevelCouncil = nullptr;
int* MapAreaTiles = nullptr;
int* MapAreaSqRoot = nullptr;
int* MapToggleFlat = nullptr;
int* MapLandmarkCount = nullptr;
Landmark* Landmarks = nullptr;
int* MapAbstractX = nullptr;
int* MapAbstractY = nullptr;
int* MapAbstractCount = nullptr;
int* MapSizePlanet = nullptr;
int* MapOceanCoverage = nullptr;
int* MapLandCoverage = nullptr;
int* MapErosiveForces = nullptr;
int* MapPlanetaryOrbit = nullptr;
int* MapCloudCover = nullptr;
int* MapNativeLifeForms = nullptr;
char* MapFilePath = nullptr;
int* MapHalfX = nullptr;
MAP** MapTiles = &map_tiles_ptr;
uint8_t** MapAbstract = &map_abstract_ptr;

MFaction* MFactions = nullptr;
Faction* Factions = nullptr;
int* FactionRankingsUnk = nullptr;
int16_t (*FactionTurnMight)[8] = nullptr;
int* ReplayEventSize = nullptr;
ReplayEvent* ReplayEvents = nullptr;
Continent* Continents = nullptr;
UNIT* Units = nullptr;
VEH* Vehs = nullptr;
BASE* Bases = nullptr;
Monument* Monuments = nullptr;
int* TectonicDetonationCount = nullptr;
int* ObjectiveReqVictory = nullptr;
int* ObjectivesSuddenDeathVictory = nullptr;
int* ObjectiveAchievePts = nullptr;
int* VictoryAchieveBonusPts = nullptr;
int* StartingMissionYear = nullptr;
int* EndingMissionYear = nullptr;
Console* MapWin = nullptr;
MapWindow** MapWinPtr = map_win_ptrs;
WorldWindow* WorldWin = nullptr;
StatusWindow* StatusWin = nullptr;
int* CanvasWidth = nullptr;
int* CanvasHeight = nullptr;

CRules* Rules = nullptr;
CResourceInfo* ResInfo = nullptr;
CCitizen* Citizen = nullptr;
CResourceName* ResName = nullptr;
CCombatMode* OffenseMode = nullptr;
CCombatMode* DefenseMode = nullptr;
CBonusName* BonusName = nullptr;
CSocialParam* SocialParam = nullptr;
CEnergy* Energy = nullptr;
CChassis* Chassis = nullptr;
CNatural* Natural = nullptr;
CWeapon* Weapon = nullptr;
CSocialField* SocialField = nullptr;
CMandate* Mandate = nullptr;
CMight* Might = nullptr;
CTimeControl* TimeControl = nullptr;
CArmor* Armor = nullptr;
CTech* Tech = nullptr;
CWorldbuilder* WorldBuilder = nullptr;
CMorale* Morale = nullptr;
CReactor* Reactor = nullptr;
COrder* Order = nullptr;
CFacility* Facility = nullptr;
CProposal* Proposal = nullptr;
CAbility* Ability = nullptr;
CTerraform* Terraform = nullptr;
DefaultPref* DefaultPrefs = nullptr;
AlphaIniPref* AlphaIniPrefs = nullptr;

int* ExpansionEnabled = nullptr;
int* MultiplayerActive = nullptr;
int* PbemActive = nullptr;
int* CurrentPlayerFaction = nullptr;
int* ControlTurnMove = nullptr;
int* ComputeBaseID = nullptr;
int* CurrentBaseID = nullptr;
BASE** CurrentBase = nullptr;
int* GameDrawState = nullptr;
int* VehBitError = nullptr;
int* VehDropLiftVehID = nullptr;
int* VehLiftX = nullptr;
int* VehLiftY = nullptr;
int* BaseFindDist = nullptr;
int* VehFindDist = nullptr;
int* VehBattleState = nullptr;
int* SaveFileWrite = nullptr;
int* SaveFileEncrypt = nullptr;
int* SaveFileMenu = nullptr;
int* SaveFileStatus = nullptr;
int* GameLanguage = nullptr;
int* TechValidCount = nullptr;
int* TechCommerceCount = nullptr;
int* AltNatural = nullptr;
int* ParseNumTable = nullptr;
char256* ParseStrBuffer = nullptr;
int* ParseStrGender = nullptr;
int* ParseStrPlurality = nullptr;
int* GenderDefault = nullptr;
int* PluralDefault = nullptr;

static_assert(sizeof(int) == 4, "engine assumes 32-bit int like the original");

template <class T>
static void zero(T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "plain record expected");
    memset((void*)&value, 0, sizeof(T));
}

static void put_le32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void engine_reset(EngineState& s) {
    zero(s.legacy_prefs);
    put_le32(s.legacy_prefs + 0x304, 12); // Thinker writes a zeroed legacy header marked v12
    zero(s.prefs);
    s.prefs.SaveFileVersion = 12;
    for (int& sp : s.prefs.SecretProjects) {
        sp = SP_Unbuilt;
    }
    zero(s.factions);
    zero(s.mfactions);
    zero(s.faction_rankings_unk);
    zero(s.map);
    s.map_half_x = 0;
    s.map_tiles.clear();
    s.map_tiles.shrink_to_fit();
    s.map_abstract.clear();
    s.map_abstract.shrink_to_fit();
    zero(s.faction_turn_might);
    s.replay_event_size = 0;
    zero(s.replay_events);
    zero(s.continents);
    zero(s.units);
    zero(s.bunglist);
    zero(s.vehs);
    zero(s.bases);
    zero(s.scenario_rules);
    zero(s.scenario_name);
    zero(s.objectives);
    zero(s.monuments);
    zero(s.tectonic_detonation_count);
    zero(s.map_win);
    s.map_win.VehID = -1;
    s.map_win.ReadyVehID = -1;
    zero(s.map_wins);
    zero(s.world_win);
    s.status_win.pending_veh_id = -1;
    s.status_win.active_veh_id = -1;
    s.canvas_width = 0;
    s.canvas_height = 0;
    s.save_tick_seed = 0;

    zero(s.rules);
    zero(s.res_info);
    zero(s.citizen);
    zero(s.res_name);
    zero(s.offense_mode);
    zero(s.defense_mode);
    zero(s.bonus_name);
    zero(s.social_param);
    zero(s.energy);
    zero(s.chassis);
    zero(s.natural);
    zero(s.weapon);
    zero(s.social_field);
    zero(s.mandate);
    zero(s.might);
    zero(s.time_control);
    zero(s.armor);
    zero(s.tech);
    zero(s.world_builder);
    zero(s.morale);
    zero(s.reactor);
    zero(s.order);
    zero(s.facility);
    zero(s.proposal);
    zero(s.ability);
    zero(s.terraform);
    zero(s.default_prefs);
    zero(s.alpha_ini_prefs);

    s.expansion_enabled = 1;
    s.multiplayer_active = 0;
    s.pbem_active = 0;
    s.current_player_faction = 0;
    s.control_turn_move = 0;
    s.compute_base_id = -1;
    s.current_base_id = -1;
    s.current_base = nullptr;
    s.game_draw_state = 0;
    s.veh_bit_error = 0;
    s.veh_drop_lift_veh_id = -1;
    s.veh_lift_x = 0;
    s.veh_lift_y = 0;
    s.base_find_dist = 0;
    s.veh_find_dist = 0;
    s.veh_battle_state = 0;
    s.save_file_write = 0;
    s.save_file_encrypt = 0;
    s.save_file_menu = 0;
    s.save_file_status = 0;
    s.game_language = 0;
    s.tech_valid_count = 0;
    s.tech_commerce_count = 0;
    zero(s.alt_natural);
    zero(s.parse_num_table);
    zero(s.parse_str_buffer);
    zero(s.parse_str_gender);
    zero(s.parse_str_plurality);
    s.gender_default = 0;
    s.plural_default = 0;
    s.rng = RandomState();
    if (bound == &s) {
        engine_bind(&s);
    }
}

EngineState* engine_state() {
    return bound;
}

EngineState* engine_bind(EngineState* s) {
    EngineState* previous = bound;
    bound = s;
    if (!s) {
        return previous;
    }
    GamePrefs& p = s->prefs;
    cs = &p;
    GamePreferences = &p.GamePreferences;
    GameMorePreferences = &p.GameMorePreferences;
    GameWarnings = &p.GameWarnings;
    GameRules = &p.GameRules;
    GameInterludeState = p.GameInterludeState;
    TutWinMapState = p.TutWinMapState;
    GameState = &p.GameState;
    DiffLevel = &p.DiffLevel;
    VehCount = &p.VehCount;
    BaseCount = &p.BaseCount;
    CurrentTurn = &p.CurrentTurn;
    CurrentMissionYear = &p.CurrentMissionYear;
    CurrentVehID = &p.CurrentVehID;
    GameTimeControl = p.GameTimeControl;
    FactionStatus = p.FactionStatus;
    FactionRankings = p.FactionRankings;
    RankingFactionIDUnk1 = &p.RankingFactionIDUnk1;
    RankingFactionIDUnk2 = &p.RankingFactionIDUnk2;
    SecretProjects = p.SecretProjects;
    GovernorFaction = &p.GovernorFaction;
    ProposalPassCount = p.ProposalPassCount;
    ProposalCallTurn = p.ProposalCallTurn;
    TechOwners = p.TechOwners;
    LastSavePath = p.LastSavePath;
    ClimateValueA = &p.ClimateValueA;
    ClimateValueB = &p.ClimateValueB;
    ClimateValueC = &p.ClimateValueC;
    ClimateFutureChange = &p.ClimateFutureChange;
    ScoreBonusPts = &p.ScoreBonusPts;
    ScorePopTotal = p.ScorePopTotal;
    SunspotDuration = &p.SunspotDuration;
    MountPlanetX = &p.MountPlanetX;
    MountPlanetY = &p.MountPlanetY;
    DustCloudDuration = &p.DustCloudDuration;
    SaveFileVersion = &p.SaveFileVersion;
    ScnVictFacilityObj = &p.ScnVictFacilityObj;
    SolarFlaresEvent = &p.SolarFlaresEvent;
    GameMoreRules = &p.GameMoreRules;
    CurrentFaction = &p.CurrentFaction;
    ClimateLevel = &p.ClimateLevel;

    MapMeta& m = s->map;
    MapAreaX = &m.MapAreaX;
    MapAreaY = &m.MapAreaY;
    MapRandomSeed = &m.MapRandomSeed;
    MapSeaLevel = &m.MapSeaLevel;
    MapSeaLevelCouncil = &m.MapSeaLevelCouncil;
    MapAreaTiles = &m.MapAreaTiles;
    MapAreaSqRoot = &m.MapAreaSqRoot;
    MapToggleFlat = &m.MapToggleFlat;
    MapLandmarkCount = &m.MapLandmarkCount;
    Landmarks = m.Landmarks;
    MapAbstractX = &m.MapAbstractX;
    MapAbstractY = &m.MapAbstractY;
    MapAbstractCount = &m.MapAbstractCount;
    MapSizePlanet = &m.MapSizePlanet;
    MapOceanCoverage = &m.MapOceanCoverage;
    MapLandCoverage = &m.MapLandCoverage;
    MapErosiveForces = &m.MapErosiveForces;
    MapPlanetaryOrbit = &m.MapPlanetaryOrbit;
    MapCloudCover = &m.MapCloudCover;
    MapNativeLifeForms = &m.MapNativeLifeForms;
    MapFilePath = m.MapFilePath;
    MapHalfX = &s->map_half_x;
    map_tiles_ptr = s->map_tiles.empty() ? nullptr : s->map_tiles.data();
    map_abstract_ptr = s->map_abstract.empty() ? nullptr : s->map_abstract.data();

    MFactions = s->mfactions;
    Factions = s->factions;
    FactionRankingsUnk = s->faction_rankings_unk;
    FactionTurnMight = s->faction_turn_might;
    ReplayEventSize = &s->replay_event_size;
    ReplayEvents = s->replay_events;
    Continents = s->continents;
    Units = s->units;
    Vehs = &s->vehs[1];
    Bases = s->bases;
    Monuments = s->monuments;
    TectonicDetonationCount = s->tectonic_detonation_count;
    ObjectiveReqVictory = &s->objectives[0];
    ObjectivesSuddenDeathVictory = &s->objectives[1];
    ObjectiveAchievePts = &s->objectives[2];
    VictoryAchieveBonusPts = &s->objectives[3];
    StartingMissionYear = &s->objectives[4];
    EndingMissionYear = &s->objectives[5];
    MapWin = &s->map_win;
    for (int i = 0; i < 8; i++) {
        map_win_ptrs[i] = &s->map_wins[i];
    }
    WorldWin = &s->world_win;
    StatusWin = &s->status_win;
    CanvasWidth = &s->canvas_width;
    CanvasHeight = &s->canvas_height;

    Rules = &s->rules;
    ResInfo = &s->res_info;
    Citizen = s->citizen;
    ResName = s->res_name;
    OffenseMode = s->offense_mode;
    DefenseMode = s->defense_mode;
    BonusName = s->bonus_name;
    SocialParam = s->social_param;
    Energy = s->energy;
    Chassis = s->chassis;
    Natural = s->natural;
    Weapon = s->weapon;
    SocialField = s->social_field;
    Mandate = s->mandate;
    Might = s->might;
    TimeControl = s->time_control;
    Armor = s->armor;
    Tech = s->tech;
    WorldBuilder = &s->world_builder;
    Morale = s->morale;
    Reactor = s->reactor;
    Order = s->order;
    Facility = s->facility;
    Proposal = s->proposal;
    Ability = s->ability;
    Terraform = s->terraform;
    DefaultPrefs = &s->default_prefs;
    AlphaIniPrefs = &s->alpha_ini_prefs;

    ExpansionEnabled = &s->expansion_enabled;
    MultiplayerActive = &s->multiplayer_active;
    PbemActive = &s->pbem_active;
    CurrentPlayerFaction = &s->current_player_faction;
    ControlTurnMove = &s->control_turn_move;
    ComputeBaseID = &s->compute_base_id;
    CurrentBaseID = &s->current_base_id;
    CurrentBase = &s->current_base;
    GameDrawState = &s->game_draw_state;
    VehBitError = &s->veh_bit_error;
    VehDropLiftVehID = &s->veh_drop_lift_veh_id;
    VehLiftX = &s->veh_lift_x;
    VehLiftY = &s->veh_lift_y;
    BaseFindDist = &s->base_find_dist;
    VehFindDist = &s->veh_find_dist;
    VehBattleState = &s->veh_battle_state;
    SaveFileWrite = &s->save_file_write;
    SaveFileEncrypt = &s->save_file_encrypt;
    SaveFileMenu = &s->save_file_menu;
    SaveFileStatus = &s->save_file_status;
    GameLanguage = &s->game_language;
    TechValidCount = &s->tech_valid_count;
    TechCommerceCount = &s->tech_commerce_count;
    AltNatural = s->alt_natural;
    ParseNumTable = s->parse_num_table;
    ParseStrBuffer = s->parse_str_buffer;
    ParseStrGender = s->parse_str_gender;
    ParseStrPlurality = s->parse_str_plurality;
    GenderDefault = &s->gender_default;
    PluralDefault = &s->plural_default;
    random_bind(&s->rng);
    return previous;
}

/* OpenSMACX general.cpp quick_root (GPL-3.0), original offset 0x6290E0. */
static int quick_root(int input) {
    int temp = input >> 1;
    int sq_root;
    if (input <= 1) {
        return input;
    }
    do {
        sq_root = temp;
        temp = (temp + input / temp) >> 1;
    } while (temp < sq_root);
    return sq_root;
}

void map_shutdown() {
    EngineState* s = bound;
    if (s) {
        s->map_tiles.clear();
        s->map_tiles.shrink_to_fit();
        s->map_abstract.clear();
        s->map_abstract.shrink_to_fit();
    }
    *MapTiles = nullptr;
    *MapAbstract = nullptr;
}

int map_init() {
    EngineState* s = bound;
    if (!s || *MapAreaX <= 0 || *MapAreaX > 0x1000 || *MapAreaY <= 0 || *MapAreaY > 0x1000) {
        return 1;
    }
    *MapHalfX = *MapAreaX / 2;
    *MapAreaTiles = *MapAreaY * (*MapAreaX / 2);
    *MapAreaSqRoot = quick_root(*MapAreaY * (*MapAreaX / 2));
    *MapAbstractX = (*MapAreaX + 4) / 5;
    *MapAbstractY = (*MapAreaY + 4) / 5;
    *MapAbstractCount = *MapAbstractY * ((*MapAbstractX + 1) / 2);
    s->map_tiles.assign((size_t)*MapAreaTiles, MAP());
    s->map_abstract.assign((size_t)*MapAbstractCount, 0);
    *MapTiles = s->map_tiles.empty() ? nullptr : s->map_tiles.data();
    *MapAbstract = s->map_abstract.empty() ? nullptr : s->map_abstract.data();
    return 0;
}

static uint64_t get_le64(const char* p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) {
        v = (v << 8) | (uint8_t)p[i];
    }
    return v;
}

static void set_le64(char* p, uint64_t v) {
    for (int i = 0; i < 8; i++) {
        p[i] = (char)(uint8_t)(v >> (8 * i));
    }
}

ThinkerData thinker_vars_get() {
    ThinkerData d;
    const char* p = MFactions[0].pad_2;
    d.reserved = get_le64(p);
    d.game_time_spent = get_le64(p + 8);
    memcpy(d.build_date, p + 16, 12);
    d.map_random_value = (uint32_t)(uint8_t)p[28] | ((uint32_t)(uint8_t)p[29] << 8)
        | ((uint32_t)(uint8_t)p[30] << 16) | ((uint32_t)(uint8_t)p[31] << 24);
    memcpy(d.padding, p + 32, sizeof(d.padding));
    return d;
}

void thinker_vars_set(const ThinkerData& d) {
    char* p = MFactions[0].pad_2;
    set_le64(p, d.reserved);
    set_le64(p + 8, d.game_time_spent);
    memcpy(p + 16, d.build_date, 12);
    for (int i = 0; i < 4; i++) {
        p[28 + i] = (char)(uint8_t)(d.map_random_value >> (8 * i));
    }
    memcpy(p + 32, d.padding, sizeof(d.padding));
}

static void vlog(const char* fmt, va_list ap) {
    if (debug_log) {
        vfprintf(debug_log, fmt, ap);
    }
}

void debug(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}

void debugw(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
    flushlog();
}

void debug_ver(const char* fmt, ...) {
    if (!conf.debug_verbose) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}

void flushlog() {
    if (debug_log) {
        fflush(debug_log);
    }
}

void config_vanilla(Config& c) {
    c = Config();
    // Values that Thinker docs/thinker.ini lists as the original game default.
    c.limit_project_start = 3;
    c.simple_cost_factor = 0;
    c.revised_tech_cost = 0;
    c.tech_stagnate_rate = 150;
    c.delay_drone_riots = 0;
    c.nerve_staple_mod = 0;
    c.fast_fungus_movement = 0;
    c.magtube_movement_rate = 0;
    c.chopper_attack_rate = 1;
    c.activate_skipped_units = 0;
    c.counter_espionage = 0;
    c.ignore_reactor_power = 0;
    c.long_range_artillery = 0;
    c.modify_upgrade_cost = 0;
    c.modify_unit_support = 0;
    for (int& v : c.unit_support_bonus) {
        v = 0;
    }
    c.modify_unit_limit = 0;
    c.max_veh_num = MaxVehNum;
    c.territory_border_fix = 0;
    c.auto_relocate_hq = 0;
    c.rebuild_secret_projects = 0;
    c.steal_energy_rate = 100;
    c.simple_hurry_cost = 0;
    c.eco_damage_fix = 0;
    c.planetpearls = 2;
    c.tile_output_limit[0] = c.tile_output_limit[1] = c.tile_output_limit[2] = 2;
    c.soil_improve_value = 0;
    const int lifecycle[6] = {45, 90, 170, 250, 0, 330};
    memcpy(c.native_lifecycle_levels, lifecycle, sizeof(lifecycle));
    const int talent[4] = {0, 2, 1, 2};
    memcpy(c.facility_talent_value, talent, sizeof(talent));
    for (int& v : c.facility_defense_value) {
        v = 100;
    }
    c.planet_defense_bonus = 0;
    c.sensor_defense_ocean = 0;
    c.repair_minimal = 1;
    c.repair_fungus = 2;
    c.repair_friendly = 1;
    c.repair_airbase = 1;
    c.repair_bunker = 1;
    c.repair_base = 1;
    c.repair_base_native = 10;
    c.repair_base_facility = 10;
    c.repair_nano_factory = 10;
    c.repair_battle_ogre = 0;
    c.new_world_builder = 0;
    c.modified_landmarks = 0;
    c.time_warp_mod = 0;
    c.rare_supply_pods = 0;
    c.nutrient_bonus = 0;
    c.player_colony_pods = 0;
    c.computer_colony_pods = 0;
    c.new_base_names = 0;
    c.new_unit_names = 0;
}

} // namespace engine
} // namespace alpha
