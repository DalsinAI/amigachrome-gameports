// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_pixels.h"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
static unsigned char source[64],pixels[256],clean[256],large_source[8*512];
int main(){unsigned count=0;for(unsigned i=0;i<64;++i)source[i]=i%3?static_cast<unsigned char>(i):9;std::memset(pixels,199,sizeof(pixels));std::memcpy(clean,pixels,sizeof(pixels));alpha::SpritePixelCopyInput base{};base.expand=true;base.destination_x=1;base.destination_y=1;base.x.source_offset=1;base.x.destination_offset=1;base.x.source_span=2;base.x.destination_span=3;base.x.count=2;base.x.steps[0]=1;base.x.steps[1]=2;base.y=base.x;auto in=base;alpha::SpritePixelSource src{source,sizeof(source),8,8,8,9};alpha::SpritePixelSurface dst{pixels,sizeof(pixels),16,16,16};auto original_src=src;auto original_dst=dst;unsigned char original_pixels[64];std::memcpy(original_pixels,source,sizeof(source));
 auto reject=[&](){unsigned char before_in[sizeof(in)],before_src[sizeof(src)],before_dst[sizeof(dst)];std::memcpy(before_in,&in,sizeof(in));std::memcpy(before_src,&src,sizeof(src));std::memcpy(before_dst,&dst,sizeof(dst));bool ok=!alpha::copy_sprite_pixels(in,src,dst)&&!std::memcmp(pixels,clean,sizeof(pixels))&&!std::memcmp(source,original_pixels,sizeof(source))&&!std::memcmp(&in,before_in,sizeof(in))&&!std::memcmp(&src,before_src,sizeof(src))&&!std::memcmp(&dst,before_dst,sizeof(dst));++count;return ok;};
 for(unsigned k=0;k<16;++k){in=base;switch(k){case 0:in.x.count=0;break;case 1:in.x.count=801;break;case 2:in.x.steps[1]=0;in.x.destination_span=1;break;case 3:in.x.steps[1]=5;in.x.destination_span=6;break;case 4:in.y.steps[1]=5;in.y.destination_span=6;break;case 5:in.x.source_offset=-1;break;case 6:in.y.source_offset=8;break;case 7:in.x.source_span=3;break;case 8:in.y.destination_span=4;break;case 9:in.destination_x=-1;break;case 10:in.destination_y=14;break;case 11:in.x.destination_offset=-1;break;case 12:in.y.source_span=0;break;case 13:in.expand=false;break;case 14:in.x.steps[0]=-1;in.x.destination_span=1;break;case 15:in.y.count=0;break;}if(!reject())return 20;}
 in=base;
 for(unsigned k=0;k<8;++k){src=original_src;dst=original_dst;switch(k){case 0:src.pixels=nullptr;break;case 1:dst.pixels=nullptr;break;case 2:src.bytes=63;break;case 3:dst.bytes=255;break;case 4:src.pitch=7;break;case 5:dst.pitch=15;break;case 6:src.width=0;break;case 7:dst.height=-1;break;}if(!reject())return 20;}
 src=original_src;dst=original_dst;
 for(const void* p:{static_cast<const void*>(&in),static_cast<const void*>(&src),static_cast<const void*>(&dst),static_cast<const void*>(source),static_cast<const void*>(source+63)})for(unsigned k=0;k<2;++k){dst=original_dst;auto address=reinterpret_cast<std::uintptr_t>(p);if(k)address-=255;dst.pixels=reinterpret_cast<unsigned char*>(address);if(!reject())return 20;}
 dst=original_dst;in=base;in.expand=false;in.x.source_span=3;in.x.destination_span=2;in.y=in.x;in.y.steps[1]=256;in.y.source_span=257;src={large_source,sizeof(large_source),8,512,8,9};if(!reject())return 20;
 in=base;src=original_src;dst=original_dst;if(!alpha::copy_sprite_pixels(in,src,dst))return 20;++count;
 std::printf("%u pixel-copy validation and no-write controls\n",count);return 0;}
