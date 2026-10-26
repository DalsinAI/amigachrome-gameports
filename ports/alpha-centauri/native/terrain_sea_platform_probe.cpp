// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_platform.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
static bool read_i(std::int32_t&v){unsigned char b[4];if(std::fread(b,1,4,stdin)!=4)return false;v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void write_i(std::uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};std::fwrite(b,1,4,stdout);}
int main(){std::int32_t q[15];unsigned char source[2880],pixels[16384];while(read_i(q[0])){for(unsigned i=1;i<15;++i)if(!read_i(q[i]))return 20;if(std::fread(source,1,sizeof(source),stdin)!=sizeof(source)||std::fread(pixels,1,sizeof(pixels),stdin)!=sizeof(pixels))return 20;
 alpha::TerrainSeaPlatformContext c{q[0],q[1],q[2],q[3],q[4],(std::uint32_t)q[5],(std::uint32_t)q[6],(std::uint32_t)q[7],q[8]!=0,q[9]!=0,q[10]!=0};alpha::SpriteRect clip{q[11],q[12],q[13],q[14]};alpha::SpriteDrawPlan plan{};auto a=alpha::prepare_terrain_sea_platform(c,clip,&plan);alpha::SpritePixelSource src{source,sizeof(source),72,40,72,9};alpha::SpritePixelSurface dst{pixels,sizeof(pixels),128,128,128};auto b=alpha::draw_terrain_sea_platform(c,clip,src,dst);if(a!=b)return 20;write_i((std::uint32_t)a);for(auto v:{plan.full.left,plan.full.top,plan.full.right,plan.full.bottom,plan.intersection.left,plan.intersection.top,plan.intersection.right,plan.intersection.bottom})write_i(v);std::fwrite(pixels,1,sizeof(pixels),stdout);}return 0;}
