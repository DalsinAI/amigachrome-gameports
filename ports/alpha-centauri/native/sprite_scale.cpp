// SPDX-License-Identifier: GPL-3.0-or-later
#include "sprite_scale.h"
#include <cstddef>
#include <cstdint>
namespace alpha {
namespace {
bool overlaps(const void* a,std::size_t an,const void* b,std::size_t bn) {
 auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
 return x<=y ? y-x<an : x-y<bn;
}
}
bool prepare_sprite_scale(std::int32_t numerator,std::int32_t denominator,SpriteScaleTable* out) {
 if(!out||numerator<1||denominator<1||numerator>1024||denominator>1024)return false;
 auto high=numerator>denominator?numerator:denominator;
 auto low=numerator<denominator?numerator:denominator;
 SpriteScaleTable result{};
 const auto quotient=high/low;
 if(low==1) {for(auto& x:result.steps)x=quotient;}
 else {
  const auto advance=2*(high%low),period=2*low;
  auto error=high%low-period;
  if(quotient&1)error+=high;
  for(auto& x:result.steps) {error+=advance;x=quotient;if(error>0){error-=period;++x;}}
 }
 *out=result;return true;
}
bool prepare_sprite_axis(const SpriteScaleTable& table,bool expand,
 std::int32_t begin,std::int32_t end,std::int32_t clipped_begin,
 std::int32_t clipped_end,SpriteAxisPlan* out) {
 if(!out||overlaps(out,sizeof(*out),&table,sizeof(table)))return false;
 const std::int64_t offset=std::int64_t(clipped_begin)-begin;
 const std::int64_t length=std::int64_t(clipped_end)-clipped_begin;
 if(begin>end||clipped_begin<begin||clipped_end>end||length<=0||offset>819200||length>819200)return false;
 for(auto x:table.steps)if(x<1||x>1024)return false;
 SpriteAxisPlan result{};result.destination_offset=static_cast<std::int32_t>(offset);
 result.destination_span=static_cast<std::int32_t>(length);
 if(!expand) {
  if(offset+length>sprite_scale_capacity)return false;
  for(unsigned i=0;i<static_cast<unsigned>(offset);++i)result.source_offset+=table.steps[i];
  result.count=static_cast<unsigned>(length);
  for(unsigned i=0;i<result.count;++i){result.steps[i]=table.steps[static_cast<unsigned>(offset)+i];result.source_span+=result.steps[i];}
 } else {
  auto remaining=offset;unsigned cursor=0;
  while(remaining>0) {if(cursor==sprite_scale_capacity)return false;remaining-=table.steps[cursor++];}
  result.source_offset=static_cast<std::int32_t>(cursor);
  auto needed=length;
  if(remaining<0) {result.steps[result.count++]=static_cast<std::int32_t>(-remaining);needed+=remaining;--result.source_offset;}
  while(needed>0) {
   if(cursor==sprite_scale_capacity||result.count==sprite_scale_capacity)return false;
   auto step=table.steps[cursor++];result.steps[result.count++]=step;needed-=step;
  }
  if(needed<0)result.steps[result.count-1]+=static_cast<std::int32_t>(needed);
  result.source_span=static_cast<std::int32_t>(result.count);
 }
 *out=result;return true;
}
}
