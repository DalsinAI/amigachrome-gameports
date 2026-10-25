/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_world.h"
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <new>
#include <type_traits>
namespace alpha {
template<class T>static T* allocate(size_t count){
 static_assert(std::is_trivially_destructible<T>::value,"owned records require trivial destruction");
 if(!count)return nullptr;
 auto* p=(T*)malloc(count*sizeof(T));if(p)for(size_t i=0;i<count;++i)new(p+i) T;
 return p;
}
void OriginalWorld::clear(){
 free(raw_);free(tiles_);free(vehicles_);free(bases_);free(prototypes_);free(factions_);free(definitions_);
 raw_=nullptr;tiles_=nullptr;vehicles_=nullptr;bases_=nullptr;prototypes_=nullptr;factions_=nullptr;definitions_=nullptr;
 view_={};length_=memory_=0;random_seed_=0;
}
void OriginalWorld::swap(OriginalWorld& other){
 using std::swap;
 swap(raw_,other.raw_);swap(tiles_,other.tiles_);swap(vehicles_,other.vehicles_);swap(bases_,other.bases_);
 swap(prototypes_,other.prototypes_);swap(factions_,other.factions_);swap(definitions_,other.definitions_);
 swap(view_,other.view_);swap(length_,other.length_);swap(memory_,other.memory_);swap(random_seed_,other.random_seed_);
}
static bool coordinate(const OriginalMapInfo& m,int32_t x,int32_t y){return x>=0 && (uint32_t)x<m.width && y>=0 && (uint32_t)y<m.height && !((x+y)&1);}
static bool reference(int32_t value,uint32_t count){return value==-1 || (value>=0 && (uint32_t)value<count);}
WorldLoadError OriginalWorld::load(const uint8_t* bytes,size_t length,uint32_t runtime_seed,size_t limit){
 OriginalGameView source;
 if(read_original_game(bytes,length,source)!=GameReadError::ok)return WorldLoadError::bad_format;
 const uint64_t required=(uint64_t)sizeof(OriginalWorld)+length+(uint64_t)source.map.info.tile_count*sizeof(OriginalMapTile)+
  (uint64_t)source.vehicle_count*sizeof(OriginalVehicle)+(uint64_t)source.base_count*sizeof(OriginalBase)+
  512u*(uint64_t)sizeof(OriginalPrototype)+8u*(uint64_t)(sizeof(OriginalFactionState)+sizeof(OriginalFactionDefinition));
 if(required>limit)return WorldLoadError::memory_limit;
 OriginalWorld next;next.length_=length;next.memory_=(size_t)required;next.random_seed_=runtime_seed;
 next.raw_=(uint8_t*)malloc(length);
 next.tiles_=allocate<OriginalMapTile>(source.map.info.tile_count);
 next.vehicles_=allocate<OriginalVehicle>(source.vehicle_count);
 next.bases_=allocate<OriginalBase>(source.base_count);
 next.prototypes_=allocate<OriginalPrototype>(512);
 next.factions_=allocate<OriginalFactionState>(8);
 next.definitions_=allocate<OriginalFactionDefinition>(8);
 if(!next.raw_ || !next.tiles_ || !next.prototypes_ || !next.factions_ || !next.definitions_ ||
    (source.vehicle_count && !next.vehicles_) || (source.base_count && !next.bases_))return WorldLoadError::out_of_memory;
 memcpy(next.raw_,bytes,length);
 if(read_original_game(next.raw_,length,next.view_)!=GameReadError::ok)return WorldLoadError::bad_format;
 for(size_t i=0;i<source.map.info.tile_count;++i)if(original_map_tile(next.view_.map,i,next.tiles_[i])!=MapReadError::ok)return WorldLoadError::bad_format;
 for(size_t i=0;i<512;++i)if(original_prototype(next.view_,i,next.prototypes_[i])!=GameReadError::ok)return WorldLoadError::bad_format;
 for(size_t i=0;i<source.vehicle_count;++i){
  auto& v=next.vehicles_[i];if(original_vehicle(next.view_,i,v)!=GameReadError::ok)return WorldLoadError::bad_format;
  if(!coordinate(source.map.info,v.x,v.y) || v.faction_id>=8 || v.unit_id<0 || v.unit_id>=512 ||
     !reference(v.home_base_id,source.base_count) || !reference(v.next_veh_id_stack,source.vehicle_count) ||
     !reference(v.prev_veh_id_stack,source.vehicle_count))return WorldLoadError::bad_reference;
 }
 for(size_t i=0;i<source.base_count;++i){
  auto& b=next.bases_[i];if(original_base(next.view_,i,b)!=GameReadError::ok)return WorldLoadError::bad_format;
  if(!coordinate(source.map.info,b.x,b.y) || b.faction_id<0 || b.faction_id>=8 || b.queue_size< -1 || b.queue_size>=10)return WorldLoadError::bad_reference;
 }
 for(size_t i=0;i<8;++i)if(original_faction_state(next.view_,i,next.factions_[i])!=GameReadError::ok || original_faction_definition(next.view_,i,next.definitions_[i])!=GameReadError::ok)return WorldLoadError::bad_format;
 swap(next);return WorldLoadError::ok;
}
const char* world_load_error_name(WorldLoadError e){switch(e){case WorldLoadError::ok:return "ok";case WorldLoadError::bad_format:return "unsupported or incomplete original game";case WorldLoadError::memory_limit:return "world exceeds memory budget";case WorldLoadError::out_of_memory:return "cannot allocate original world";case WorldLoadError::bad_reference:return "original actor references invalid";}return "unknown world error";}
}
