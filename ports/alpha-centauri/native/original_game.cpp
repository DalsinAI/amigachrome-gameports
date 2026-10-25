/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in original_game.h. */
#include "original_game.h"
#include <type_traits>
namespace alpha {
static uint32_t u32(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t i32(const uint8_t* p){uint32_t n=u32(p);return n<=0x7fffffffu?(int32_t)n:-1-(int32_t)(~n);}
template<class T>static T field(const uint8_t* p){
 uint32_t n=sizeof(T)==1?p[0]:sizeof(T)==2?(uint32_t)p[0]|((uint32_t)p[1]<<8):u32(p);
 if constexpr(std::is_signed<T>::value){
  if constexpr(sizeof(T)==1)return n<=127?(T)n:(T)(-1-(int32_t)(255-n));
  if constexpr(sizeof(T)==2)return n<=32767?(T)n:(T)(-1-(int32_t)(65535-n));
  if constexpr(sizeof(T)==4)return (T)i32(p);
 }else return (T)n;
}
struct Cursor {
 const uint8_t* bytes;size_t length,position;
 bool take(size_t n,OriginalByteSpan& output){if(n>length-position)return false;output={bytes+position,n};position+=n;return true;}
};
static void preferences(const uint8_t* p,bool expanded,OriginalGamePreferences& result){
 auto offset=[&](size_t n){return !expanded && n>=0x184?n-0x7c:n;};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+offset(off));
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i){if(!expanded && off==0x84 && i>=33)result.name[i]=(type)-1;else result.name[i]=field<type>(p+offset(off)+i*sizeof(type));}
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[offset(off)+i];result.name[count]=0;
#include "original_prefs_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
}
GameReadError read_original_game(const uint8_t* bytes,size_t length,OriginalGameView& output){
 const uint8_t magic[]={'T','E','R','R','A','N',0,0x1a};
 if(!bytes || length<8)return GameReadError::bad_header;
 for(unsigned i=0;i<8;++i)if(bytes[i]!=magic[i])return GameReadError::bad_header;
 if(length<12)return GameReadError::truncated;
 if(u32(bytes+8)!=0x56)return GameReadError::unsupported_version;
 Cursor cursor{bytes,length,12};OriginalByteSpan legacy,prefs,part;
 if(!cursor.take(0x31c,legacy))return GameReadError::truncated;
 const int32_t marker=i32(legacy.data+0x304);if(marker<7 || marker>12)return GameReadError::unsupported_version;
 OriginalGameView result{};result.legacy_marker_version=(uint32_t)marker;result.expanded_preferences=marker>=11;
 prefs=legacy;if(result.expanded_preferences && !cursor.take(0x398,prefs))return GameReadError::truncated;
 preferences(prefs.data,result.expanded_preferences,result.preferences);
 const int32_t version=result.preferences.SaveFileVersion;if(version<7 || version>12)return GameReadError::unsupported_version;
 result.data_version=(uint32_t)version;
 if(((uint32_t)result.preferences.GameState&0x10000000u) || ((uint32_t)result.preferences.GameMoreRules&0x20u))return GameReadError::unsupported_features;
 if(result.preferences.VehCount<0 || result.preferences.VehCount>2048 || result.preferences.BaseCount<0 || result.preferences.BaseCount>512)return GameReadError::bad_counts;
 result.vehicle_count=(uint32_t)result.preferences.VehCount;result.base_count=(uint32_t)result.preferences.BaseCount;
 if(!cursor.take(4,part))return GameReadError::truncated;
 result.player=i32(part.data);
 if(result.player<0 || result.player>=8)return GameReadError::bad_player;
 if(!cursor.take(version>=11?0x10660:0x105e0,result.factions) || !cursor.take(0x2ce0,result.faction_definitions) || !cursor.take(0x20,result.rankings))return GameReadError::truncated;
 size_t used=0;if(read_original_map_block(bytes+cursor.position,length-cursor.position,result.map,used)!=MapReadError::ok)return GameReadError::bad_map;
 cursor.position+=used;
 if(!cursor.take(0x3e80,result.faction_turn_might) || !cursor.take(4,part))return GameReadError::truncated;
 const int32_t replay=i32(part.data);if(replay<0 || replay>8192)return GameReadError::bad_counts;result.replay_size=(uint32_t)replay;
 if(!cursor.take(result.replay_size,result.replay) || !cursor.take(0xe00,result.continents) || !cursor.take(0x6800,result.prototypes) ||
    !cursor.take(0x1000,result.bunglist) || !cursor.take(result.vehicle_count*52,result.vehicles) ||
    !cursor.take(result.base_count*(version>=11?308:304),result.bases))return GameReadError::truncated;
 if(((uint32_t)result.preferences.GameState&0x10u) && !cursor.take(0x100,result.scenario))return GameReadError::truncated;
 if(!cursor.take(0x100,result.scenario_name) || !cursor.take(0x98,result.objectives) || !cursor.take(0x27a0,result.monuments))return GameReadError::truncated;
 if(version>=12 && !cursor.take(0x20,result.tectonic))return GameReadError::truncated;
 if(!cursor.take(20,part))return GameReadError::truncated;
 for(unsigned i=0;i<5;++i)result.map_cursor[i]=i32(part.data+i*4);
 for(unsigned i=0;i<9;++i){if(!cursor.take(24,part))return GameReadError::truncated;
  result.views[i]={i32(part.data),i32(part.data+4),i32(part.data+8),i32(part.data+12),i32(part.data+16),i32(part.data+20)};}
 if(!cursor.take(8,part))return GameReadError::truncated;
 result.canvas_width=i32(part.data);result.canvas_height=i32(part.data+4);
 if(!cursor.take(4,part))return GameReadError::truncated;
 result.tick_seed=u32(part.data);
 if(cursor.position!=length)return GameReadError::trailing_data;
 output=result;return GameReadError::ok;
}
GameReadError original_vehicle(const OriginalGameView& view,size_t index,OriginalVehicle& output){
 if(!view.vehicles.data || index>=view.vehicle_count || view.vehicles.size/52<=index)return GameReadError::bad_index;
 const uint8_t* p=view.vehicles.data+index*52;OriginalVehicle result{};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+off);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=field<type>(p+off+i*sizeof(type));
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[off+i];result.name[count]=0;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 output=result;return GameReadError::ok;
}
GameReadError original_prototype(const OriginalGameView& view,size_t index,OriginalPrototype& output){
 if(!view.prototypes.data || index>=512 || view.prototypes.size/52<=index)return GameReadError::bad_index;
 const uint8_t* p=view.prototypes.data+index*52;OriginalPrototype result{};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+off);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=field<type>(p+off+i*sizeof(type));
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[off+i];result.name[count]=0;
#include "original_prototype_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 output=result;return GameReadError::ok;
}
GameReadError original_base(const OriginalGameView& view,size_t index,OriginalBase& output){
 const size_t size=view.data_version>=11?308:304;
 if(!view.bases.data || index>=view.base_count || view.bases.size/size<=index)return GameReadError::bad_index;
 const uint8_t* p=view.bases.data+index*size;OriginalBase result{};
 auto offset=[&](size_t n){return view.data_version<11 && n>=0x98?n-4:n;};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+offset(off));
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i){if(view.data_version<11 && off==0x8c && i>=8)result.name[i]=0;else result.name[i]=field<type>(p+offset(off)+i*sizeof(type));}
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[offset(off)+i];result.name[count]=0;
#include "original_base_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
 output=result;return GameReadError::ok;
}
const char* game_read_error_name(GameReadError error){
 switch(error){case GameReadError::ok:return "ok";case GameReadError::bad_header:return "not an original TERRAN game";
 case GameReadError::unsupported_version:return "unsupported save version";case GameReadError::unsupported_features:return "encrypted/network save not yet supported";
 case GameReadError::truncated:return "incomplete game file";case GameReadError::bad_counts:return "invalid game record counts";
 case GameReadError::bad_player:return "invalid current player";case GameReadError::bad_map:return "invalid embedded map";
 case GameReadError::trailing_data:return "unexpected bytes after game";case GameReadError::bad_index:return "record outside game";}
 return "unknown game error";
}
}
