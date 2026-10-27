// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_TERRAIN_WATER_SPRITES_H
#define ALPHA_TERRAIN_WATER_SPRITES_H
#include "terrain_sea_late.h"
#include "sprite_draw.h"
namespace alpha {
struct WaterItemSprite {SpriteTrim trim;SpritePixelSource pixels;bool source_supplied;};
struct WaterItemSprites {WaterItemSprite item[6];};
struct WaterItemDrawRuntime {std::int32_t caller_x,caller_y,minimum_width,minimum_height;SpriteRect clip;bool coherent_source_cache_supplied,supplied;};
enum class WaterItemDrawStatus:std::uint32_t {rendered=0,missing_runtime=1,held_crop=2,held_operation=3,held_profile=4,invalid_input=5,out_of_memory=6,failed_stage=7};
// Original TER1 crop facts for item10,40,400000,800000,1000000(default/alternate).
// Caller can use qualified extract_sprite_ram on its explicitly prepared owner
// indexed image. No physical PCX loader/palette/cache authority is inferred.
bool water_item_sprite_crop(SeaLateOperation,std::uint32_t variant,SpriteCrop*);
// Acquired-RAM LATE LAYER ONLY, in source intent order, normal zoom-5/5. Caller
// supplies matching owner trims/pixels and actual callerXY/clip/minima/cache.
// Any crop event is a hold BEFORE ALL WRITES, even though sea ignores its return.
// Sea-platform and highbit Sprite_draw_dest operations remain held here; no
// source call/resource is silently omitted. No World/scene/frontend enabling.
// Original Sprite_draw3 prefixes and separate pixel suffixes, NOT USER32 or
// original complete-wrapper execution. Failure is transactional; all inputs and
// sprite allocations must be disjoint from the original destination.
WaterItemDrawStatus render_water_item_sprites(const SeaLateContext&,const WaterItemDrawRuntime&,
 const WaterItemSprites&,const SpritePixelSurface&);
}
#endif
