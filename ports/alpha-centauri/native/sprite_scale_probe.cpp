// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_scale.h"
#include <cstdio>
#include <cstdint>
static bool short_read=false;
static bool read32(std::int32_t& v){unsigned char b[4];auto n=std::fread(b,1,4,stdin);if(n!=4){short_read=n!=0;return false;}v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void write32(std::int32_t v){auto x=static_cast<std::uint32_t>(v);unsigned char b[4]={static_cast<unsigned char>(x),static_cast<unsigned char>(x>>8),static_cast<unsigned char>(x>>16),static_cast<unsigned char>(x>>24)};std::fwrite(b,1,4,stdout);}
int main(){std::int32_t q[7];while(read32(q[0])){for(unsigned i=1;i<7;++i)if(!read32(q[i]))return 20;alpha::SpriteScaleTable table{};alpha::SpriteAxisPlan plan{};if(q[2]<0||q[2]>3||!alpha::prepare_sprite_scale(q[0],q[1],&table)||!alpha::prepare_sprite_axis(table,q[2]%2==0,q[3],q[4],q[5],q[6],&plan))return 20;for(auto x:table.steps)write32(x);write32(plan.source_offset);write32(plan.destination_offset);write32(plan.source_span);write32(plan.destination_span);write32(static_cast<std::int32_t>(plan.count));for(auto x:plan.steps)write32(x);}return short_read||std::ferror(stdin)?20:0;}
