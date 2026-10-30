/* SPDX-License-Identifier: GPL-3.0-or-later
 * asmac: a small, stable C API over the Alpha Centauri play layer
 * (native/play_session.h, alpha::play::Session) and the engine it drives.
 *
 * Only plain C types cross this boundary: ints, fixed-size char arrays,
 * byte buffers and the structs below (no bool, no C++ types). Booleans are
 * int 0/1. Strings are NUL-terminated, cp1252 like the rules text. The
 * Python package alpha_smac loads the shared library built by
 * build_python.sh with ctypes and mirrors these structs field by field.
 *
 * Status codes are alpha::play::PlayStatus (ASMAC_OK .. ASMAC_NEEDS_CONFIRM),
 * command kinds alpha::play::CommandKind. Every call binds the session's
 * engine state first (the engine binds one state at a time); the library is
 * not thread-safe. Bump ASMAC_ABI_VERSION when a struct or signature changes.
 */
#ifndef ALPHA_ASMAC_H
#define ALPHA_ASMAC_H
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__)
#define ASMAC_API __attribute__((visibility("default")))
#else
#define ASMAC_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ASMAC_ABI_VERSION 2

enum {
 ASMAC_OK=0,ASMAC_REFUSED=1,ASMAC_BAD_ARGS=2,ASMAC_NO_GAME=3,ASMAC_UNAVAILABLE=4,ASMAC_IO=5,
 ASMAC_GAME_OVER=6,ASMAC_ATTACK=7,ASMAC_NEEDS_CONFIRM=8,
 ASMAC_SHORT_BUFFER=100 /* asmac only: the caller's buffer is too small */
};

typedef struct asmac_session asmac_session;
typedef struct asmac_world asmac_world;

typedef struct asmac_command { int kind,a,b,c,d,e;char text[32]; } asmac_command_t;
typedef struct asmac_message { int turn,kind;char text[120]; } asmac_message_t;
typedef struct asmac_turn_info { int status,faction,turn,year,events; } asmac_turn_info_t;
typedef struct asmac_unit {
 int valid,id,unit_id,faction,x,y,damage,moves_left,speed,order,home_base;
 int colony,former,combat,own;char name[32];
} asmac_unit_t;
/* One map square of the engine (alpha::engine::MAP), decoded. */
typedef struct asmac_tile {
 int valid,altitude,ocean,rainfall,temperature,rocky,owner,region;
 unsigned visibility;   /* bit f: faction f has seen the square */
 unsigned items;        /* BIT_* of engine_enums.h (road 0x4, fungus 0x20, ...) */
 unsigned landmarks;
 int base,unit_count,unit_faction; /* base id or -1; units on the square; faction of the first */
} asmac_tile_t;
typedef struct asmac_faction {
 int valid,id,alive,human,credits,bases,units,research_tech,labs,techs_known;
 int social[4];
 char noun[24],formal_name[40],leader[24],title[24],adjective[64];
} asmac_faction_t;
typedef struct asmac_base {
 int valid,id,x,y,faction,pop_size,nutrients,minerals,nutrient_surplus,mineral_surplus,energy_surplus;
 int current_item,queue_count,cost,turns;
 char name[26],item_name[32];
} asmac_base_t;

/* Frames and text items of the ui_*.cpp panel renderers. */
typedef struct asmac_frame { uint8_t* rgb;size_t capacity;unsigned width,height; } asmac_frame_t;
typedef struct asmac_rect { int x,y,w,h; } asmac_rect_t;
typedef struct asmac_text_item { int x,baseline,width,ink,heading;char text[96]; } asmac_text_item_t;

