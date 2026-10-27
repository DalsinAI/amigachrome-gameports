// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_water_sprites.h"
#include <cstdio>
static bool rd(std::int32_t&v){unsigned char b[4];if(std::fread(b,1,4,stdin)!=4)return false;v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void wr(std::uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};std::fwrite(b,1,4,stdout);}
static const alpha::SpriteTrim trims[6]={{9,54,54,48,100,62,26,0},{9,48,48,33,100,62,28,9},{9,63,63,38,100,62,16,2},{9,56,56,39,100,62,21,1},{9,73,73,35,100,62,18,6},{9,62,62,41,100,62,12,2}};
static unsigned char images[6][6200],pixels[32768];
int main(){std::int32_t q[26];while(rd(q[0])){for(unsigned i=1;i<26;++i)if(!rd(q[i]))return 20;alpha::WaterItemSprites resources{};for(unsigned i=0;i<6;++i){const auto&t=trims[i];const auto n=t.width*t.height;if(std::fread(images[i],1,n,stdin)!=(unsigned)n)return 20;resources.item[i]={t,{images[i],(size_t)n,t.width,t.height,t.pitch,9},(q[24]&(1<<i))!=0};}const unsigned pitch=q[25]?256:128;if(std::fread(pixels,1,pitch*128,stdin)!=pitch*128)return 20;
 alpha::SeaLateContext c{q[0],q[1],q[2],q[3],q[4],q[5],(std::uint32_t)q[6],(std::uint32_t)q[7],(std::uint32_t)q[8],(std::uint32_t)q[9],q[10]!=0,q[11]!=0,q[12]!=0,q[13]!=0};alpha::WaterItemDrawRuntime in{q[14],q[15],q[16],q[17],{q[18],q[19],q[20],q[21]},q[22]!=0,q[23]!=0};alpha::SpritePixelSurface dst{pixels,pitch*128,128,128,(int32_t)pitch};auto status=alpha::render_water_item_sprites(c,in,resources,dst);wr((unsigned)status);std::fwrite(pixels,1,pitch*128,stdout);}return 0;}
