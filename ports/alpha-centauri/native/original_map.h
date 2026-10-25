/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original MP layout and loading flow adapted from Thinker savegame.cpp,
 * engine.cpp/engine_types.h, pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors; MIT notice in THINKER-LICENSE.txt.
 */
#ifndef AC_ALPHA_ORIGINAL_MAP_H
#define AC_ALPHA_ORIGINAL_MAP_H
#include <stdint.h>
#include <stddef.h>
namespace alpha {
constexpr size_t original_map_metadata_size=0xaa4;
constexpr size_t original_map_tile_size=44;
struct OriginalMapLandmark { int32_t x,y;char name[33];bool raw_coordinate_on_map; };
struct OriginalMapInfo {
 uint32_t width,height,tile_count,abstract_width,abstract_height,abstract_count,random_seed;
 int32_t sea_level,council_sea_level,world_settings[7];
 bool flat;uint32_t landmark_count,off_map_landmarks;OriginalMapLandmark landmarks[64];
 bool stored_counts_differ;
};
struct OriginalMapTile {
 uint8_t climate,contour,val2,region,visibility,val3,unknown;
 int8_t owner;
 uint32_t items,landmarks,visible_items[7];
};
/* Borrowed, immutable byte spans: input must remain alive/unchanged. No imported
 * process pointers or packed native structs. All integer interpretation is LE.
 * Tile access requires an unmodified successfully parsed view.
 * metadata retains unknown fields including inert historical pointer BYTES;
 * those fields are never treated as native addresses. Runtime needs owned tile
 * state initialized with original_map_tile(), not casts/memcpy of the file.
 */
struct OriginalMapView {
 OriginalMapInfo info;
 const uint8_t* metadata;
 const uint8_t* tiles;
 const uint8_t* abstract;
};
enum class MapReadError { ok,bad_header,unsupported_version,bad_dimensions,bad_landmarks,
                        truncated,trailing_data,bad_index };
/* Original TERRANMAP\0/0x1a format version5 only. Dimensions bounded2..4096
 * even width,1..4096 height. Counts and abstract dimensions recomputed exactly
 * as map_init; stored counts/pointers cannot drive allocation. Exact file
 * length required. Original marker bytes/unknown bits preserved without game
 * semantic interpretation. Raw off-map landmark coordinates are preserved and
 * flagged (present in the supplied large Planet maps), never used as indices. No original fixup_landmarks/editor field clearing
 * or full SAV/SC game-state import is claimed. Outputs unchanged on failure.
 */
MapReadError read_original_map(const uint8_t* bytes,size_t length,OriginalMapView& output);
MapReadError original_map_tile(const OriginalMapView& view,size_t index,OriginalMapTile& output);
const char* map_read_error_name(MapReadError error);
}
#endif
