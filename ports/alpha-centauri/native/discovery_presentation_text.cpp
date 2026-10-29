/* SPDX-License-Identifier: GPL-3.0-or-later. See discovery_presentation_text.h. */
#include "discovery_presentation_text.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
DiscoveryTextError discovery_presentation_text(const DiscoveryContinuationPlan& plan,const DiscoveryTextRuntime& runtime,DiscoveryPresentationText& out){using detail::effects_range_overlap;using detail::effects_array_overlap;
 const auto& state=plan.state();
 if(effects_range_overlap(&out,sizeof out,&plan,sizeof plan) || effects_range_overlap(&out,sizeof out,&runtime,sizeof runtime))return DiscoveryTextError::bad_state;
 if(state.ready() && (effects_range_overlap(&out,sizeof out,state.wire_baseline().data,state.wire_baseline().size) || effects_range_overlap(&out,sizeof out,state.monuments().data,state.monuments().size) || effects_array_overlap(&out,sizeof out,state.tile(0),state.map_info().tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,state.vehicle(0),state.vehicle_count(),sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,state.base(0),state.base_count(),sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,state.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,state.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,state.faction_definition(0),8,sizeof(OriginalFactionDefinition))))return DiscoveryTextError::bad_state;
 if(runtime.runtime!=DiscoveryRuntimeObservation::supplied)return DiscoveryTextError::runtime_required;
 if(!plan.ready() || !state.ready() || plan.faction()<1 || plan.faction()>7 || plan.map_owner()<0 || plan.map_owner()>7)return DiscoveryTextError::bad_state;
 const auto boundary=plan.boundary();if(boundary!=DiscoveryContinuationBoundary::reactor_presentation && boundary!=DiscoveryContinuationBoundary::first_secrets_presentation)return DiscoveryTextError::pending_kind_required;
 const auto* file_end=(const char*)memchr(runtime.script_file,0,256);if(!file_end || file_end==runtime.script_file)return DiscoveryTextError::bad_state;
 const char* text=boundary==DiscoveryContinuationBoundary::reactor_presentation?state.pending_reactor_name():plan.pending_secrets_technology_name();
 if(!text || !memchr(text,0,128))return DiscoveryTextError::text_held;
 DiscoveryPresentationText next;OriginalParseContext prepared{};
 OriginalParseTextInput request{DiscoveryRuntimeObservation::supplied,0,-1,-1,(const uint8_t*)text,128};
 if(original_parse_text(runtime.parser,request,prepared)!=OriginalParseError::ok)return DiscoveryTextError::text_held;
 next.parser_=prepared;
 if(boundary==DiscoveryContinuationBoundary::first_secrets_presentation){const auto* faction=state.faction_definition((size_t)plan.faction());if(!faction || !memchr(faction->adj_name_faction,0,128))return DiscoveryTextError::text_held;
  request.index=1;request.bytes=(const uint8_t*)faction->adj_name_faction;
  if(original_parse_text(next.parser_,request,prepared)!=OriginalParseError::ok)return DiscoveryTextError::text_held;
  next.parser_=prepared;const char* key=plan.map_owner()==plan.faction()?"THESECRET0":"THESECRET1";memcpy(next.popup_key_,key,11);
 }else memcpy(next.popup_key_,"REACTORUPGRADE",14);
 memcpy(next.script_file_,runtime.script_file,(size_t)(file_end-runtime.script_file)+1);next.faction_=plan.faction();next.technology_=plan.technology();next.boundary_=boundary;next.ready_=true;out=next;return DiscoveryTextError::ok;
}
}
