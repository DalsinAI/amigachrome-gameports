/* SPDX-License-Identifier: GPL-3.0-or-later. See initialized_snapshot.h. */
#include "initialized_snapshot.h"
#include "technology_effects_ranges.h"
#include <string.h>
namespace alpha { namespace {
bool disjoint(const OriginalWorld&w,const void*p,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();
 return !effects_range_overlap(p,n,&w,sizeof w)&&!w.overlaps_byte_storage(p,n)&&
 !effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))&&
 !effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))&&
 !effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))&&
 !effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))&&
 !effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))&&
 !effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
}
LocalSnapshotError encode_initialized_snapshot(const OriginalWorld&w,const Rules&rules,void*out,size_t capacity,LocalSnapshotReceipt&receipt,size_t limit){
 if(!w.loaded())return LocalSnapshotError::bad_world;
 const size_t n=w.original_size();
 if(!out || capacity<n || !disjoint(w,out,n) || !disjoint(w,&receipt,sizeof receipt) ||
    detail::effects_range_overlap(out,n,&rules,sizeof rules) || detail::effects_range_overlap(&receipt,sizeof receipt,&rules,sizeof rules) ||
    detail::effects_range_overlap(out,n,&receipt,sizeof receipt))return LocalSnapshotError::bad_buffer;
 OriginalGameView baseline{};
 if(read_original_game(w.original_bytes(),n,baseline)!=GameReadError::ok)return LocalSnapshotError::bad_world;
 const auto&g=w.game();
 if(g.vehicle_count!=baseline.vehicle_count || g.base_count!=baseline.base_count ||
    g.map.info.tile_count!=baseline.map.info.tile_count)return LocalSnapshotError::unsupported_change;
 if(g.map.metadata!=baseline.map.metadata || g.map.tiles!=baseline.map.tiles || g.map.abstract!=baseline.map.abstract)return LocalSnapshotError::unsupported_change;
#define CHECK_SPAN(name) if(g.name.data!=baseline.name.data || g.name.size!=baseline.name.size)return LocalSnapshotError::unsupported_change;
 CHECK_SPAN(factions) CHECK_SPAN(faction_definitions) CHECK_SPAN(rankings) CHECK_SPAN(faction_turn_might) CHECK_SPAN(replay) CHECK_SPAN(continents) CHECK_SPAN(prototypes) CHECK_SPAN(bunglist) CHECK_SPAN(vehicles) CHECK_SPAN(bases) CHECK_SPAN(scenario) CHECK_SPAN(scenario_name) CHECK_SPAN(objectives) CHECK_SPAN(monuments) CHECK_SPAN(tectonic)
#undef CHECK_SPAN
 const uintptr_t raw=(uintptr_t)w.original_bytes(),factions=(uintptr_t)baseline.factions.data;
 const size_t stride=baseline.data_version<11?0x20bc:0x20cc;
 if(factions<raw || factions-raw>n || stride*8>n-(size_t)(factions-raw))return LocalSnapshotError::bad_world;
 const size_t faction_offset=(size_t)(factions-raw);
 OriginalWorld projection;const auto loaded=projection.load_with_record_repairs(w.original_bytes(),n,w.runtime_seed(),rules,limit);
 if(loaded!=WorldLoadError::ok)return loaded==WorldLoadError::bad_format || loaded==WorldLoadError::bad_reference?LocalSnapshotError::bad_world:LocalSnapshotError::out_of_memory;
 const auto& repaired=projection.game();
 if(g.preferences.FactionStatus[1]!=repaired.preferences.FactionStatus[1])return LocalSnapshotError::unsupported_change;
 for(size_t i=0;i<8;++i)if((w.faction(i)->player_flags^projection.faction(i)->player_flags)&0x200u)return LocalSnapshotError::unsupported_change;
 const size_t memory=projection.memory_bytes();
 if(memory>limit || memory>limit-memory)return LocalSnapshotError::out_of_memory;
 const size_t preferences_offset=12+(baseline.expanded_preferences?0x31c:0);
 if(preferences_offset>n || 0x5a>n-preferences_offset)return LocalSnapshotError::bad_world;
 // This is a private, non-const local allocation. Const inspection accessors
 // are cast only for this owned encoding projection, never the caller World.
 auto&view=const_cast<OriginalGameView&>(projection.game());view=g;
 view.preferences.FactionStatus[1]=baseline.preferences.FactionStatus[1];
 const auto rebase=[&](const uint8_t*p){return p?projection.original_bytes()+((uintptr_t)p-raw):nullptr;};
 view.map.metadata=rebase(baseline.map.metadata);view.map.tiles=rebase(baseline.map.tiles);view.map.abstract=rebase(baseline.map.abstract);
#define REBASE(name) view.name={rebase(baseline.name.data),baseline.name.size};
 REBASE(factions) REBASE(faction_definitions) REBASE(rankings) REBASE(faction_turn_might) REBASE(replay) REBASE(continents) REBASE(prototypes) REBASE(bunglist) REBASE(vehicles) REBASE(bases) REBASE(scenario) REBASE(scenario_name) REBASE(objectives) REBASE(monuments) REBASE(tectonic)
#undef REBASE
 for(size_t i=0;i<g.map.info.tile_count;++i)*const_cast<OriginalMapTile*>(projection.tile(i))=*w.tile(i);
 for(size_t i=0;i<g.vehicle_count;++i)*const_cast<OriginalVehicle*>(projection.vehicle(i))=*w.vehicle(i);
 for(size_t i=0;i<g.base_count;++i)*const_cast<OriginalBase*>(projection.base(i))=*w.base(i);
 for(size_t i=0;i<512;++i)*const_cast<OriginalPrototype*>(projection.prototype(i))=*w.prototype(i);
 const auto monuments=w.current_monuments(),clone_monuments=projection.current_monuments();
 if(monuments.size!=clone_monuments.size || !monuments.data || !clone_monuments.data)return LocalSnapshotError::unsupported_change;
 memcpy(const_cast<uint8_t*>(clone_monuments.data),monuments.data,monuments.size);
 for(size_t i=0;i<8;++i){
  auto&f=*const_cast<OriginalFactionState*>(projection.faction(i));const auto&current=*w.faction(i);
  const uint8_t*original=baseline.factions.data+i*stride;
  const uint32_t old_flags=(uint32_t)original[0]|((uint32_t)original[1]<<8)|((uint32_t)original[2]<<16)|((uint32_t)original[3]<<24);
  f=current;f.player_flags=(f.player_flags&~0x200u)|(old_flags&0x200u);
  *const_cast<OriginalFactionDefinition*>(projection.faction_definition(i))=*w.faction_definition(i);
 }
 // Frozen phase validator retains all its field, stack and metadata seals.
 LocalSnapshotReceipt next;const auto result=encode_phase_snapshot(projection,out,capacity,next,limit-memory);
 if(result!=LocalSnapshotError::ok)return result;
 // All validation is complete. Explicit LE patches have no fallible steps.
 auto*bytes=(uint8_t*)out;
 bytes[preferences_offset+0x59]=g.preferences.FactionStatus[1];
 for(size_t i=0;i<8;++i)put(bytes+faction_offset+i*stride,w.faction(i)->player_flags);
 receipt=next;return LocalSnapshotError::ok;
}
}
