// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_SPRITE_EXTRACT_H
#define ALPHA_SPRITE_EXTRACT_H
#include "sprite_pixels.h"
namespace alpha {
struct SpriteCrop {std::int32_t x,y,width,height;};
struct SpriteTrim {std::int32_t key,pitch,width,height,full_width,full_height,offset_x,offset_y;};
enum class SpriteExtractStatus:std::uint32_t {ok=0,invalid_input=3,insufficient_storage=4,empty=9};
// Acquired indexed RAM image + explicit crop/key. No PCX loader, arena/cache
// constructor or OS callback. Caller owns destination storage. Invalid input,
// aliases and insufficient storage leave both outputs untouched. A valid empty
// crop reports the original empty descriptor and does not touch pixel storage.
SpriteExtractStatus extract_sprite_ram(const SpritePixelSource&,const SpriteCrop&,
 SpriteTrim* out,std::uint8_t* pixels,std::size_t capacity);
}
#endif
