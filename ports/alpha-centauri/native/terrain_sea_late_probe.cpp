// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_late.h"
#include <cstdio>
static bool rd(std::int32_t&v){unsigned char b[4];if(std::fread(b,1,4,stdin)!=4)return false;v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void wr(std::uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};std::fwrite(b,1,4,stdout);}
int main(){std::int32_t q[14];while(rd(q[0])){for(unsigned i=1;i<14;++i)if(!rd(q[i]))return 20;alpha::SeaLateContext c{q[0],q[1],q[2],q[3],q[4],q[5],(std::uint32_t)q[6],(std::uint32_t)q[7],(std::uint32_t)q[8],(std::uint32_t)q[9],q[10]!=0,q[11]!=0,q[12]!=0,q[13]!=0};alpha::SeaLatePlan p{};wr((unsigned)alpha::inspect_sea_late(c,&p));wr(p.count);for(auto a:p.crop_arguments)wr(a);for(auto e:p.events){wr((unsigned)e.operation);wr(e.variant);}}return 0;}
