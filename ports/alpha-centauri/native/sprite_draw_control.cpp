// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_draw.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
static unsigned char source[2880],pixels[16384],clean[16384];
static alpha::SpriteDrawPlan out;
int main(){using S=alpha::SpriteDrawStatus;std::memset(source,37,sizeof(source));std::memset(pixels,199,sizeof(pixels));std::memcpy(clean,pixels,sizeof(pixels));alpha::SpriteDrawContext base{11,16,0,0,true},ctx=base;alpha::SpriteTrim original{9,72,72,40,100,62,14,4},trim=original;alpha::SpriteRect original_clip{0,0,128,128},clip=original_clip;std::memset(&out,0xa5,sizeof(out));unsigned char before[sizeof(out)];std::memcpy(before,&out,sizeof(out));unsigned count=0;
 auto reject=[&](alpha::SpriteDrawPlan* p,std::int32_t x,std::int32_t y,S expected){unsigned char a[sizeof(ctx)],b[sizeof(trim)],c[sizeof(clip)];std::memcpy(a,&ctx,sizeof(ctx));std::memcpy(b,&trim,sizeof(trim));std::memcpy(c,&clip,sizeof(clip));++count;return alpha::prepare_sprite_draw(ctx,trim,clip,x,y,p)==expected&&!std::memcmp(before,&out,sizeof(out))&&!std::memcmp(a,&ctx,sizeof(ctx))&&!std::memcmp(b,&trim,sizeof(trim))&&!std::memcmp(c,&clip,sizeof(clip));};
 ctx.source_context_supplied=false;if(!reject(&out,0,0,S::missing_context)){return 20;}
 for(unsigned k=0;k<15;++k){ctx=base;trim=original;clip=original_clip;switch(k){case 0:ctx.numerator=0;break;case 1:ctx.denominator=0;break;case 2:ctx.numerator=65;break;case 3:ctx.minimum_width=-1;break;case 4:ctx.minimum_height=16385;break;case 5:trim.pitch=71;break;case 6:trim.offset_x=-1;break;case 7:trim.offset_y=23;break;case 8:trim.width=0;break;case 9:trim.key=256;break;case 10:trim.full_width=85;break;case 11:clip.left=-1;break;case 12:clip.top=129;break;case 13:trim.width=16385;break;case 14:trim.offset_x=16385;break;}if(!reject(&out,0,0,S::invalid_input)){return 20;}}
 ctx=base;trim=original;clip=original_clip;if(!reject(nullptr,0,0,S::invalid_input)||!reject(&out,524288,0,S::invalid_input)||!reject(&out,0,-524288,S::invalid_input)){return 20;}
 const void* descriptions[]={&ctx,&trim,&clip};for(auto p:descriptions)for(unsigned k=0;k<3;++k){auto address=reinterpret_cast<std::uintptr_t>(p);if(k==1)++address;if(k==2)address-=sizeof(out)-1;if(!reject(reinterpret_cast<alpha::SpriteDrawPlan*>(address),0,0,S::invalid_input)){return 20;}}
 alpha::SpritePixelSource src{source,sizeof(source),72,40,72,9};alpha::SpritePixelSurface dst{pixels,sizeof(pixels),128,128,128};auto saved_dst=dst;
 const void* inputs[]={&ctx,&trim,&clip,&src,&dst,source,source+sizeof(source)-1};for(auto p:inputs)for(unsigned k=0;k<2;++k){auto address=reinterpret_cast<std::uintptr_t>(p);if(k)address-=sizeof(pixels)-1;dst.pixels=reinterpret_cast<unsigned char*>(address);unsigned char a[sizeof(ctx)],b[sizeof(trim)],c[sizeof(clip)],d[sizeof(src)],e[sizeof(dst)];std::memcpy(a,&ctx,sizeof(ctx));std::memcpy(b,&trim,sizeof(trim));std::memcpy(c,&clip,sizeof(clip));std::memcpy(d,&src,sizeof(src));std::memcpy(e,&dst,sizeof(dst));if(alpha::draw_sprite_ram(ctx,trim,clip,0,0,src,dst)!=S::invalid_input||std::memcmp(pixels,clean,sizeof(pixels))||std::memcmp(a,&ctx,sizeof(ctx))||std::memcmp(b,&trim,sizeof(trim))||std::memcmp(c,&clip,sizeof(clip))||std::memcmp(d,&src,sizeof(src))||std::memcmp(e,&dst,sizeof(dst))){return 20;}++count;dst=saved_dst;}
 if(alpha::prepare_sprite_draw(ctx,trim,clip,-10,-2,&out)!=S::ready||out.full.left!=0||out.full.top!=0||out.full.right!=49||out.full.bottom!=27){return 20;}++count;
 ctx.minimum_width=50;if(alpha::draw_sprite_ram(ctx,trim,clip,-10,-2,src,dst)!=S::minimum_size||std::memcmp(pixels,clean,sizeof(pixels))){return 20;}++count;
 ctx=base;if(alpha::draw_sprite_ram(ctx,trim,clip,200,200,src,dst)!=S::clipped||std::memcmp(pixels,clean,sizeof(pixels))){return 20;}++count;
 if(alpha::draw_sprite_ram(ctx,trim,clip,-10,-2,src,dst)!=S::ready){return 20;}++count;
 std::printf("%u placement/composition boundary controls\n",count);return 0;}
