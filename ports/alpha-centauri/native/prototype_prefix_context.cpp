/* SPDX-License-Identifier: GPL-3.0-or-later. See prototype_prefix_context.h. */
#include "prototype_prefix_context.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
static bool overlap(const OriginalWorld& w,const PrototypePrefixComparison& out){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) || effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
PrototypePrefixError world_prototype_prefix_compare(const OriginalWorld& w,Variant v,const PrototypePrefixFact& fact,const uint8_t* term,size_t length,PrototypePrefixComparison& out){using detail::effects_range_overlap;
 if(effects_range_overlap(&out,sizeof out,&w,sizeof w) || effects_range_overlap(&out,sizeof out,&fact,sizeof fact) || effects_range_overlap(&out,sizeof out,term,length) || !w.loaded() || overlap(w,out) || !term || !length || length>256 || term[length-1] || memchr(term,0,length-1) || (v!=Variant::base && v!=Variant::alien_crossfire) || (w.game().data_version<11)!=(v==Variant::base))return PrototypePrefixError::bad_state;
 const auto* first=w.prototype(0);if(!first || !memchr(first->name,0,32))return PrototypePrefixError::bad_state;
 uint8_t prefix[84];const bool base=v==Variant::base;
 if(base){const auto table=w.game().bunglist;if(!table.data || table.size!=4096)return PrototypePrefixError::bad_state;memcpy(prefix,table.data+4044,52);}
 else{if(fact.observation!=PrototypePrefixObservation::supplied_bytes)return PrototypePrefixError::runtime_context_required;memcpy(prefix,fact.bytes,52);}
 memcpy(prefix+52,first->name,32);PrototypePrefixComparison next{};next.fromOwnedHistory=base;
 for(size_t i=0;i<sizeof prefix;++i){const uint8_t a=term[i<length?i:length-1],b=prefix[i];++next.logicalBytesCompared;if(a!=b){next.sign=a<b?-1:1;out=next;return PrototypePrefixError::ok;}if(!a){out=next;return PrototypePrefixError::ok;}}
 return PrototypePrefixError::bad_state;
}
}
