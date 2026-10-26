// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_TERRAIN_SEA_PLATFORM_H
#define ALPHA_TERRAIN_SEA_PLATFORM_H
#include "sprite_draw.h"
namespace alpha {
struct TerrainSeaPlatformContext {
 std::int32_t caller_x,caller_y,source_zoom,minimum_width,minimum_height;
 std::uint32_t draw_flags,displayed_items,raw_altitude;
 bool late_visible,normal_textured_path,source_context_supplied;
};
enum class TerrainSeaPlatformStatus:std::uint32_t {
 rendered=0,inactive=1,clipped=2,minimum_size=3,missing_context=4,held_path=5,invalid_input=6
};
// An explicit acquired-RAM late layer, AFTER terrain/river/base-work grid and
// BEFORE roads. Source MapWin reverses preliminary polygon offsets before this
// call: caller_x/y are its ORIGINAL input coordinates, without an added tile
// half-width/height. Water MapWin_get_alt returns zero before reading cache.
// Caller supplies the actual resolved observation/late gate, coherent source
// scale-cache/minimum dimensions and normal-textured-path authority. Nothing
// here derives them from save pointers, guessed globals or a gameplay RNG.
// Only the qualified normal zoom -5/5 profile is accepted. Sea contexts sharing
// bit20 remain held because later crop_yield callbacks are not restored.
// Exactly the original prepared ter1 crop100x62 trimmed72x40 offset14,4/key9
// is accepted; physical loader/cache/import/OS/full MapWin execution is unclaimed.
// No World mutation or frontend enabling. Invalid/missing/held inputs never write.
TerrainSeaPlatformStatus prepare_terrain_sea_platform(const TerrainSeaPlatformContext&,
 const SpriteRect&,SpriteDrawPlan*);
TerrainSeaPlatformStatus draw_terrain_sea_platform(const TerrainSeaPlatformContext&,
 const SpriteRect&,const SpritePixelSource&,const SpritePixelSurface&);
}
#endif