/* Panel models (ui_basewin.h, ui_research.h, ...), bools as int. */
typedef struct asmac_production_item { int id,kind;char name[32];int cost,turns; } asmac_production_item_t;
typedef struct asmac_production {
 int valid,base;char name[26];
 int current;char current_name[32];int minerals,cost,surplus,turns;
 int queue_count;int queue[10];char queue_names[10][32];
 int hurry_cost,credits,hurried;
 int count;asmac_production_item_t items[96];int selected;
} asmac_production_t;
typedef struct asmac_research {
 int valid,faction,current;char current_name[40];
 int accumulated,cost,labs,known;
 int count;int ids[32];char names[32][40];char categories[32][12];
 int selected,confirm;
} asmac_research_t;
typedef struct asmac_social {
 int valid,faction;char categories[4][24];char models[4][4][24];
 int available[4][4];int cost[4][4];int current[4];int credits;int row,col;
} asmac_social_t;
typedef struct asmac_design_choice { int id;char name[32]; } asmac_design_choice_t;
typedef struct asmac_design {
 int valid,faction;int counts[5];asmac_design_choice_t choices[5][32];int selected[5];
 int row,preview_ok;char preview_name[40];int preview_cost,last_unit;
} asmac_design_t;
typedef struct asmac_battle {
 int valid,preview,x,y;char attacker[32],defender[32],attacker_owner[24],defender_owner[24];
 int odds,outcome;char outcome_text[48];
 int offense,defense,hp_atk_before,hp_def_before,hp_atk_after,hp_def_after,rounds,attacker_hits,defender_hits,killed;
 int promoted_atk,promoted_def,base_captured,base_destroyed;
 int mod_count[2];char mods[2][4][48];
} asmac_battle_t;
typedef struct asmac_diplo {
 int valid,player,other;char other_name[40],leader[48],mood[24],status[48];
 int count;int proposals[16];char labels[16][48];int selected,last_answer;char reply[160];
} asmac_diplo_t;
typedef struct asmac_world_info { int vehicle_count,base_count,current_turn,player; } asmac_world_info_t;

ASMAC_API int asmac_abi_version(void);
ASMAC_API const char* asmac_status_name(int status);
ASMAC_API const char* asmac_command_name(int kind);
ASMAC_API int asmac_parse_command(const char* line,asmac_command_t* out); /* 1 ok, 0 unknown/short */
ASMAC_API int asmac_new_game_available(void);
ASMAC_API int asmac_diplomacy_available(void);
ASMAC_API uint32_t asmac_fnv1a(const uint8_t* p,size_t n,uint32_t h); /* h=2166136261 to start */

/* Sessions. asmac_session_shared() is play::session(), the one the native
 * title screen and map window use; free it never. */
ASMAC_API asmac_session* asmac_session_new(void);
ASMAC_API asmac_session* asmac_session_shared(void);
ASMAC_API void asmac_session_free(asmac_session*);
ASMAC_API int asmac_load_rules(asmac_session*,const char* dir,int expansion);
ASMAC_API const char* asmac_rules_error(asmac_session*);
ASMAC_API int asmac_new_game(asmac_session*,uint32_t seed,int faction,int difficulty,int map_size);
ASMAC_API int asmac_load_game(asmac_session*,const uint8_t* bytes,size_t length);
/* save_daemon_bytes / game_write_bytes into the session's buffer; then copy
 * it out with asmac_buffer (returns ASMAC_SHORT_BUFFER if cap < length). */
ASMAC_API int asmac_save_game(asmac_session*,size_t* length);
ASMAC_API int asmac_view_bytes(asmac_session*,size_t* length);
ASMAC_API int asmac_buffer(asmac_session*,uint8_t* out,size_t cap);
ASMAC_API int asmac_loaded(asmac_session*);
ASMAC_API int asmac_game_over(asmac_session*);
ASMAC_API int asmac_run(asmac_session*,const asmac_command_t*);
ASMAC_API void asmac_last_turn(asmac_session*,asmac_turn_info_t*);
ASMAC_API int asmac_message_count(asmac_session*);
ASMAC_API int asmac_message(asmac_session*,int i,asmac_message_t*);
ASMAC_API void asmac_take_messages(asmac_session*);
ASMAC_API int asmac_player(asmac_session*);
ASMAC_API int asmac_turn(asmac_session*);
ASMAC_API int asmac_year(asmac_session*);
ASMAC_API int asmac_credits(asmac_session*);
ASMAC_API int asmac_vehicle_count(asmac_session*);
ASMAC_API int asmac_base_count(asmac_session*);
ASMAC_API int asmac_unit(asmac_session*,int veh,asmac_unit_t*);
ASMAC_API int asmac_next_unit(asmac_session*,int veh);
ASMAC_API int asmac_unit_at(asmac_session*,int x,int y);
ASMAC_API int asmac_base_at(asmac_session*,int x,int y);
ASMAC_API int asmac_step_target(asmac_session*,int veh,int direction,int* x,int* y);
ASMAC_API int asmac_hostile_at(asmac_session*,int x,int y);
ASMAC_API int asmac_upgrade_target(asmac_session*,int veh);
ASMAC_API int asmac_contacts(asmac_session*,int* ids,int max);
ASMAC_API uint32_t asmac_digest(asmac_session*);

