/* SPDX-License-Identifier: GPL-3.0-or-later
 * EngineState: one object holding every global of the original game that the
 * engine uses, with full original capacity (2048+1 vehicles, 512 bases,
 * 512 prototypes, 8 factions, dynamic map). Thinker's global names
 * (src/engine.h/engine.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT;
 * see engine/THINKER-LICENSE) are kept as pointers into the bound state.
 *
 * The struct is called EngineState (not GameState) because Thinker's
 * `int* GameState` flag word keeps its original name for ported code.
 *
 * Exactly one EngineState is bound at a time (engine_bind). Several states may
 * exist (for example a scratch copy for AI look-ahead or a save comparison);
 * rebinding re-points every global. Not thread-safe, like the original.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

/* The 0xAA4-byte map metadata block that map_write() copies from 0x949870
 * (MapAreaX .. MapAbstract pointer). Field order is the original memory order.
 * MapTilesPtr/MapAbstractPtr preserve the inert pointer bytes of a loaded file;
 * they are never dereferenced. MapFilePath is the 80-byte slot before them. */
struct MapMeta {
    int32_t MapAreaX;
    int32_t MapAreaY;
    int32_t MapRandomSeed;
    int32_t MapSeaLevel;
    int32_t MapSeaLevelCouncil;
    int32_t MapAreaTiles;
    int32_t MapAreaSqRoot;
    int32_t MapToggleFlat;
    int32_t MapLandmarkCount;
    Landmark Landmarks[64];
    int32_t MapAbstractX;
    int32_t MapAbstractY;
    int32_t MapAbstractCount;
    int32_t MapSizePlanet;
    int32_t MapOceanCoverage;
    int32_t MapLandCoverage;
    int32_t MapErosiveForces;
    int32_t MapPlanetaryOrbit;
    int32_t MapCloudCover;
    int32_t MapNativeLifeForms;
    char MapFilePath[80];
    uint32_t MapTilesPtr;
    uint32_t MapAbstractPtr;
};

const int MaxReplayEventNum = 1024;
const int MaxTurnMightNum = 1000;
const int MaxObjectiveNum = 38; // 0x98 bytes from ObjectiveReqVictory
const int LegacyPrefsSize = 0x31C;
const int BunglistSize = 0x1000;

/* RNG state, explicit and seedable (random.cpp). */
struct RandomState {
    uint32_t game_rand_state = 1;    // MSVC rand() state used by game_rand()/game_srand()
    uint32_t random_seed = 0;        // Random::get / game_random LCG used by random()
    uint32_t map_rand_state = 0;     // GameRandom map_rand
    uint32_t next_rand_x = 0;        // next_rand() xorshift state (Thinker used QPC)
    uint32_t next_rand_y = 0;
    uint32_t next_rand_tick = 0;     // replaces GetTickCount() in next_rand()
};

struct EngineState {
    /* ---- Saved game data (game_data() order) ---- */
    uint8_t legacy_prefs[LegacyPrefsSize]; // first header block, preserved for v>=11 files
    GamePrefs prefs;                       // GamePreferences .. ClimateLevel
    Faction factions[MaxPlayerNum];
    MFaction mfactions[MaxPlayerNum];
    int32_t faction_rankings_unk[MaxPlayerNum];
    MapMeta map;
    int32_t map_half_x;                    // MapHalfX (derived, not saved)
    std::vector<MAP> map_tiles;
    std::vector<uint8_t> map_abstract;
    int16_t faction_turn_might[MaxTurnMightNum][8];
    int32_t replay_event_size;
    ReplayEvent replay_events[MaxReplayEventNum];
    Continent continents[MaxRegionNum];
    UNIT units[MaxProtoNum];
    uint8_t bunglist[BunglistSize];        // opaque: element type unknown, kept as wire bytes
    VEH vehs[MaxVehNum + 2];               // Vehs = &vehs[1]; Vehs[MaxVehNum] is the veh_fake slot
    BASE bases[MaxBaseNum];
    uint8_t scenario_rules[0x100];         // unk_9B2178, present when STATE_IS_SCENARIO; opaque
    char scenario_name[0x100];             // unk_9B2078
    int32_t objectives[MaxObjectiveNum];   // ObjectiveReqVictory .. (0x98 bytes)
    Monument monuments[MaxPlayerNum];
    int32_t tectonic_detonation_count[MaxPlayerNum];
    Console map_win;                       // MapWin
    MapWindow map_wins[8];                 // MapWinPtr[0..7]
    WorldWindow world_win;                 // WorldWin
    StatusWindow status_win;               // StatusWin
    int32_t canvas_width;
    int32_t canvas_height;
    uint32_t save_tick_seed;               // seed appended by save_daemon (GetTickCount in original)

