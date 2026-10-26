// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_platform.h"
#include <cstring>
#include <cstdio>
#include <initializer_list>
static alpha::SpriteDrawPlan out;
static unsigned char source[2880],pixels[16384];
int main(){using S=alpha::TerrainSeaPlatformStatus;alpha::TerrainSeaPlatformContext base{0,0,-5,0,0,1,0x8000,2,true,true,true},c=base;alpha::SpriteRect clip{0,0,128,128};std::memset(&out,0xa5,sizeof(out));unsigned char saved[sizeof(out)];std::memcpy(saved,&out,sizeof(out));unsigned n=0;
 for(unsigned k=0;k<12;++k){c=base;S expected=S::invalid_input;switch(k){case 0:c.source_context_supplied=false;expected=S::missing_context;break;case 1:c.normal_textured_path=false;expected=S::held_path;break;case 2:c.source_zoom=0;expected=S::held_path;break;case 3:c.draw_flags=0x21;expected=S::held_path;break;case 4:c.raw_altitude=8;break;case 5:c.minimum_width=-1;break;case 6:c.minimum_height=16385;break;case 7:c.caller_x=524288;break;case 8:c.late_visible=false;expected=S::inactive;break;case 9:c.raw_altitude=3;expected=S::inactive;break;case 10:c.draw_flags=0;expected=S::inactive;break;case 11:c.displayed_items=0;expected=S::inactive;break;}if(alpha::prepare_terrain_sea_platform(c,clip,&out)!=expected||std::memcmp(saved,&out,sizeof(out)))return 20;++n;}
 c=base;for(const void*p:{static_cast<const void*>(&c),static_cast<const void*>(&clip)})for(unsigned k=0;k<3;++k){auto a=reinterpret_cast<std::uintptr_t>(p);if(k==1)++a;if(k==2)a-=sizeof(out)-1;if(alpha::prepare_terrain_sea_platform(c,clip,reinterpret_cast<alpha::SpriteDrawPlan*>(a))!=S::invalid_input)return 20;++n;}
 alpha::SpritePixelSource src{source,sizeof(source),72,40,72,9};alpha::SpritePixelSurface dst{pixels,sizeof(pixels),128,128,128};const auto clean=dst;for(const void*p:{static_cast<const void*>(&c),static_cast<const void*>(&clip),static_cast<const void*>(&src),static_cast<const void*>(&dst),static_cast<const void*>(source),static_cast<const void*>(source+sizeof(source)-1)})for(unsigned k=0;k<2;++k){auto a=reinterpret_cast<std::uintptr_t>(p);if(k)a-=sizeof(pixels)-1;dst.pixels=reinterpret_cast<unsigned char*>(a);if(alpha::draw_terrain_sea_platform(c,clip,src,dst)!=S::invalid_input)return 20;dst=clean;++n;}
 if(alpha::prepare_terrain_sea_platform(c,clip,&out)!=S::rendered||out.full.left!=9||out.full.top!=2){return 20;}++n;std::printf("%u sea-layer boundary controls\n",n);return 0;}
