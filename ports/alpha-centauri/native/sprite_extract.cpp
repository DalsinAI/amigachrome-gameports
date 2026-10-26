// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_extract.h"
#include <cstdint>
#include <cstring>
namespace alpha { namespace {
bool overlaps(const void* a,std::size_t an,const void* b,std::size_t bn){if(!an||!bn)return false;auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);return x<=y?y-x<an:x-y<bn;}
}
SpriteExtractStatus extract_sprite_ram(const SpritePixelSource& src,const SpriteCrop& crop,
 SpriteTrim* out,std::uint8_t* pixels,std::size_t capacity){
 using S=SpriteExtractStatus;
 if(!out||!src.pixels||src.width<1||src.height<1||src.pitch<src.width||std::uint64_t(src.pitch)*src.height>src.bytes||crop.x<0||crop.y<0||crop.width<1||crop.height<1||crop.width>16384||crop.height>16384||std::int64_t(crop.x)+crop.width>src.width||std::int64_t(crop.y)+crop.height>src.height||std::uint64_t(crop.width)*crop.height>64u*1024u*1024u)return S::invalid_input;
 if(overlaps(out,sizeof(*out),&src,sizeof(src))||overlaps(out,sizeof(*out),&crop,sizeof(crop))||overlaps(out,sizeof(*out),src.pixels,src.bytes)||(pixels&&overlaps(out,sizeof(*out),pixels,capacity)))return S::invalid_input;
 if(pixels&&(overlaps(pixels,capacity,&src,sizeof(src))||overlaps(pixels,capacity,&crop,sizeof(crop))||overlaps(pixels,capacity,src.pixels,src.bytes)))return S::invalid_input;
 auto left=crop.width,top=crop.height,right=0,bottom=0;
 for(std::int32_t y=0;y<crop.height;++y)for(std::int32_t x=0;x<crop.width;++x){
  if(src.pixels[static_cast<std::size_t>(crop.y+y)*src.pitch+crop.x+x]==src.key)continue;
  if(x<left)left=x;
  if(y<top)top=y;
  if(x>=right)right=x+1;
  if(y>=bottom)bottom=y+1;
 }
 SpriteTrim result{src.key,0,0,0,crop.width,crop.height,0,crop.height};
 if(right==0){*out=result;return S::empty;}
 result.width=right-left;result.height=bottom-top;result.pitch=result.width;result.offset_x=left;result.offset_y=top;
 const auto need=static_cast<std::size_t>(result.width)*result.height;
 if(!pixels||need>capacity)return S::insufficient_storage;
 for(std::int32_t y=0;y<result.height;++y)std::memcpy(pixels+static_cast<std::size_t>(y)*result.width,src.pixels+static_cast<std::size_t>(crop.y+top+y)*src.pitch+crop.x+left,result.width);
 *out=result;return S::ok;
}
}