    /* ---- Rules (alpha.txt / alphax.txt); not saved ---- */
    CRules rules;
    CResourceInfo res_info;
    CCitizen citizen[MaxCitizenNum];
    CResourceName res_name[MaxResourceNum];
    CCombatMode offense_mode[MaxOffenseModeNum];
    CCombatMode defense_mode[MaxDefenseModeNum];
    CBonusName bonus_name[MaxBonusNameNum];
    CSocialParam social_param[11];
    CEnergy energy[MaxEnergyNum];
    CChassis chassis[MaxChassisNum];
    CNatural natural[MaxNaturalNum];
    CWeapon weapon[MaxWeaponNum];
    CSocialField social_field[MaxSocialCatNum];
    CMandate mandate[MaxMandateNum];
    CMight might[MaxMightNum];
    CTimeControl time_control[MaxTimeControlNum];
    CArmor armor[MaxArmorNum];
    CTech tech[MaxTechnologyNum];
    CWorldbuilder world_builder;
    CMorale morale[MaxMoraleNum];
    CReactor reactor[MaxReactorNum];
    COrder order[MaxOrderNum];
    CFacility facility[SP_ID_Last + 1];
    CProposal proposal[MaxProposalNum];
    CAbility ability[MaxAbilityNum];
    CTerraform terraform[MaxTerrainNum];
    DefaultPref default_prefs;
    AlphaIniPref alpha_ini_prefs;

    /* ---- Runtime globals of the original executable; not saved ---- */
    int32_t expansion_enabled;     // ExpansionEnabled (Alien Crossfire rules)
    int32_t multiplayer_active;    // MultiplayerActive
    int32_t pbem_active;           // PbemActive
    int32_t current_player_faction;// CurrentPlayerFaction
    int32_t control_turn_move;     // ControlTurnMove
    int32_t compute_base_id;       // ComputeBaseID
    int32_t current_base_id;       // CurrentBaseID
    BASE* current_base;            // CurrentBase
    int32_t game_draw_state;       // GameDrawState
    int32_t veh_bit_error;         // VehBitError
    int32_t veh_drop_lift_veh_id;  // VehDropLiftVehID
    int32_t veh_lift_x;            // VehLiftX
    int32_t veh_lift_y;            // VehLiftY
    int32_t base_find_dist;        // BaseFindDist
    int32_t veh_find_dist;         // VehFindDist
    int32_t veh_battle_state;      // VehBattleState
    int32_t save_file_write;       // SaveFileWrite
    int32_t save_file_encrypt;     // SaveFileEncrypt
    int32_t save_file_menu;        // SaveFileMenu
    int32_t save_file_status;      // SaveFileStatus
    int32_t game_language;         // GameLanguage
    int32_t tech_valid_count;      // TechValidCount
    int32_t tech_commerce_count;   // TechCommerceCount
    int32_t alt_natural[11];       // AltNatural (load_daemon copies ElevDetail into it)
    int32_t parse_num_table[10];   // ParseNumTable
    char256 parse_str_buffer[10];  // ParseStrBuffer
    int32_t parse_str_gender[10];  // ParseStrGender
    int32_t parse_str_plurality[10];// ParseStrPlurality
    int32_t gender_default;        // GenderDefault
    int32_t plural_default;        // PluralDefault
    RandomState rng;
};

/* Allocate/zero a state with the defaults of a freshly started executable
 * (all records cleared, empty 0x0 map, SaveFileVersion 12). */
void engine_reset(EngineState& s);
/* Point every Thinker global at this state. Must be called before using any
 * engine function. Returns the previously bound state (may be null). */
EngineState* engine_bind(EngineState* s);
EngineState* engine_state();
/* Allocate the map arrays for MapAreaX/MapAreaY already stored in the state and
 * recompute derived counts exactly like Thinker map_init(). Tiles are zeroed.
 * Returns 0 on success, 1 on invalid dimensions (Thinker convention). */
int map_init();
/* Release the map arrays (map_shutdown). */
void map_shutdown();

/* ---- Thinker globals, bound by engine_bind() ---- */
extern GamePrefs* cs;
extern int* GamePreferences;
extern int* GameMorePreferences;
extern int* GameWarnings;
extern int* GameRules;
extern int* GameInterludeState;
extern int* TutWinMapState;
extern int* GameState;
extern int* DiffLevel;
extern int* VehCount;
extern int* BaseCount;
extern int* CurrentTurn;
extern int* CurrentMissionYear;
extern int* CurrentVehID;
extern int* GameTimeControl;
extern uint8_t* FactionStatus;
extern int* FactionRankings;
extern int* RankingFactionIDUnk1;
extern int* RankingFactionIDUnk2;
extern int* SecretProjects;
extern int* GovernorFaction;
extern int* ProposalPassCount;
extern int* ProposalCallTurn;
extern uint8_t* TechOwners;
extern char* LastSavePath;
extern int* ClimateValueA;
extern int* ClimateValueB;
extern int* ClimateValueC;
extern int* ClimateFutureChange;
extern int* ScoreBonusPts;
extern int* ScorePopTotal;
extern int* SunspotDuration;
extern int* MountPlanetX;
extern int* MountPlanetY;
extern int* DustCloudDuration;
extern int* SaveFileVersion;
extern int* ScnVictFacilityObj;
extern int* SolarFlaresEvent;
extern int* GameMoreRules;
extern int* CurrentFaction;
extern int* ClimateLevel;

