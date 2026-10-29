/* SPDX-License-Identifier: GPL-3.0-or-later. */
#include "notification_layout.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
NotificationLayoutError notification_layout(const DiscoveryNotification& notice,const NotificationLayoutInput& in,NotificationLayout& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&notice,sizeof notice)||effects_range_overlap(&out,sizeof out,&in,sizeof in))return NotificationLayoutError::bad_state;
 if(in.runtime!=DiscoveryRuntimeObservation::supplied)return NotificationLayoutError::runtime_required;
 if(!notice.ready()||!notice.pending())return NotificationLayoutError::bad_state;
 if(in.width<1||in.width>4096||in.max_height<1||in.max_height>4096||in.caption_top<0||in.body_top<0||in.caption_top>=in.max_height||in.body_top>=in.max_height)return NotificationLayoutError::geometry_held;
 const NotificationFontMetrics* fonts[]={&in.body,&in.caption};
 for(const auto*font:fonts){if(font->ascent<1||font->ascent>font->line_height||font->line_height>128)return NotificationLayoutError::metrics_held;for(unsigned i=0;i<256;++i)if(font->advance[i]<-1||font->advance[i]>128*64)return NotificationLayoutError::metrics_held;}
 NotificationLayout next;const int32_t max=in.width*64;
 auto width=[&](const char*p,size_t n,const NotificationFontMetrics& font,int32_t& result){result=0;for(size_t i=0;i<n;++i){const int32_t a=font.advance[(uint8_t)p[i]];if(a<0)return NotificationLayoutError::glyph_held;if(result>max-a)return NotificationLayoutError::geometry_held;result+=a;}return NotificationLayoutError::ok;};
 auto put=[&](const char*p,size_t n,int32_t x,int32_t top,bool caption){const auto&font=caption?in.caption:in.body;if(top<0||top>in.max_height-font.line_height)return NotificationLayoutError::geometry_held;if(n>2048-next.count_)return NotificationLayoutError::capacity;for(size_t i=0;i<n;++i){const auto a=font.advance[(uint8_t)p[i]];if(a<0)return NotificationLayoutError::glyph_held;auto&g=next.glyphs_[next.count_++];g.x26=x;g.baseline=top+font.ascent;g.byte=(uint8_t)p[i];g.caption=caption;x+=a;}const int32_t bottom=top+font.line_height;if(next.height_<bottom)next.height_=bottom;return NotificationLayoutError::ok;};
 const char* rawCaption=notice.caption();const size_t length=strlen(rawCaption);if(length>=8192)return NotificationLayoutError::capacity;char caption[8192];for(size_t i=0;i<length;++i){const auto c=(uint8_t)rawCaption[i];if(c>=128)return NotificationLayoutError::glyph_held;caption[i]=c>='a'&&c<='z'?(char)(c-'a'+'A'):(char)c;}caption[length]=0;int32_t measured;auto error=width(caption,length,in.caption,measured);if(error!=NotificationLayoutError::ok)return error;if((error=put(caption,length,(max-measured)/2,in.caption_top,true))!=NotificationLayoutError::ok)return error;
 char paragraph[16384];size_t used=0;int32_t y=in.body_top;
 auto flush=[&](bool centered){size_t pos=0;while(pos<used){while(pos<used&&paragraph[pos]==' ')++pos;if(pos==used)break;size_t end=pos,lastSpace=pos;int32_t w=0;while(end<used){const int32_t a=in.body.advance[(uint8_t)paragraph[end]];if(a<0)return NotificationLayoutError::glyph_held;if(w>max-a)break;w+=a;if(paragraph[end]==' ')lastSpace=end;++end;}
   size_t stop=end,nextPos=end;if(end<used){if(lastSpace==pos)return NotificationLayoutError::geometry_held;stop=lastSpace;nextPos=lastSpace+1;}
   while(stop>pos&&paragraph[stop-1]==' ')--stop;
   int32_t actual;if((error=width(paragraph+pos,stop-pos,in.body,actual))!=NotificationLayoutError::ok)return error;if((error=put(paragraph+pos,stop-pos,centered?(max-actual)/2:0,y,false))!=NotificationLayoutError::ok)return error;y+=in.body.line_height;pos=nextPos;}
  used=0;return NotificationLayoutError::ok;};
 for(size_t i=0;i<notice.line_count();++i){const auto* line=notice.line(i);const char* text=notice.line_text(i);if(!line||!text||line->original_style>3)return NotificationLayoutError::bad_state;
  if(line->original_style){if((error=flush(false))!=NotificationLayoutError::ok)return error;}
  if(line->original_style==3){if(y>in.max_height-in.body.line_height)return NotificationLayoutError::geometry_held;y+=in.body.line_height;continue;}
  if(line->length+used+(used?1u:0u)>sizeof paragraph)return NotificationLayoutError::capacity;
  if(used)paragraph[used++]=' ';
  memcpy(paragraph+used,text,line->length);used+=line->length;
  if(line->original_style==2&&((error=flush(true))!=NotificationLayoutError::ok))return error;
 }
 if((error=flush(false))!=NotificationLayoutError::ok)return error;
 if(next.height_<y)next.height_=y;
 next.faction_=notice.faction();next.technology_=notice.technology();next.boundary_=notice.source_boundary();next.ready_=true;out=next;return NotificationLayoutError::ok;
}
}
