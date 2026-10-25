/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original game_data/record layouts adapted from Thinker, MIT.
 * Copyright (c) Thinker Mod authors; see THINKER-LICENSE.txt.
 * Pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef AC_ALPHA_ORIGINAL_GAME_H
#define AC_ALPHA_ORIGINAL_GAME_H
#include "original_map.h"
namespace alpha {
struct OriginalGamePreferences {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#include "original_prefs_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
};
struct OriginalVehicle {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
};
struct OriginalPrototype {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
};
struct OriginalBase {
#define WIRE_SCALAR(type,name,offset) type name;
#define WIRE_ARRAY(type,name,count,offset) type name[count];
#define WIRE_STRING(name,count,offset) char name[count+1];
#include "original_base_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
};
struct OriginalByteSpan { const uint8_t* data;size_t size; };
struct OriginalViewState { int32_t draw_b,draw_a,flags,zoom,x,y; };
struct OriginalGameView {
 uint32_t data_version,legacy_marker_version,vehicle_count,base_count,replay_size,tick_seed;
 int32_t player,map_cursor[5],canvas_width,canvas_height;
 bool expanded_preferences;
 OriginalGamePreferences preferences;
 OriginalMapView map;
 OriginalViewState views[9];
 OriginalByteSpan factions,faction_definitions,rankings,faction_turn_might,replay,continents;
 OriginalByteSpan prototypes,bunglist,vehicles,bases,scenario,scenario_name,objectives,monuments,tectonic;
};
enum class GameReadError { ok,bad_header,unsupported_version,unsupported_features,truncated,
                          bad_counts,bad_player,bad_map,trailing_data,bad_index };
/* Borrowed immutable spans; buffer must remain alive/unchanged. Plain TERRAN,
 * outer0x56 and data versions7..12 only. Encrypted/network variants fail
 * explicitly. Reads the complete supported container, including UI/tick footer.
 * Native records use explicit endian decoding, not packed copies. Legacy base
 * facility bytes are zero-extended and preference projects default-extended as
 * in source, but gameplay ID conversions (weapons/production IDs) are NOT
 * implicit: raw wire IDs remain tagged by data_version for world initialization.
 * This imports data, not original load side-effects, AI/upkeep or a running game.
 * Every output remains unchanged on failure. Actors require a validated view.
 */
GameReadError read_original_game(const uint8_t* bytes,size_t length,OriginalGameView& output);
GameReadError original_vehicle(const OriginalGameView& view,size_t index,OriginalVehicle& output);
GameReadError original_prototype(const OriginalGameView& view,size_t index,OriginalPrototype& output);
GameReadError original_base(const OriginalGameView& view,size_t index,OriginalBase& output);
const char* game_read_error_name(GameReadError error);
}
#endif
