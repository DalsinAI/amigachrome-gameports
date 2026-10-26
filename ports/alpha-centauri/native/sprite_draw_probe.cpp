// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_draw.h"
#include <cstdio>
#include <cstdint>
static bool partial=false;
static bool get(std::int32_t& v){unsigned char b[4];auto n=std::fread(b,1,4,stdin);if(n!=4){partial=n!=0;return false;}v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void put(std::uint32_t x){const unsigned char b[4]={static_cast<unsigned char>(x),static_cast<unsigned char>(x>>8),static_cast<unsigned char>(x>>16),static_cast<unsigned char>(x>>24)};std::fwrite(b,1,4,stdout);}
static unsigned char source[65536],pixels[16384];
int main(){std::int32_t q[16];while(get(q[0])){for(unsigned k=1;k<16;++k)if(!get(q[k]))return 20;if(q[2]<1||q[3]<1||std::int64_t(q[2])*q[3]>65536)return 20;auto sb=static_cast<unsigned>(q[2]*q[3]);if(std::fread(source,1,sb,stdin)!=sb||std::fread(pixels,1,sizeof(pixels),stdin)!=sizeof(pixels))return 20;alpha::SpriteDrawContext ctx{q[0],q[1],q[14],q[15],true};alpha::SpriteTrim trim{9,q[2],q[2],q[3],q[6],q[7],q[4],q[5]};alpha::SpriteRect clip{q[10],q[11],q[12],q[13]};alpha::SpriteDrawPlan plan{};auto status=alpha::prepare_sprite_draw(ctx,trim,clip,q[8],q[9],&plan);alpha::SpritePixelSource src{source,sb,q[2],q[3],q[2],9};alpha::SpritePixelSurface dst{pixels,sizeof(pixels),128,128,128};if(alpha::draw_sprite_ram(ctx,trim,clip,q[8],q[9],src,dst)!=status)return 20;put(static_cast<unsigned>(status));const auto f=plan.full,c=plan.intersection;const std::int32_t words[]={f.left,f.top,f.right,f.bottom,c.left,c.top,c.right,c.bottom};for(auto x:words)put(static_cast<std::uint32_t>(x));std::fwrite(pixels,1,sizeof(pixels),stdout);}return partial||std::ferror(stdin)?20:0;}