extern int* MapAreaX;
extern int* MapAreaY;
extern int* MapRandomSeed;
extern int* MapSeaLevel;
extern int* MapSeaLevelCouncil;
extern int* MapAreaTiles;
extern int* MapAreaSqRoot;
extern int* MapToggleFlat;
extern int* MapLandmarkCount;
extern Landmark* Landmarks;
extern int* MapAbstractX;
extern int* MapAbstractY;
extern int* MapAbstractCount;
extern int* MapSizePlanet;
extern int* MapOceanCoverage;
extern int* MapLandCoverage;
extern int* MapErosiveForces;
extern int* MapPlanetaryOrbit;
extern int* MapCloudCover;
extern int* MapNativeLifeForms;
extern char* MapFilePath;
extern int* MapHalfX;
extern MAP** MapTiles;
extern uint8_t** MapAbstract;

extern MFaction* MFactions;
extern Faction* Factions;
extern int* FactionRankingsUnk;
extern int16_t (*FactionTurnMight)[8];
extern int* ReplayEventSize;
extern ReplayEvent* ReplayEvents;
extern Continent* Continents;
extern UNIT* Units;
extern VEH* Vehs;
extern BASE* Bases;
extern Monument* Monuments;
extern int* TectonicDetonationCount;
extern int* ObjectiveReqVictory;
extern int* ObjectivesSuddenDeathVictory;
extern int* ObjectiveAchievePts;
extern int* VictoryAchieveBonusPts;
extern int* StartingMissionYear;
extern int* EndingMissionYear;
extern Console* MapWin;
extern MapWindow** MapWinPtr;
extern WorldWindow* WorldWin;
extern StatusWindow* StatusWin;
extern int* CanvasWidth;
extern int* CanvasHeight;

extern CRules* Rules;
extern CResourceInfo* ResInfo;
extern CCitizen* Citizen;
extern CResourceName* ResName;
extern CCombatMode* OffenseMode;
extern CCombatMode* DefenseMode;
extern CBonusName* BonusName;
extern CSocialParam* SocialParam;
extern CEnergy* Energy;
extern CChassis* Chassis;
extern CNatural* Natural;
extern CWeapon* Weapon;
extern CSocialField* SocialField;
extern CMandate* Mandate;
extern CMight* Might;
extern CTimeControl* TimeControl;
extern CArmor* Armor;
extern CTech* Tech;
extern CWorldbuilder* WorldBuilder;
extern CMorale* Morale;
extern CReactor* Reactor;
extern COrder* Order;
extern CFacility* Facility;
extern CProposal* Proposal;
extern CAbility* Ability;
extern CTerraform* Terraform;
extern DefaultPref* DefaultPrefs;
extern AlphaIniPref* AlphaIniPrefs;

extern int* ExpansionEnabled;
extern int* MultiplayerActive;
extern int* PbemActive;
extern int* CurrentPlayerFaction;
extern int* ControlTurnMove;
extern int* ComputeBaseID;
extern int* CurrentBaseID;
extern BASE** CurrentBase;
extern int* GameDrawState;
extern int* VehBitError;
extern int* VehDropLiftVehID;
extern int* VehLiftX;
extern int* VehLiftY;
extern int* BaseFindDist;
extern int* VehFindDist;
extern int* VehBattleState;
extern int* SaveFileWrite;
extern int* SaveFileEncrypt;
extern int* SaveFileMenu;
extern int* SaveFileStatus;
extern int* GameLanguage;
extern int* TechValidCount;
extern int* TechCommerceCount;
extern int* AltNatural;
extern int* ParseNumTable;
extern char256* ParseStrBuffer;
extern int* ParseStrGender;
extern int* ParseStrPlurality;
extern int* GenderDefault;
extern int* PluralDefault;

/* Thinker-specific save variables stored in MFactions[0].pad_2. */
struct ThinkerData {
    uint64_t reserved;
    uint64_t game_time_spent;
    char build_date[12];
    uint32_t map_random_value;
    int8_t padding[144];
};
/* Decoded view of MFactions[0].pad_2 (little-endian, like the wire). */
ThinkerData thinker_vars_get();
void thinker_vars_set(const ThinkerData& data);

extern NetMessage* NetMsg;
extern Sprite** FactionPortraits; // [8], UI may fill; null by default

} // namespace engine
} // namespace alpha
