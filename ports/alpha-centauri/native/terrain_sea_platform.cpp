// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_platform.h"
namespace alpha {namespace {
bool overlap(const void*a,std::size_t n,const void*b,std::size_t m){auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);return n&&m&&(x<=y?y-x<n:x-y<m);}
TerrainSeaPlatformStatus gate(const TerrainSeaPlatformContext&c){using S=TerrainSeaPlatformStatus;
 if(!c.source_context_supplied)return S::missing_context;
 if(c.raw_altitude>7||c.minimum_width<0||c.minimum_height<0||c.minimum_width>16384||c.minimum_height>16384||c.caller_x< -524287||c.caller_x>524287||c.caller_y< -524287||c.caller_y>524287)return S::invalid_input;
 if(!c.normal_textured_path||(c.source_zoom!=-5&&c.source_zoom!=5)||(c.draw_flags&~0x04000421u))return S::held_path;
 if(!c.late_visible||c.raw_altitude>=3||!(c.draw_flags&1u)||!(c.displayed_items&0x8000u))return S::inactive;
 if(c.draw_flags&0x20u)return S::held_path;
 return S::rendered;
}
TerrainSeaPlatformStatus convert(SpriteDrawStatus s){using S=TerrainSeaPlatformStatus;switch(s){case SpriteDrawStatus::ready:return S::rendered;case SpriteDrawStatus::clipped:return S::clipped;case SpriteDrawStatus::minimum_size:return S::minimum_size;case SpriteDrawStatus::missing_context:return S::missing_context;default:return S::invalid_input;}}
SpriteTrim trim(){return {9,72,72,40,100,62,14,4};}
SpriteDrawContext context(const TerrainSeaPlatformContext&c){return {c.source_zoom+16,16,c.minimum_width,c.minimum_height,true};}
}
TerrainSeaPlatformStatus prepare_terrain_sea_platform(const TerrainSeaPlatformContext&c,const SpriteRect&clip,SpriteDrawPlan*out){using S=TerrainSeaPlatformStatus;
 if(!out||overlap(out,sizeof(*out),&c,sizeof(c))||overlap(out,sizeof(*out),&clip,sizeof(clip)))return S::invalid_input;
 auto status=gate(c);if(status!=S::rendered)return status;
 const auto t=trim();const auto ctx=context(c);return convert(prepare_sprite_draw(ctx,t,clip,c.caller_x,c.caller_y,out));
}
TerrainSeaPlatformStatus draw_terrain_sea_platform(const TerrainSeaPlatformContext&c,const SpriteRect&clip,const SpritePixelSource&src,const SpritePixelSurface&dst){using S=TerrainSeaPlatformStatus;
 if(overlap(dst.pixels,dst.bytes,&c,sizeof(c)))return S::invalid_input;
 auto status=gate(c);if(status!=S::rendered)return status;
 const auto t=trim();const auto ctx=context(c);return convert(draw_sprite_ram(ctx,t,clip,c.caller_x,c.caller_y,src,dst));
}
}
