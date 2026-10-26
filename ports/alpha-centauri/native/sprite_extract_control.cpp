// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_extract.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <climits>
static unsigned char source[64],pixels[64],clean[64];
int main(){using S=alpha::SpriteExtractStatus;for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)source[y*8+x]=(x>1&&x<6&&y>1&&y<6)?37:9;std::memset(pixels,199,sizeof(pixels));std::memcpy(clean,pixels,sizeof(pixels));alpha::SpritePixelSource base{source,sizeof(source),8,8,8,9};alpha::SpriteCrop crop{0,0,8,8},original_crop=crop;alpha::SpriteTrim out;std::memset(&out,0xa5,sizeof(out));unsigned char before[sizeof(out)],input[sizeof(source)];std::memcpy(before,&out,sizeof(out));std::memcpy(input,source,sizeof(source));auto src=base;unsigned count=0;
 auto reject=[&](alpha::SpriteTrim* result,unsigned char* destination,std::size_t bytes,S expected){unsigned char s[sizeof(src)],c[sizeof(crop)];std::memcpy(s,&src,sizeof(src));std::memcpy(c,&crop,sizeof(crop));auto status=alpha::extract_sprite_ram(src,crop,result,destination,bytes);++count;return status==expected&&!std::memcmp(&out,before,sizeof(out))&&!std::memcmp(source,input,sizeof(source))&&!std::memcmp(pixels,clean,sizeof(pixels))&&!std::memcmp(&src,s,sizeof(src))&&!std::memcmp(&crop,c,sizeof(crop));};
 for(unsigned k=0;k<13;++k){src=base;crop=original_crop;switch(k){case 0:src.pixels=nullptr;break;case 1:src.bytes=63;break;case 2:src.pitch=7;break;case 3:src.width=0;break;case 4:src.height=-1;break;case 5:crop.x=-1;break;case 6:crop.y=-1;break;case 7:crop.width=0;break;case 8:crop.height=-1;break;case 9:crop.x=INT_MAX;break;case 10:crop.width=9;break;case 11:crop.height=16385;break;case 12:crop.y=8;break;}if(!reject(&out,pixels,sizeof(pixels),S::invalid_input)){return 20;}}
 src=base;crop=original_crop;if(!reject(nullptr,pixels,sizeof(pixels),S::invalid_input)||!reject(&out,nullptr,0,S::insufficient_storage)||!reject(&out,pixels,15,S::insufficient_storage)){return 20;}
 const void* aliases[]={&src,&crop,source,source+63};for(auto p:aliases)for(unsigned k=0;k<3;++k){auto address=reinterpret_cast<std::uintptr_t>(p);if(k==1){if(p==source+63)--address;else ++address;}if(k==2)address-=sizeof(out)-1;if(!reject(reinterpret_cast<alpha::SpriteTrim*>(address),pixels,sizeof(pixels),S::invalid_input)){return 20;}}
 for(auto p:aliases)for(unsigned k=0;k<2;++k){auto address=reinterpret_cast<std::uintptr_t>(p);if(k)address-=63;if(!reject(&out,reinterpret_cast<unsigned char*>(address),64,S::invalid_input)){return 20;}}
 if(!reject(&out,reinterpret_cast<unsigned char*>(&out),64,S::invalid_input)||!reject(reinterpret_cast<alpha::SpriteTrim*>(pixels+63),pixels,64,S::invalid_input)){return 20;}
 if(alpha::extract_sprite_ram(src,crop,&out,pixels,sizeof(pixels))!=S::ok||out.width!=4||out.height!=4||out.pitch!=4||out.offset_x!=2||out.offset_y!=2){return 20;}++count;
 for(unsigned i=0;i<16;++i)if(pixels[i]!=37){return 20;}
 for(unsigned i=16;i<64;++i)if(pixels[i]!=199){return 20;}
 std::memset(source,9,sizeof(source));std::memset(pixels,199,sizeof(pixels));if(alpha::extract_sprite_ram(src,crop,&out,pixels,0)!=S::empty||out.width||out.height||out.pitch||out.offset_x||out.offset_y!=8||std::memcmp(pixels,clean,sizeof(pixels))){return 20;}++count;
 std::printf("%u extraction validation/alias/empty controls\n",count);return 0;}
