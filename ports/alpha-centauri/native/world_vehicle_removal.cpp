/* SPDX-License-Identifier: GPL-3.0-or-later.
 * Current-frame capture follows the frozen LandEngagementPlan ownership pattern.
 */
#include "world_vehicle_removal.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <type_traits>
namespace alpha {namespace {
template<class T>T* copy(const T* from,size_t n){static_assert(std::is_trivially_copyable<T>::value,"owned record copy");if(!n||!from||n>SIZE_MAX/sizeof(T))return nullptr;auto*p=(T*)malloc(n*sizeof(T));if(p){for(size_t i=0;i<n;++i)new(p+i)T;memcpy(p,from,n*sizeof(T));}return p;}
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
RemovalError VehicleRemovalPlan::capture(const OriginalWorld&w,size_t limit){
 if(!w.loaded()||!w.game().bunglist.data||w.game().bunglist.size!=4096)return RemovalError::bad_state;
 const auto&g=w.game();const uint64_t arrays=(uint64_t)g.map.info.tile_count*sizeof(OriginalMapTile)+(uint64_t)g.vehicle_count*sizeof(OriginalVehicle)+(uint64_t)g.base_count*sizeof(OriginalBase)+512u*(uint64_t)sizeof(OriginalPrototype)+8u*(uint64_t)(sizeof(OriginalFactionState)+sizeof(OriginalFactionDefinition));const uint64_t required=sizeof(*this)+(uint64_t)w.length_+0x27a0u+arrays;if(required>limit||required>SIZE_MAX)return RemovalError::out_of_memory;
 frame_.length_=w.length_;frame_.memory_=(size_t)required-sizeof(*this)+sizeof(OriginalWorld);frame_.random_seed_=w.random_seed_;frame_.view_=g;
 frame_.raw_=copy(w.raw_,w.length_);frame_.monuments_=copy(w.monuments_,size_t(0x27a0));frame_.tiles_=copy(w.tiles_,g.map.info.tile_count);frame_.vehicles_=copy(w.vehicles_,g.vehicle_count);frame_.bases_=copy(w.bases_,g.base_count);frame_.prototypes_=copy(w.prototypes_,512);frame_.factions_=copy(w.factions_,8);frame_.definitions_=copy(w.definitions_,8);
 if(!frame_.raw_||!frame_.monuments_||!frame_.tiles_||!frame_.prototypes_||!frame_.factions_||!frame_.definitions_||(g.vehicle_count&&!frame_.vehicles_)||(g.base_count&&!frame_.bases_))return RemovalError::out_of_memory;
 auto relocate=[&](const uint8_t*source,size_t size,const uint8_t*&dest){if(!source&&!size){dest=nullptr;return true;}const uintptr_t x=(uintptr_t)source,start=(uintptr_t)w.raw_;if(x<start||x-start>w.length_||size>w.length_-(size_t)(x-start))return false;dest=frame_.raw_+(size_t)(x-start);return true;};auto span=[&](OriginalByteSpan&s){return relocate(s.data,s.size,s.data);};
 if(!relocate(g.map.metadata,original_map_metadata_size,frame_.view_.map.metadata)||!relocate(g.map.tiles,(size_t)g.map.info.tile_count*original_map_tile_size,frame_.view_.map.tiles)||!relocate(g.map.abstract,g.map.info.abstract_count,frame_.view_.map.abstract))return RemovalError::bad_state;
#define REBASE(name) if(!span(frame_.view_.name))return RemovalError::bad_state;
 REBASE(factions) REBASE(faction_definitions) REBASE(rankings) REBASE(faction_turn_might) REBASE(replay) REBASE(continents) REBASE(prototypes) REBASE(vehicles) REBASE(bases) REBASE(scenario) REBASE(scenario_name) REBASE(objectives) REBASE(monuments) REBASE(tectonic)
#undef REBASE
 memcpy(history_,g.bunglist.data,4096);frame_.view_.bunglist={history_,4096};return RemovalError::ok;
}

RemovalError VehicleRemovalPlan::apply(size_t id,const RemovalRuntime&runtime){
 runtime_=runtime;removed_=frame_.vehicles_[id];const auto&m=frame_.view_.map.info;
 VehicleRemovalStorage storage{{(int32_t)m.width,(int32_t)m.height,frame_.tiles_,m.tile_count,frame_.vehicles_,frame_.view_.vehicle_count},frame_.prototypes_,512,frame_.factions_,8,&runtime_};
 const auto error=remove_vehicle_storage(storage,id);if(error!=RemovalError::ok)return error;
 frame_.view_.vehicle_count=(uint32_t)storage.stacks.vehicle_count;
 frame_.view_.preferences.VehCount=(int32_t)storage.stacks.vehicle_count;
 ready_=true;return RemovalError::ok;
}
void VehicleRemovalPlan::swap(VehicleRemovalPlan&p){
 frame_.swap(p.frame_);for(size_t i=0;i<4096;++i){auto x=history_[i];history_[i]=p.history_[i];p.history_[i]=x;}
 auto r=runtime_;runtime_=p.runtime_;p.runtime_=r;auto v=removed_;removed_=p.removed_;p.removed_=v;const auto ready=ready_;ready_=p.ready_;p.ready_=ready;
 if(frame_.loaded())frame_.view_.bunglist={history_,4096};
 if(p.frame_.loaded())p.frame_.view_.bunglist={p.history_,4096};
}
RemovalError stage_vehicle_removal(const OriginalWorld&w,size_t victim,const RemovalRuntime&runtime,VehicleRemovalPlan&out,size_t limit){
 if(!w.loaded()||victim>=w.game().vehicle_count||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||alias(w,&runtime,sizeof runtime)||alias(w,&out,sizeof out)||detail::effects_range_overlap(&out,sizeof out,&runtime,sizeof runtime)||alias(out.frame_,&runtime,sizeof runtime))return RemovalError::bad_state;
 if(runtime.observation!=RemovalObservation::supplied)return RemovalError::bad_state;
 if(runtime.multiplayer_active||runtime.logging_enabled||runtime.base_window_visible)return RemovalError::unsupported;
 VehicleRemovalPlan next;auto error=next.capture(w,limit);if(error!=RemovalError::ok)return error;
 error=next.apply(victim,runtime);if(error==RemovalError::ok)out.swap(next);return error;
}
}
