// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_SPRITE_DRAW_H
#define ALPHA_SPRITE_DRAW_H
#include "sprite_extract.h"
namespace alpha {
struct SpriteRect {std::int32_t left,top,right,bottom;};
struct SpriteDrawContext {
 std::int32_t numerator,denominator,minimum_width,minimum_height;
 // Caller explicitly supplies the fresh/consistent source scale-cache profile
 // and original nearest/double-precision numeric profile. No runtime defaults.
 bool source_context_supplied;
};
struct SpriteDrawPlan {SpriteRect full,intersection;SpritePixelCopyInput copy;};
enum class SpriteDrawStatus:std::uint32_t {ready=0,clipped=1,minimum_size=2,missing_context=3,invalid_input=4};
SpriteDrawStatus prepare_sprite_draw(const SpriteDrawContext&,const SpriteTrim&,
 const SpriteRect& clip,std::int32_t caller_x,std::int32_t caller_y,SpriteDrawPlan*);
// Owned rectangle intersection + qualified numeric pixel copier; does not call
// or claim qualification of USER32/whole original Sprite_draw3.
SpriteDrawStatus draw_sprite_ram(const SpriteDrawContext&,const SpriteTrim&,
 const SpriteRect& clip,std::int32_t caller_x,std::int32_t caller_y,
 const SpritePixelSource&,const SpritePixelSurface&);
}
#endif
