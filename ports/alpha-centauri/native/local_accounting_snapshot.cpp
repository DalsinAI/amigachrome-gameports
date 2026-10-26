/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original wire schemas from Thinker, MIT. Copyright (c) Thinker Mod authors;
 * see THINKER-LICENSE.txt and the pinned schemas. Independently written encoder.
 */
#include "local_accounting_snapshot.h"
#include "vehicle_stack.h"
#include <stdint.h>
#include <string.h>
namespace alpha { namespace {
template<class T> bool same_array(const T* a,const T* b,size_t n){for(size_t i=0;i<n;++i)if(a[i]!=b[i])return false;return true;}
bool overlaps(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(n>UINTPTR_MAX-x || m>UINTPTR_MAX-y)return true;
 return x<y+m && y<x+n;
}
bool disjoint(const OriginalWorld& w,const void* p,size_t n){
 if(overlaps(p,n,&w,sizeof(w)) || w.overlaps_byte_storage(p,n) ||
    overlaps(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) ||
    overlaps(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) ||
    overlaps(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) ||
    overlaps(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) ||
    overlaps(p,n,w.faction(0),8*sizeof(OriginalFactionState)) ||
    overlaps(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 return true;
}
bool same(const OriginalGamePreferences& a,const OriginalGamePreferences& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_prefs_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 return true;
}
bool same(const OriginalPrototype& a,const OriginalPrototype& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 return true;
}
bool same(const OriginalBase& a,const OriginalBase& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_base_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 return true;
}
bool same(const OriginalFactionState& a,const OriginalFactionState& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_faction_state_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 return true;
}
bool same(const OriginalFactionDefinition& a,const OriginalFactionDefinition& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_faction_definition_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 return true;
}
bool same_info(const OriginalMapInfo& a,const OriginalMapInfo& b){
 if(a.width!=b.width || a.height!=b.height || a.tile_count!=b.tile_count ||
    a.abstract_width!=b.abstract_width || a.abstract_height!=b.abstract_height ||
    a.abstract_count!=b.abstract_count || a.random_seed!=b.random_seed ||
    a.sea_level!=b.sea_level || a.council_sea_level!=b.council_sea_level ||
    a.flat!=b.flat || a.landmark_count!=b.landmark_count || a.off_map_landmarks!=b.off_map_landmarks ||
    a.stored_counts_differ!=b.stored_counts_differ || !same_array(a.world_settings,b.world_settings,7))return false;
 for(size_t i=0;i<64;++i){const auto& x=a.landmarks[i];const auto& y=b.landmarks[i];
  if(x.x!=y.x || x.y!=y.y || x.raw_coordinate_on_map!=y.raw_coordinate_on_map || memcmp(x.name,y.name,33))return false;
 }return true;
}
bool same_view(const OriginalGameView& a,const OriginalGameView& b){
 if(a.data_version!=b.data_version || a.legacy_marker_version!=b.legacy_marker_version ||
    a.vehicle_count!=b.vehicle_count || a.base_count!=b.base_count || a.replay_size!=b.replay_size ||
    a.tick_seed!=b.tick_seed || a.player!=b.player || a.canvas_width!=b.canvas_width || a.canvas_height!=b.canvas_height ||
    a.expanded_preferences!=b.expanded_preferences || !same_array(a.map_cursor,b.map_cursor,5) ||
    !same(a.preferences,b.preferences) || !same_info(a.map.info,b.map.info))return false;
 for(size_t i=0;i<9;++i){const auto& x=a.views[i];const auto& y=b.views[i];
  if(x.draw_b!=y.draw_b || x.draw_a!=y.draw_a || x.flags!=y.flags || x.zoom!=y.zoom || x.x!=y.x || x.y!=y.y)return false;
 }return true;
}
void put(uint8_t* p,uint32_t n,size_t bytes){for(size_t i=0;i<bytes;++i){p[i]=(uint8_t)n;n>>=8;}}
bool span_offset(const OriginalWorld& w,const void* p,size_t bytes,size_t& out){
 const uintptr_t raw=(uintptr_t)w.original_bytes(),at=(uintptr_t)p;
 if(at<raw || at-raw>w.original_size() || bytes>w.original_size()-(size_t)(at-raw))return false;
 out=(size_t)(at-raw);return true;
}
}
LocalSnapshotError encode_local_accounting_snapshot(const OriginalWorld& w,void* output,size_t capacity,LocalSnapshotReceipt& receipt){
 if(!w.loaded())return LocalSnapshotError::bad_world;
 const size_t n=w.original_size();
 if(!output || capacity<n || !disjoint(w,output,n) || !disjoint(w,&receipt,sizeof(receipt)) ||
    overlaps(output,n,&receipt,sizeof(receipt)))return LocalSnapshotError::bad_buffer;
 OriginalGameView baseline{};
 if(read_original_game(w.original_bytes(),n,baseline)!=GameReadError::ok)return LocalSnapshotError::bad_world;
 if(!same_view(baseline,w.game()))return LocalSnapshotError::unsupported_change;
 const auto monuments=w.current_monuments();
 if(monuments.size!=baseline.monuments.size || !monuments.data ||
    memcmp(monuments.data,baseline.monuments.data,monuments.size))return LocalSnapshotError::unsupported_change;
 const auto& live=w.game();
 if(baseline.map.metadata!=live.map.metadata || baseline.map.tiles!=live.map.tiles || baseline.map.abstract!=live.map.abstract)return LocalSnapshotError::unsupported_change;
#define CHECK_SPAN(name) if(baseline.name.data!=live.name.data || baseline.name.size!=live.name.size)return LocalSnapshotError::unsupported_change;
 CHECK_SPAN(factions) CHECK_SPAN(faction_definitions) CHECK_SPAN(rankings) CHECK_SPAN(faction_turn_might)
 CHECK_SPAN(replay) CHECK_SPAN(continents) CHECK_SPAN(prototypes) CHECK_SPAN(bunglist) CHECK_SPAN(vehicles)
 CHECK_SPAN(bases) CHECK_SPAN(scenario) CHECK_SPAN(scenario_name) CHECK_SPAN(objectives) CHECK_SPAN(monuments) CHECK_SPAN(tectonic)
#undef CHECK_SPAN
 for(size_t i=0;i<512;++i){OriginalPrototype p{};if(original_prototype(baseline,i,p)!=GameReadError::ok || !same(p,*w.prototype(i)))return LocalSnapshotError::unsupported_change;}
 for(size_t i=0;i<w.game().base_count;++i){OriginalBase b{};if(original_base(baseline,i,b)!=GameReadError::ok)return LocalSnapshotError::bad_world;
  const auto& live_base=*w.base(i);b.minerals_accumulated=live_base.minerals_accumulated;b.queue_items[0]=live_base.queue_items[0];
#define ACCOUNTING_FIELD(name,offset) b.name=live_base.name;
#define ACCOUNTING_SPECIALISTS(offset) for(size_t j=0;j<2;++j)b.specialist_types[j]=live_base.specialist_types[j];
#include "local_accounting_fields.inc"
#undef ACCOUNTING_FIELD
#undef ACCOUNTING_SPECIALISTS
  if(!same(b,live_base))return LocalSnapshotError::unsupported_change;
 }
 for(size_t i=0;i<8;++i){OriginalFactionState f{};OriginalFactionDefinition d{};
  if(original_faction_state(baseline,i,f)!=GameReadError::ok || original_faction_definition(baseline,i,d)!=GameReadError::ok)return LocalSnapshotError::bad_world;
  for(size_t j=0;j<512;++j)f.units_queue[j]=w.faction(i)->units_queue[j];
  if(!same(f,*w.faction(i)) || !same(d,*w.faction_definition(i)))return LocalSnapshotError::unsupported_change;
 }
 for(size_t i=0;i<w.game().vehicle_count;++i){const auto& v=*w.vehicle(i);if(v.unit_id<0 || v.unit_id>=512 || v.home_base_id< -1 || v.home_base_id>=(int32_t)w.game().base_count)return LocalSnapshotError::bad_reference;}
 VehicleStackStorage storage{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,const_cast<OriginalMapTile*>(w.tile(0)),w.game().map.info.tile_count,const_cast<OriginalVehicle*>(w.vehicle(0)),w.game().vehicle_count};
 const auto stack=validate_vehicle_stacks(storage);if(stack!=StackError::ok)return stack==StackError::out_of_memory?LocalSnapshotError::out_of_memory:LocalSnapshotError::bad_reference;
 size_t tile_offset,vehicle_offset;
 if(!span_offset(w,baseline.map.tiles,baseline.map.info.tile_count*44,tile_offset) || !span_offset(w,baseline.vehicles.data,baseline.vehicle_count*52,vehicle_offset))return LocalSnapshotError::bad_world;
 const size_t base_stride=baseline.data_version>=11?308:304;
 const size_t faction_stride=baseline.data_version>=11?0x20cc:0x20bc;
 // The legacy schema shifts this field by FOUR bytes, unlike later fields
 // after project-intel expansion. Do not use the unrelated twelve-byte delta.
 const size_t queue_offset=baseline.data_version>=11?0xa58:0xa54;
 size_t base_offset,faction_offset;
 if(!span_offset(w,baseline.bases.data,baseline.base_count*base_stride,base_offset) ||
    !span_offset(w,baseline.factions.data,8*faction_stride,faction_offset))return LocalSnapshotError::bad_world;
 // All validation is complete before the first output write. Opaque bytes,
 // runtime/footer tick, inactive queue slots and mineral baseline stay intact.
 auto* bytes=(uint8_t*)output;memcpy(bytes,w.original_bytes(),n);
 for(size_t i=0;i<baseline.map.info.tile_count;++i){const auto& t=*w.tile(i);auto* p=bytes+tile_offset+i*44;
  p[0]=t.climate;p[1]=t.contour;p[2]=t.val2;p[3]=t.region;p[4]=t.visibility;p[5]=t.val3;p[6]=t.unknown;p[7]=(uint8_t)t.owner;
  put(p+8,t.items,4);put(p+12,t.landmarks,4);for(size_t j=0;j<7;++j)put(p+16+j*4,t.visible_items[j],4);
 }
 for(size_t i=0;i<baseline.vehicle_count;++i){const auto& v=*w.vehicle(i);auto* p=bytes+vehicle_offset+i*52;
#define WIRE_SCALAR(type,name,offset) put(p+offset,(uint32_t)v.name,sizeof(type));
#define WIRE_ARRAY(type,name,count,offset) for(size_t j=0;j<count;++j)put(p+offset+j*sizeof(type),(uint32_t)v.name[j],sizeof(type));
#define WIRE_STRING(name,count,offset) memcpy(p+offset,v.name,count);
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 }
 for(size_t i=0;i<baseline.base_count;++i){const auto& b=*w.base(i);auto* p=bytes+base_offset+i*base_stride;
  put(p+0x40,(uint32_t)b.minerals_accumulated,4);put(p+0x50,(uint32_t)b.queue_items[0],4);
#define ACCOUNTING_FIELD(name,offset) put(p+offset-(baseline.data_version<11 && offset>=0x98?4:0),(uint32_t)b.name,4);
#define ACCOUNTING_SPECIALISTS(offset) for(size_t j=0;j<2;++j)put(p+offset+j*4,(uint32_t)b.specialist_types[j],4);
#include "local_accounting_fields.inc"
#undef ACCOUNTING_FIELD
#undef ACCOUNTING_SPECIALISTS
 }
 for(size_t i=0;i<8;++i)memcpy(bytes+faction_offset+i*faction_stride+queue_offset,w.faction(i)->units_queue,512);
 receipt={n,w.runtime_seed()};return LocalSnapshotError::ok;
}
const char* accounting_snapshot_error_name(LocalSnapshotError e){switch(e){
 case LocalSnapshotError::ok:return "ok";case LocalSnapshotError::bad_world:return "bad_world";
 case LocalSnapshotError::bad_buffer:return "bad_buffer";case LocalSnapshotError::unsupported_change:return "unsupported_change";
 case LocalSnapshotError::bad_reference:return "bad_reference";case LocalSnapshotError::out_of_memory:return "out_of_memory";
 }return "unknown";}
}
