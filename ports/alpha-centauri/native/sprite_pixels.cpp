// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_pixels.h"
#include <cstdint>
namespace alpha { namespace {
bool overlap(const void* a,std::size_t an,const void* b,std::size_t bn){auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);return x<=y?y-x<an:x-y<bn;}
bool valid_axis(const SpriteAxisPlan& p,bool expand,bool vertical) {
 if(p.source_offset<0||p.destination_offset<0||p.source_span<1||p.destination_span<1||p.count<1||p.count>sprite_scale_capacity)return false;
 std::int64_t sum=0;
 for(unsigned k=0;k<p.count;++k){auto v=p.steps[k];if(v<1||v>(expand?4:(vertical?255:1024)))return false;sum+=v;}
 return expand?(p.source_span==static_cast<std::int32_t>(p.count)&&sum==p.destination_span):(p.destination_span==static_cast<std::int32_t>(p.count)&&sum==p.source_span);
}
}
bool copy_sprite_pixels(const SpritePixelCopyInput& in,const SpritePixelSource& src,const SpritePixelSurface& dst) {
 if(!src.pixels||!dst.pixels||src.width<1||src.height<1||dst.width<1||dst.height<1||src.pitch<src.width||dst.pitch<dst.width||in.destination_x<0||in.destination_y<0)return false;
 const std::uint64_t sb=std::uint64_t(src.pitch)*src.height,db=std::uint64_t(dst.pitch)*dst.height;
 if(sb>src.bytes||db>dst.bytes||!valid_axis(in.x,in.expand,false)||!valid_axis(in.y,in.expand,true))return false;
 if(std::int64_t(in.x.source_offset)+in.x.source_span>src.width||std::int64_t(in.y.source_offset)+in.y.source_span>src.height||std::int64_t(in.destination_x)+in.x.destination_span>dst.width||std::int64_t(in.destination_y)+in.y.destination_span>dst.height)return false;
 if(overlap(dst.pixels,dst.bytes,&in,sizeof(in))||overlap(dst.pixels,dst.bytes,&src,sizeof(src))||overlap(dst.pixels,dst.bytes,&dst,sizeof(dst))||overlap(dst.pixels,dst.bytes,src.pixels,src.bytes))return false;
 auto sy=in.y.source_offset;auto dy=in.destination_y;
 for(unsigned y=0;y<in.y.count;++y){
  auto sx=in.x.source_offset;auto dx=in.destination_x;
  const auto rows=in.expand?in.y.steps[y]:1;
  for(unsigned x=0;x<in.x.count;++x){
   auto c=src.pixels[static_cast<std::size_t>(sy)*src.pitch+sx];const auto cols=in.expand?in.x.steps[x]:1;
   if(c!=src.key)for(int row=0;row<rows;++row)for(int col=0;col<cols;++col)dst.pixels[static_cast<std::size_t>(dy+row)*dst.pitch+dx+col]=c;
   sx+=in.expand?1:in.x.steps[x];dx+=cols;
  }
  sy+=in.expand?1:in.y.steps[y];dy+=rows;
 }
 return true;
}
}