/* Engine data for a map view (read only). */
ASMAC_API int asmac_map_size(asmac_session*,int* width,int* height);
ASMAC_API int asmac_tile(asmac_session*,int x,int y,asmac_tile_t*);
/* All squares at once, row-major width*height (squares with (x+y) odd are
 * not on the map and come back with valid=0). */
ASMAC_API int asmac_tiles(asmac_session*,asmac_tile_t* out,int cap);
ASMAC_API int asmac_faction(asmac_session*,int id,asmac_faction_t*);
ASMAC_API int asmac_base(asmac_session*,int id,asmac_base_t*);
ASMAC_API const char* asmac_item_name(asmac_session*,int item);
/* The faction is played by a person (1) or the computer (0). Turning the
 * player's own faction off makes End Turn play whole computer rounds. */
ASMAC_API int asmac_set_human(asmac_session*,int faction,int human);

/* Panels: models are in/out (the session keeps the cursor of the model it
 * is given, as the native panels do). */
ASMAC_API int asmac_base_production(asmac_session*,int base,asmac_production_t*);
ASMAC_API int asmac_research(asmac_session*,asmac_research_t*);
ASMAC_API int asmac_social(asmac_session*,asmac_social_t*);
ASMAC_API int asmac_design(asmac_session*,asmac_design_t*);
ASMAC_API void asmac_design_preview(asmac_session*,asmac_design_t*);
ASMAC_API void asmac_design_selection(const asmac_design_t*,int* chassis,int* weapon,int* armor,int* reactor,int* abilities);
ASMAC_API void asmac_battle_model(asmac_session*,asmac_battle_t*);
ASMAC_API int asmac_diplomacy(asmac_session*,int other,asmac_diplo_t*);
ASMAC_API int asmac_ui_key_from_char(int c);
ASMAC_API int asmac_base_production_key(asmac_production_t*,int c);
ASMAC_API int asmac_research_key(asmac_research_t*,int key);
ASMAC_API int asmac_social_key(asmac_social_t*,int key);
ASMAC_API int asmac_design_key(asmac_design_t*,int key);
ASMAC_API int asmac_battle_key(const asmac_battle_t*,int key);
ASMAC_API int asmac_diplo_key(asmac_diplo_t*,int key);
/* Renderers: draw into frame (RGB24) and return the number of text items. */
ASMAC_API int asmac_render_base_production(const asmac_production_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);
ASMAC_API int asmac_render_research(const asmac_research_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);
ASMAC_API int asmac_render_social(const asmac_social_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);
ASMAC_API int asmac_render_design(const asmac_design_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);
ASMAC_API int asmac_render_battle(const asmac_battle_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);
ASMAC_API int asmac_render_diplo(const asmac_diplo_t*,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);

/* OriginalWorld: the window's view rebuilt from SAV bytes (view_bytes). */
ASMAC_API asmac_world* asmac_world_load(const uint8_t* bytes,size_t length,uint32_t runtime_seed,int* error);
ASMAC_API const char* asmac_world_error_name(int error);
ASMAC_API void asmac_world_free(asmac_world*);
ASMAC_API void asmac_world_info_get(const asmac_world*,asmac_world_info_t*);
ASMAC_API int asmac_world_vehicle(const asmac_world*,int i,int* x,int* y,int* unit_id);
ASMAC_API int asmac_world_base_faction(const asmac_world*,int i);
/* build_base_window + render_base_window; -1 if the model cannot be built. */
ASMAC_API int asmac_world_render_base_window(const asmac_world*,int base,int viewer,const asmac_frame_t*,const asmac_rect_t*,asmac_text_item_t*,int cap);

/* ---- ABI 2: map view with the user's original art (asmac_view.cpp) ---- */
typedef struct asmac_view asmac_view;
typedef struct asmac_pcx asmac_pcx;
enum { ASMAC_ART_TERRAIN=1,ASMAC_ART_UNITS=2,ASMAC_ART_CONSOLE=4,ASMAC_ART_PALETTE=8,ASMAC_ART_INFANTRY=16,ASMAC_ART_SPEEDER=32,ASMAC_ART_RULES=64 };
/* Reads texture.pcx, Units.pcx, console.pcx, the CVR models and the rules
 * tables from dir (case-insensitive names). Missing files only disable the
 * matching drawing; asmac_view_assets reports what was found (faction art
 * mask in bits 16..23 after the first update). */
ASMAC_API asmac_view* asmac_view_open(const char* dir,int expansion);
ASMAC_API void asmac_view_free(asmac_view*);
ASMAC_API unsigned asmac_view_assets(const asmac_view*);
/* Rebuild the view from the session (OriginalWorld of view_bytes). reopen=1
 * after a new game or a load: faction art and MapViewExtras are reopened. */
