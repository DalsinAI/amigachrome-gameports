/* SPDX-License-Identifier: GPL-3.0-or-later. See original_parse_context.h. */
#include "original_parse_context.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
OriginalParseError original_parse_text(const OriginalParseContext& in,const OriginalParseTextInput& request,OriginalParseContext& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&in,sizeof in) || effects_range_overlap(&out,sizeof out,&request,sizeof request) || (request.size && (!request.bytes || effects_range_overlap(&out,sizeof out,request.bytes,request.size))))return OriginalParseError::bad_state;
 if(request.runtime!=DiscoveryRuntimeObservation::supplied)return OriginalParseError::runtime_required;
 if(request.index<0 || request.index>9)return OriginalParseError::bad_index;
 if(!request.bytes || !request.size)return OriginalParseError::bad_text;
 const size_t bounded=request.size<256?request.size:256;const auto* end=(const uint8_t*)memchr(request.bytes,0,bounded);
 if(!end)return request.size>=256?OriginalParseError::overlong:OriginalParseError::bad_text;
 const size_t length=(size_t)(end-request.bytes);OriginalParseContext next=in;
 next.genders[request.index]=request.gender<0?in.default_gender:request.gender;
 next.plurals[request.index]=request.plural<0?in.default_plural:request.plural;
 memcpy(next.strings[request.index],request.bytes,length+1);out=next;return OriginalParseError::ok;
}
}
