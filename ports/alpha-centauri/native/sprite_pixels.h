// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_SPRITE_PIXELS_H
#define ALPHA_SPRITE_PIXELS_H
#include "sprite_scale.h"
#include <cstddef>
namespace alpha {
struct SpritePixelSource {
 const std::uint8_t* pixels;std::size_t bytes;
 std::int32_t width,height,pitch;std::uint8_t key;
};
struct SpritePixelSurface {
 std::uint8_t* pixels;std::size_t bytes;
 std::int32_t width,height,pitch;
};
struct SpritePixelCopyInput {
 SpriteAxisPlan x,y;
 bool expand;
 std::int32_t destination_x,destination_y;
};
// The caller supplies validated prepared plans and a real destination region.
// Numeric clipped-copy boundary only; no loader/OS intersection/cache callback.
// Source spans include the original post-last-pixel advance. All validation is
// completed before drawing; unsupported overlap fails without writing.
bool copy_sprite_pixels(const SpritePixelCopyInput&,const SpritePixelSource&,
 const SpritePixelSurface&);
}
#endif
