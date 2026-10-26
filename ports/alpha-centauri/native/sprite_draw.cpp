// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_draw.h"
#include <cstdint>
namespace alpha {namespace {
bool overlap(const void*a,std::size_t an,const void*b,std::size_t bn){auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);return x<=y?y-x<an:x-y<bn;}
}
SpriteDrawStatus prepare_sprite_draw(const SpriteDrawContext& ctx,const SpriteTrim& trim,
 const SpriteRect& clip,std::int32_t x,std::int32_t y,SpriteDrawPlan* out){
 using S=SpriteDrawStatus;
 if(!out||overlap(out,sizeof(*out),&ctx,sizeof(ctx))||overlap(out,sizeof(*out),&trim,sizeof(trim))||overlap(out,sizeof(*out),&clip,sizeof(clip)))return S::invalid_input;
 if(!ctx.source_context_supplied)return S::missing_context;
 if(ctx.numerator<1||ctx.denominator<1||ctx.numerator>1024||ctx.denominator>1024||ctx.numerator>4*ctx.denominator||ctx.minimum_width<0||ctx.minimum_height<0||ctx.minimum_width>16384||ctx.minimum_height>16384||x< -524287||x>524287||y< -524287||y>524287||clip.left<0||clip.top<0||clip.left>clip.right||clip.top>clip.bottom||trim.key<0||trim.key>255||trim.width<1||trim.height<1||trim.width>16384||trim.height>16384||trim.pitch!=trim.width||trim.offset_x<0||trim.offset_y<0||trim.full_width<trim.width||trim.full_height<trim.height||std::int64_t(trim.offset_x)+trim.width>trim.full_width||std::int64_t(trim.offset_y)+trim.height>trim.full_height||trim.offset_x>16384||trim.offset_y>16384)return S::invalid_input;
 SpriteDrawPlan plan{};
 volatile float ratio=static_cast<float>(static_cast<double>(ctx.numerator)/ctx.denominator);
 const double factor=ctx.numerator==ctx.denominator?1.0:static_cast<double>(ratio);
 // Truncate AFTER adding the caller coordinate. Splitting the offset truncation
 // would change placements that cross zero.
 plan.full.left=static_cast<std::int32_t>(x+trim.offset_x*factor);
 plan.full.top=static_cast<std::int32_t>(y+trim.offset_y*factor);
 plan.full.right=plan.full.left+static_cast<std::int32_t>(trim.width*factor);
 plan.full.bottom=plan.full.top+static_cast<std::int32_t>(trim.height*factor);
 plan.intersection={plan.full.left>clip.left?plan.full.left:clip.left,plan.full.top>clip.top?plan.full.top:clip.top,plan.full.right<clip.right?plan.full.right:clip.right,plan.full.bottom<clip.bottom?plan.full.bottom:clip.bottom};
 if(plan.intersection.left>=plan.intersection.right||plan.intersection.top>=plan.intersection.bottom){plan.intersection={0,0,0,0};*out=plan;return S::clipped;}
 if(ctx.numerator<ctx.denominator&&(plan.full.right-plan.full.left<ctx.minimum_width||plan.full.bottom-plan.full.top<ctx.minimum_height)){*out=plan;return S::minimum_size;}
 SpriteScaleTable table{};plan.copy.expand=ctx.numerator>ctx.denominator;plan.copy.destination_x=plan.intersection.left;plan.copy.destination_y=plan.intersection.top;
 if(!prepare_sprite_scale(ctx.numerator,ctx.denominator,&table)||!prepare_sprite_axis(table,plan.copy.expand,plan.full.left,plan.full.right,plan.intersection.left,plan.intersection.right,&plan.copy.x)||!prepare_sprite_axis(table,plan.copy.expand,plan.full.top,plan.full.bottom,plan.intersection.top,plan.intersection.bottom,&plan.copy.y))return S::invalid_input;
 *out=plan;return S::ready;
}
SpriteDrawStatus draw_sprite_ram(const SpriteDrawContext& ctx,const SpriteTrim& trim,
 const SpriteRect& clip,std::int32_t x,std::int32_t y,const SpritePixelSource& src,const SpritePixelSurface& dst){
 using S=SpriteDrawStatus;
 if(!src.pixels||src.width<1||src.height<1||src.pitch<src.width||std::uint64_t(src.pitch)*src.height>src.bytes||!dst.pixels||dst.width<1||dst.height<1||dst.pitch<dst.width||std::uint64_t(dst.pitch)*dst.height>dst.bytes||clip.right>dst.width||clip.bottom>dst.height||overlap(dst.pixels,dst.bytes,&ctx,sizeof(ctx))||overlap(dst.pixels,dst.bytes,&trim,sizeof(trim))||overlap(dst.pixels,dst.bytes,&clip,sizeof(clip))||overlap(dst.pixels,dst.bytes,&src,sizeof(src))||overlap(dst.pixels,dst.bytes,&dst,sizeof(dst))||overlap(dst.pixels,dst.bytes,src.pixels,src.bytes)||src.width!=trim.width||src.height!=trim.height||src.pitch!=trim.pitch||src.key!=trim.key)return S::invalid_input;
 SpriteDrawPlan plan{};auto status=prepare_sprite_draw(ctx,trim,clip,x,y,&plan);
 if(status!=S::ready)return status;
 return copy_sprite_pixels(plan.copy,src,dst)?S::ready:S::invalid_input;
}
}
