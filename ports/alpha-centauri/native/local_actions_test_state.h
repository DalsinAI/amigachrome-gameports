/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only explicit field equality. */
#ifndef ALPHA_LOCAL_ACTIONS_TEST_STATE_H
#define ALPHA_LOCAL_ACTIONS_TEST_STATE_H
#include "original_world.h"
#include <string.h>
namespace alpha_test {
using namespace alpha;
template<class T> bool same_array(const T* a,const T* b,size_t n){for(size_t i=0;i<n;++i)if(a[i]!=b[i])return false;return true;}
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
bool same(const OriginalVehicle& a,const OriginalVehicle& b){
#define WIRE_SCALAR(type,name,offset) if(a.name!=b.name)return false;
#define WIRE_ARRAY(type,name,count,offset) if(!same_array(a.name,b.name,count))return false;
#define WIRE_STRING(name,count,offset) if(memcmp(a.name,b.name,count+1))return false;
#define WIRE_MATRIX(type,name,rows,cols,offset) if(!same_array(&a.name[0][0],&b.name[0][0],rows*cols))return false;
#define WIRE_TEXT_MATRIX(name,rows,cols,offset) for(size_t row=0;row<rows;++row)if(memcmp(a.name[row],b.name[row],cols+1))return false;
#define WIRE_GOALS(name,count,offset) for(size_t i=0;i<count;++i)if(a.name[i].type!=b.name[i].type || a.name[i].priority!=b.name[i].priority || a.name[i].x!=b.name[i].x || a.name[i].y!=b.name[i].y || a.name[i].base_id!=b.name[i].base_id)return false;
#include "original_vehicle_fields.inc"
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

bool same_world(const OriginalWorld& a,const OriginalWorld& b,bool original_identity=true){
 if(a.original_size()!=b.original_size() || (original_identity && memcmp(a.original_bytes(),b.original_bytes(),a.original_size())) || a.runtime_seed()!=b.runtime_seed() || !same_view(a.game(),b.game()))return false;
 for(size_t i=0;i<512;++i)if(!same(*a.prototype(i),*b.prototype(i)))return false;
 for(size_t i=0;i<8;++i)if(!same(*a.faction(i),*b.faction(i)) || !same(*a.faction_definition(i),*b.faction_definition(i)))return false;
 for(size_t i=0;i<a.game().vehicle_count;++i)if(!same(*a.vehicle(i),*b.vehicle(i)))return false;
 for(size_t i=0;i<a.game().base_count;++i)if(!same(*a.base(i),*b.base(i)))return false;
 for(size_t i=0;i<a.game().map.info.tile_count;++i){const auto& x=*a.tile(i);const auto& y=*b.tile(i);if(x.climate!=y.climate || x.contour!=y.contour || x.val2!=y.val2 || x.region!=y.region || x.visibility!=y.visibility || x.val3!=y.val3 || x.unknown!=y.unknown || x.owner!=y.owner || x.items!=y.items || x.landmarks!=y.landmarks || !same_array(x.visible_items,y.visible_items,7))return false;}
 return true;
}
}
#endif
