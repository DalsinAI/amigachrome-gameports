/* SPDX-License-Identifier: GPL-3.0-or-later
 * Rules tables loader (WP0b): alpha.txt / alphax.txt, labels.txt and the
 * faction files into the bound EngineState.
 * Ported from Thinker Mod src/config.cpp, src/game.cpp (labels_init) and
 * src/strings.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

/* Original label table (labels.txt #labels). */
struct Label {
    char** labels;
    int label_count;
};

/* Thinker globals for the plain string tables (engine.cpp 0x945D48 ...).
 * Unlike the record tables these are shared by every EngineState; they only
 * hold text from the last engine_load_rules call. */
extern Label* TextLabels;
extern char** Compass;        // [MaxCompassNum]
extern char** PlansShortName; // [MaxPlanNum]
extern char** PlansFullName;  // [MaxPlanNum]
extern char** TriadName;      // [MaxTriadNum]
extern char** Difficulty;     // [MaxDiffNum]
extern char** Mood;           // [MaxMoodNum]
extern char** Repute;         // [MaxReputeNum]

/* Faction files from #FACTIONS, #NEWFACTIONS and #CUSTOMFACTIONS
 * (filename, search key), as in Thinker main.cpp. */
extern std::vector<std::pair<std::string,std::string>> faction_pool;
extern std::vector<std::pair<size_t,size_t>> faction_pair;

/* Number of rows in the variable-size tables of the loaded variant.
 * Alien Crossfire: 26 weapons, 14 armors, 29 abilities, 133 facilities,
 * 41 bonus names. Base game: 21, 10, 24, 71, 29 (rows are placed into the
 * Alien Crossfire slots, see rules.cpp; slots without a row are disabled). */
struct RulesCounts {
    int weapons;
    int armors;
    int abilities;
    int facilities;
    int bonus_names;
};
RulesCounts rules_counts(int expansion);
/* Engine slot for row `row` of a variable-size section in the loaded variant. */
int rules_weapon_slot(int row);
int rules_facility_slot(int row); // row 1..counts.facilities

/* Load every rules table from directory `dir` (null or "" = current
 * directory) into the bound EngineState: alphax.txt when expansion is
 * nonzero (Alien Crossfire, sets *ExpansionEnabled = 1), otherwise alpha.txt
 * (base game). Also reads labels.txt and the eight faction files named by
 * #FACTIONS/#NEWFACTIONS into MFactions[1..7], and the #UNITS default
 * prototypes into Units[0..] (make_proto derives the rest). File names are
 * matched as given, then lower case, then upper case.
 * Returns 0 on success, otherwise 1 with engine_rules_error() describing the
 * first problem (missing file or section, unknown key). */
int engine_load_rules(const char* dir, int expansion);
/* Same, choosing whether #UNITS and the faction files are read
 * (Thinker read_rules(tgl_all_rules)). */
int engine_load_rules_ex(const char* dir, int expansion, int all_rules);
const char* engine_rules_error();
/* Faction files that could not be opened during the last load (Thinker shows
 * PLAYERFILE and continues with the cleared faction). */
int engine_rules_missing_factions();

/* ---- Thinker functions (config.cpp, game.cpp) ---- */
int __cdecl text_open(const char* filename, const char* label);
void __cdecl text_close();
char* __cdecl text_get();
char* __cdecl text_item();
char* __cdecl text_item_string();
int __cdecl text_item_number();
int __cdecl text_item_binary();
int __cdecl text_get_number(int min_val, int max_val);
char* text_buf_ptr();
int __cdecl tech_name(char* name);
int __cdecl chas_name(char* name);
int __cdecl weap_name(char* name);
int __cdecl arm_name(char* name);
int __cdecl tech_item();
int __cdecl read_basic_rules();
int __cdecl read_tech();
void __cdecl clear_faction(MFaction* plr);
void __cdecl read_faction(MFaction* plr, int toggle);
void __cdecl read_faction2(int faction_id);
int __cdecl read_factions();
size_t pick_random_faction(int faction_id);
void __cdecl noun_item(int32_t* gender, int32_t* plural);
int __cdecl read_units();
int __cdecl read_rules(int tgl_all_rules);
void __cdecl prefs_fac_load();
int __cdecl labels_init();
void __cdecl labels_shutdown();
char* label_get(size_t index);

/* ---- Thinker string helpers (strings.cpp) ---- */
int __cdecl btoi(const char* src);
int __cdecl htoi(const char* src);
int __cdecl stoi(const char* src);
void __cdecl purge_spaces(char* buf);
void __cdecl kill_lf(char* buf);
char* strtrail(char* s);
char* strtrim(char* s);
int _stricmp(const char* a, const char* b);
int _strnicmp(const char* a, const char* b, size_t n);
/* Replaces Strings_put(TextTable, src): returns a stable interned copy. */
char* Strings_put(const char* src);
/* Original parse_string for the $STRn/$WORDn/$NUMn tokens used in rules text. */
int __cdecl parse_string(const char* src, char* dst);

} // namespace engine
} // namespace alpha
