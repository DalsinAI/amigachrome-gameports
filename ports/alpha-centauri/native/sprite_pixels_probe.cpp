// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_pixels.h"
#include <cstdio>
#include <cstdint>
static bool short_read=false;
static bool get(std::int32_t& v){unsigned char b[4];auto n=std::fread(b,1,4,stdin);if(n!=4){short_read=n!=0;return false;}v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static unsigned char source[144*64],pixels[144*128];
int main(){std::int32_t q[15];while(get(q[0])){for(unsigned i=1;i<15;++i)if(!get(q[i]))return 20;if(q[2]<1||q[2]>128||q[3]<1||q[3]>64||q[4]<q[2]||q[4]>144||q[5]<128||q[5]>144||q[6]<0||q[6]>255)return 20;auto sb=static_cast<unsigned>(q[4]*q[3]),db=static_cast<unsigned>(q[5]*128);if(std::fread(source,1,sb,stdin)!=sb||std::fread(pixels,1,db,stdin)!=db)return 20;alpha::SpriteScaleTable table{};alpha::SpritePixelCopyInput in{};in.expand=q[0]>q[1];in.destination_x=q[11];in.destination_y=q[12];if(!alpha::prepare_sprite_scale(q[0],q[1],&table)||!alpha::prepare_sprite_axis(table,in.expand,q[7],q[9],q[11],q[13],&in.x)||!alpha::prepare_sprite_axis(table,in.expand,q[8],q[10],q[12],q[14],&in.y))return 20;alpha::SpritePixelSource src{source,sb,q[2],q[3],q[4],static_cast<std::uint8_t>(q[6])};alpha::SpritePixelSurface dst{pixels,db,128,128,q[5]};if(!alpha::copy_sprite_pixels(in,src,dst))return 20;const unsigned char status[4]={0,0,0,0};std::fwrite(status,1,4,stdout);std::fwrite(pixels,1,db,stdout);}return short_read||std::ferror(stdin)?20:0;}
