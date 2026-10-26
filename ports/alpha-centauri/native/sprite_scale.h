// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ALPHA_SPRITE_SCALE_H
#define ALPHA_SPRITE_SCALE_H
#include <cstdint>
namespace alpha {
constexpr unsigned sprite_scale_capacity=800;
struct SpriteScaleTable { std::int32_t steps[sprite_scale_capacity]; };
struct SpriteAxisPlan {
 std::int32_t source_offset,destination_offset,source_span,destination_span;
 std::uint32_t count;
 std::int32_t steps[sprite_scale_capacity];
};
// Numeric preparation only. Does not load/extract sprites or call the original
// OS rectangle operation. The caller supplies an already intersected axis.
bool prepare_sprite_scale(std::int32_t numerator,std::int32_t denominator,SpriteScaleTable* out);
bool prepare_sprite_axis(const SpriteScaleTable& table,bool expand,
 std::int32_t full_begin,std::int32_t full_end,
 std::int32_t clipped_begin,std::int32_t clipped_end,SpriteAxisPlan* out);
}
#endif
