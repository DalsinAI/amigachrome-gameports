/* SPDX-License-Identifier: GPL-3.0-or-later. See proposer_record_stages.h. */
#include "proposer_record_stages.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha {
using detail::effects_range_overlap;
static bool overlap(const OriginalWorld& w,const OriginalPrototype& out){using detail::effects_array_overlap;return effects_range_overlap(&out,sizeof out,&w,sizeof w) || w.overlaps_byte_storage(&out,sizeof out) || effects_array_overlap(&out,sizeof out,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(&out,sizeof out,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(&out,sizeof out,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(&out,sizeof out,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(&out,sizeof out,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(&out,sizeof out,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
ProposerRecordError prepare_proposer_record(const OriginalPrototype& prior,const OriginalPrototype& constructed,const ProposerPreparationFacts& f,OriginalPrototype& out){
 if(effects_range_overlap(&out,sizeof out,&prior,sizeof prior) || effects_range_overlap(&out,sizeof out,&constructed,sizeof constructed) || effects_range_overlap(&out,sizeof out,&f,sizeof f))return ProposerRecordError::bad_state;
 OriginalPrototype next=constructed;
 if(f.requested_group==-2 || f.difficulty<2)next.unit_flags|=4;
 if(!(prior.unit_flags&1))next.unit_flags|=0x40;
 next.unit_flags|=0x80;
 if(f.requested_group>=0){const uint32_t raw=(uint32_t)f.requested_group&255u;next.group_id=(int8_t)(raw<128?int32_t(raw):int32_t(raw)-256);if(f.requested_group==10)next.plan=3;}
 out=next;return ProposerRecordError::ok;
}
ProposerRecordError store_proposer_automatic_name(const OriginalPrototype& prepared,const PrototypeLogicalName& name,OriginalPrototype& out){
 if(effects_range_overlap(&out,sizeof out,&prepared,sizeof prepared) || effects_range_overlap(&out,sizeof out,&name,sizeof name))return ProposerRecordError::bad_state;
 const char* end=(const char*)memchr(name.bytes,0,sizeof name.bytes);if(!end)return ProposerRecordError::bad_state;
 const size_t length=(size_t)(end-name.bytes);OriginalPrototype next=prepared;memset(next.name,0,32);memcpy(next.name,name.bytes,length<31?length:31);next.unit_flags&=(uint16_t)~2u;out=next;return ProposerRecordError::ok;
}
static bool valid(const OriginalWorld& w,Variant v){return w.loaded() && (v==Variant::base || v==Variant::alien_crossfire) && ((w.game().data_version<11)==(v==Variant::base));}
ProposerRecordError world_prepare_proposer_record(const OriginalWorld& w,Variant v,int32_t id,const OriginalPrototype& constructed,const ProposerPreparationFacts& f,OriginalPrototype& out){if(!valid(w,v) || id<64 || id>=512 || overlap(w,out))return ProposerRecordError::bad_state;const auto* prior=w.prototype((uint32_t)id);if(!prior)return ProposerRecordError::bad_state;return prepare_proposer_record(*prior,constructed,f,out);}
ProposerRecordError world_store_proposer_automatic_name(const OriginalWorld& w,Variant v,const OriginalPrototype& prepared,const PrototypeLogicalName& name,OriginalPrototype& out){if(!valid(w,v) || overlap(w,out))return ProposerRecordError::bad_state;return store_proposer_automatic_name(prepared,name,out);}
}
