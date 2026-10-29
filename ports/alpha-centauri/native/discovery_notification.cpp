/* SPDX-License-Identifier: GPL-3.0-or-later. */
#include "discovery_notification.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
DiscoveryNotificationError discovery_notification(const DiscoveryPresentationText& prepared,const DiscoveryScriptInput& in,DiscoveryNotification& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&prepared,sizeof prepared)||effects_range_overlap(&out,sizeof out,&in,sizeof in)||(in.size&&(!in.bytes||effects_range_overlap(&out,sizeof out,in.bytes,in.size))))return DiscoveryNotificationError::bad_state;
 if(in.runtime!=DiscoveryRuntimeObservation::supplied)return DiscoveryNotificationError::runtime_required;
 if(!prepared.ready()||!memchr(in.file,0,sizeof in.file)||strcmp(in.file,prepared.script_file())||!in.bytes||!in.size||in.size>4u*1024u*1024u)return DiscoveryNotificationError::bad_state;
 const char* key=prepared.popup_key();if(strcmp(key,"THESECRET0")&&strcmp(key,"THESECRET1")&&strcmp(key,"REACTORUPGRADE"))return DiscoveryNotificationError::resource_held;
 DiscoveryNotification next;bool found=false,selected=false,caption=false,width=false,body=false,ended=false;
 const char*p=(const char*)in.bytes;const char*end=p+in.size;
 while(p<end){const char*start=p;while(p<end&&*p!='\r'&&*p!='\n'){if(!*p)return DiscoveryNotificationError::resource_held;++p;}const size_t n=(size_t)(p-start);if(p<end&&*p=='\r')++p;if(p<end&&*p=='\n')++p;
  const bool section=n>1&&start[0]=='#'&&!memchr(start,' ',n)&&!memchr(start,'\t',n);
  if(section){const bool match=n==strlen(key)+1&&!memcmp(start+1,key,n-1);if(match&&found)return DiscoveryNotificationError::resource_held;selected=match;if(match){found=true;ended=false;}continue;}
  if(!selected)continue;
  if(!n){if(body)ended=true;continue;}
  if(ended)return DiscoveryNotificationError::resource_held;
  if(start[0]=='#'){
   if(body)return DiscoveryNotificationError::resource_held;
   if(n>4&&!memcmp(start,"#xs ",4)){if(width)return DiscoveryNotificationError::resource_held;uint32_t v=0;for(size_t j=4;j<n;++j){if(start[j]<'0'||start[j]>'9'||v>409)return DiscoveryNotificationError::resource_held;v=v*10+(unsigned)(start[j]-'0');}if(!v||v>4096)return DiscoveryNotificationError::resource_held;next.width_=v;width=true;}
   else if(n>=9&&!memcmp(start,"#caption ",9)){if(caption||n-9>=8192)return DiscoveryNotificationError::resource_held;char text[8192];memcpy(text,start+9,n-9);text[n-9]=0;OriginalExpandedText expanded;const OriginalTemplateInput q{in.runtime,in.language_mode,(const uint8_t*)text,n-8};if(original_expand_template(prepared.parser(),q,expanded)!=OriginalTemplateError::ok)return DiscoveryNotificationError::template_held;if(expanded.length+1>sizeof next.storage_)return DiscoveryNotificationError::overlong;memcpy(next.storage_,expanded.bytes,expanded.length+1);next.used_=expanded.length+1;caption=true;}
   else return DiscoveryNotificationError::resource_held;
   continue;
  }
  if(!caption||!width)return DiscoveryNotificationError::resource_held;
  if(n>=8192||next.count_==64)return DiscoveryNotificationError::overlong;
  char text[8192];memcpy(text,start,n);text[n]=0;OriginalExpandedText expanded;const OriginalTemplateInput q{in.runtime,in.language_mode,(const uint8_t*)text,n+1};if(original_expand_template(prepared.parser(),q,expanded)!=OriginalTemplateError::ok)return DiscoveryNotificationError::template_held;
  size_t skip=0;uint8_t style=0;if(expanded.length&&expanded.bytes[0]=='^'){skip=1;style=expanded.length==1?3:1;if(expanded.length>1&&expanded.bytes[1]=='^'){skip=expanded.length>2&&expanded.bytes[2]=='^'?3:2;style=2;}}
  const size_t length=expanded.length-skip;if(length+1>sizeof next.storage_-next.used_)return DiscoveryNotificationError::overlong;
  auto& line=next.lines_[next.count_++];line.offset=(uint32_t)next.used_;line.length=(uint32_t)length;line.original_style=style;memcpy(next.storage_+next.used_,expanded.bytes+skip,length+1);next.used_+=length+1;body=true;
 }
 if(!found)return DiscoveryNotificationError::resource_missing;
 if(!caption||!width||!body)return DiscoveryNotificationError::resource_held;
 next.faction_=prepared.faction();next.technology_=prepared.technology();next.boundary_=prepared.source_boundary();next.ready_=true;out=next;return DiscoveryNotificationError::ok;
}
}
