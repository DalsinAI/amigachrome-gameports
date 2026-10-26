// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_extract.h"
#include <cstdio>
#include <cstdint>
static bool partial=false;
static bool get(std::int32_t& v){unsigned char b[4];auto n=std::fread(b,1,4,stdin);if(n!=4){partial=n!=0;return false;}v=static_cast<std::int32_t>(std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24));return true;}
static void put(std::uint32_t x){const unsigned char b[4]={static_cast<unsigned char>(x),static_cast<unsigned char>(x>>8),static_cast<unsigned char>(x>>16),static_cast<unsigned char>(x>>24)};std::fwrite(b,1,4,stdout);}
static unsigned char image[1100*800],pixels[65536];
int main(){std::int32_t q[8];while(get(q[0])){for(unsigned i=1;i<8;++i)if(!get(q[i]))return 20;if(q[0]<1||q[0]>1024||q[1]<1||q[1]>768||q[2]<q[0]||q[2]>1100||q[3]<0||q[3]>255||q[6]<1||q[7]<1||std::int64_t(q[6])*q[7]>65536)return 20;auto n=static_cast<unsigned>(q[2]*q[1]),cap=static_cast<unsigned>(q[6]*q[7]);if(std::fread(image,1,n,stdin)!=n)return 20;for(auto&x:pixels)x=199;alpha::SpritePixelSource src{image,n,q[0],q[1],q[2],static_cast<std::uint8_t>(q[3])};alpha::SpriteCrop crop{q[4],q[5],q[6],q[7]};alpha::SpriteTrim out{};auto status=alpha::extract_sprite_ram(src,crop,&out,pixels,cap);if(status!=alpha::SpriteExtractStatus::ok&&status!=alpha::SpriteExtractStatus::empty)return 20;put(static_cast<std::uint32_t>(status));const std::int32_t values[]={out.key,out.pitch,out.width,out.height,out.full_width,out.full_height,out.offset_x,out.offset_y};for(auto x:values)put(static_cast<std::uint32_t>(x));std::fwrite(pixels,1,cap,stdout);}return partial||std::ferror(stdin)?20:0;}