ASMAC_API int asmac_view_update(asmac_view*,asmac_session*,int reopen);
ASMAC_API void asmac_view_set_viewer(asmac_view*,int viewer); /* -1 all squares */
ASMAC_API int asmac_view_viewer(const asmac_view*);
/* The native map window's map drawing on an RGB24 frame of any size, the
 * camera square at the frame centre, zoom 1 or 2 (64x32 / 128x64 squares).
 * Returns the label text items (base names, designs without a model). */
ASMAC_API int asmac_view_draw_map(asmac_view*,const asmac_frame_t*,int camera_x,int camera_y,int zoom,int selected_vehicle,asmac_text_item_t* labels,int cap);
ASMAC_API int asmac_view_draw_minimap(asmac_view*,const asmac_frame_t*,const asmac_rect_t*,int camera_x,int camera_y,int zoom,int view_w,int view_h);
ASMAC_API int asmac_view_minimap_click(const asmac_view*,const asmac_rect_t*,int px,int py,int* x,int* y);
ASMAC_API int asmac_view_draw_base_window(asmac_view*,const asmac_frame_t*,const asmac_rect_t*,int base,asmac_text_item_t*,int cap);
ASMAC_API int asmac_view_draw_console(const asmac_view*,const asmac_frame_t*,int x,int y);
ASMAC_API int asmac_view_screen_tile(const asmac_view*,int camera_x,int camera_y,int zoom,int width,int height,int px,int py,int* x,int* y);
ASMAC_API int asmac_view_tile_position(const asmac_view*,int camera_x,int camera_y,int zoom,int width,int height,int x,int y,int* sx,int* sy);
ASMAC_API int asmac_view_tile_visible(const asmac_view*,int x,int y);
ASMAC_API int asmac_view_messages(asmac_view*,char* out,int cap);
ASMAC_API int asmac_view_faction_stem(const asmac_view*,int faction,char* out,int cap);
ASMAC_API asmac_pcx* asmac_pcx_open(const char* dir,const char* name);
ASMAC_API void asmac_pcx_free(asmac_pcx*);
ASMAC_API int asmac_pcx_size(const asmac_pcx*,int* w,int* h);
ASMAC_API int asmac_pcx_rgb(const asmac_pcx*,uint8_t* out,size_t cap);
ASMAC_API int asmac_find_file(const char* dir,const char* name,char* out,int cap);

/* New game screens (ui_newgame.h). Load the rules first; keys are NewGameKey
 * (0 up, 1 down, 2 left, 3 right, 4 enter, 5 back); screens NG_FACTION 0,
 * NG_SETTINGS 1, NG_PLANETFALL 2, NG_DONE 3, NG_CANCELLED 4. On NG_DONE,
 * asmac_adopt_new_game starts turn 1 like the native frontend. */
typedef struct asmac_line { char text[96];int selected,heading; } asmac_line_t;
ASMAC_API int asmac_newgame_open(asmac_session*,uint32_t seed);
ASMAC_API int asmac_newgame_key(asmac_session*,int key);
ASMAC_API int asmac_newgame_screen(asmac_session*);
ASMAC_API int asmac_newgame_row(asmac_session*);
ASMAC_API int asmac_newgame_start_status(asmac_session*);
ASMAC_API int asmac_newgame_view(asmac_session*,char* title,int title_cap,asmac_line_t* lines,int cap);
ASMAC_API int asmac_adopt_new_game(asmac_session*);

/* Audio catalog (audio_events.h): event keys, the fx file of each event under
 * an optional alpha-audio.cfg text, mixer levels, music sets, voice paths. */
ASMAC_API int asmac_audio_event_count(void);
ASMAC_API const char* asmac_audio_event_key(int event);
ASMAC_API int asmac_audio_event_file(const char* settings,size_t length,int event,char* out,int cap);
ASMAC_API int asmac_audio_levels(const char* settings,size_t length,int* out,int count); /* master sfx voice music sfx_on voice_on music_on */
ASMAC_API const char* asmac_faction_music_set(const char* stem);
ASMAC_API int asmac_music_set_count(void);
ASMAC_API const char* asmac_music_set(int i);
ASMAC_API int asmac_technology_voice_path(int id,char* out,int cap);
ASMAC_API int asmac_facility_voice_path(int id,char* out,int cap);
ASMAC_API int asmac_faction_voice_path(const char* stem,int expansion,char* out,int cap);

#ifdef __cplusplus
}
#endif
#endif
