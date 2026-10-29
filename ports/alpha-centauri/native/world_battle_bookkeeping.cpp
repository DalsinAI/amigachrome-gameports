/* SPDX-License-Identifier: GPL-3.0-or-later.
 * Current-frame ownership follows frozen VehicleRemovalPlan capture/rebase.
 */
#include "world_battle_bookkeeping.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <type_traits>
namespace alpha {namespace {
template<class T>T* copy(const T* from,size_t n){static_assert(std::is_trivially_copyable<T>::value,"owned record copy");if(!n||!from||n>SIZE_MAX/sizeof(T))return nullptr;auto*p=(T*)malloc(n*sizeof(T));if(p){for(size_t i=0;i<n;++i)new(p+i)T;memcpy(p,from,n*sizeof(T));}return p;}
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
WorldBookkeepingError WorldBattleBookkeepingPlan::capture(const OriginalWorld&w,size_t limit){
 if(!w.loaded()||!w.game().bunglist.data||w.game().bunglist.size!=4096)return WorldBookkeepingError::bad_state;
 const auto&g=w.game();const uint64_t arrays=(uint64_t)g.map.info.tile_count*sizeof(OriginalMapTile)+(uint64_t)g.vehicle_count*sizeof(OriginalVehicle)+(uint64_t)g.base_count*sizeof(OriginalBase)+512u*(uint64_t)sizeof(OriginalPrototype)+8u*(uint64_t)(sizeof(OriginalFactionState)+sizeof(OriginalFactionDefinition));const uint64_t required=sizeof(*this)+(uint64_t)w.length_+0x27a0u+arrays;if(required>limit||required>SIZE_MAX)return WorldBookkeepingError::out_of_memory;
 frame_.length_=w.length_;frame_.memory_=(size_t)required-sizeof(*this)+sizeof(OriginalWorld);frame_.random_seed_=w.random_seed_;frame_.view_=g;
 frame_.raw_=copy(w.raw_,w.length_);frame_.monuments_=copy(w.monuments_,size_t(0x27a0));frame_.tiles_=copy(w.tiles_,g.map.info.tile_count);frame_.vehicles_=copy(w.vehicles_,g.vehicle_count);frame_.bases_=copy(w.bases_,g.base_count);frame_.prototypes_=copy(w.prototypes_,512);frame_.factions_=copy(w.factions_,8);frame_.definitions_=copy(w.definitions_,8);
 if(!frame_.raw_||!frame_.monuments_||!frame_.tiles_||!frame_.prototypes_||!frame_.factions_||!frame_.definitions_||(g.vehicle_count&&!frame_.vehicles_)||(g.base_count&&!frame_.bases_))return WorldBookkeepingError::out_of_memory;
 auto relocate=[&](const uint8_t*source,size_t size,const uint8_t*&dest){if(!source&&!size){dest=nullptr;return true;}const uintptr_t x=(uintptr_t)source,start=(uintptr_t)w.raw_;if(x<start||x-start>w.length_||size>w.length_-(size_t)(x-start))return false;dest=frame_.raw_+(size_t)(x-start);return true;};auto span=[&](OriginalByteSpan&s){return relocate(s.data,s.size,s.data);};
 if(!relocate(g.map.metadata,original_map_metadata_size,frame_.view_.map.metadata)||!relocate(g.map.tiles,(size_t)g.map.info.tile_count*original_map_tile_size,frame_.view_.map.tiles)||!relocate(g.map.abstract,g.map.info.abstract_count,frame_.view_.map.abstract))return WorldBookkeepingError::bad_state;
#define REBASE(name) if(!span(frame_.view_.name))return WorldBookkeepingError::bad_state;
 REBASE(factions) REBASE(faction_definitions) REBASE(rankings) REBASE(faction_turn_might) REBASE(replay) REBASE(continents) REBASE(prototypes) REBASE(vehicles) REBASE(bases) REBASE(scenario) REBASE(scenario_name) REBASE(objectives) REBASE(monuments) REBASE(tectonic)
#undef REBASE
 memcpy(history_,g.bunglist.data,4096);frame_.view_.bunglist={history_,4096};return WorldBookkeepingError::ok;
}

namespace {
uint32_t rd(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
void wr(uint8_t*p,uint32_t v){for(unsigned j=0;j<4;++j){p[j]=(uint8_t)v;v>>=8;}}
int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
}
WorldBookkeepingError WorldBattleBookkeepingPlan::prepare(const OriginalWorld&w,const WorldBookkeepingFacts&facts,size_t limit){
 BattleBookkeepingContext ctx{};ctx.edition=facts.edition;ctx.attacker=facts.attacker_faction;ctx.defender=facts.defender_faction;ctx.outcome=facts.outcome;ctx.current_player=facts.current_player;ctx.multiplayer_active=facts.multiplayer_active;ctx.pbem_active=facts.pbem_active;ctx.observation=facts.observation;ctx.turn=(uint32_t)w.game().preferences.CurrentTurn;ctx.start_year=facts.start_year;
 BattleBookkeepingState state{};
 for(size_t i=0;i<8;++i){ctx.rule_flags[i]=w.faction_definition(i)->rule_flags;state.wins[i]=facts.wins[i];state.losses[i]=facts.losses[i];const auto*f=w.faction(i);for(size_t j=0;j<8;++j){state.combat_total[i][j]=(uint32_t)f->diplo_combat_total[j];state.combat_count[i][j]=(uint32_t)f->diplo_combat_count[j];}
 const auto*p=w.current_monuments().data+i*0x4f4;auto&r=state.enemy_destroyed[i];r.year=rd(p+0x150);r.opponent=rd(p+0x158);r.leader=rd(p+0x15c);r.awarded=rd(p+0x160);r.rank=rd(p+0x164);r.focus=rd(p+0x498);
 }
 auto e=preview_battle_bookkeeping(ctx,state,result_);if(e!=BookkeepingError::ok)return e==BookkeepingError::monument_ui?WorldBookkeepingError::monument_ui:e==BookkeepingError::network_notification?WorldBookkeepingError::unsupported:WorldBookkeepingError::bad_state;
 auto status=capture(w,limit);if(status!=WorldBookkeepingError::ok)return status;
 const auto&after=result_.after_cleanup;
 for(size_t i=0;i<8;++i){for(size_t j=0;j<8;++j){frame_.factions_[i].diplo_combat_total[j]=signed32(after.combat_total[i][j]);frame_.factions_[i].diplo_combat_count[j]=signed32(after.combat_count[i][j]);}
 auto*p=frame_.monuments_+i*0x4f4;const auto&r=after.enemy_destroyed[i];wr(p+0x150,r.year);wr(p+0x158,r.opponent);wr(p+0x15c,r.leader);wr(p+0x160,r.awarded);wr(p+0x164,r.rank);wr(p+0x498,r.focus);
 }
 ready_=true;return WorldBookkeepingError::ok;
}
void WorldBattleBookkeepingPlan::swap(WorldBattleBookkeepingPlan&p){
 frame_.swap(p.frame_);for(size_t j=0;j<4096;++j){auto v=history_[j];history_[j]=p.history_[j];p.history_[j]=v;}const auto result=result_;result_=p.result_;p.result_=result;const auto ready=ready_;ready_=p.ready_;p.ready_=ready;if(frame_.loaded())frame_.view_.bunglist={history_,4096};if(p.frame_.loaded())p.frame_.view_.bunglist={p.history_,4096};
}
WorldBookkeepingError stage_world_battle_bookkeeping(const OriginalWorld&w,const WorldBookkeepingFacts&facts,WorldBattleBookkeepingPlan&out,size_t limit){
 if(!w.loaded()||w.game().vehicle_count>2048||w.game().preferences.VehCount!=(int32_t)w.game().vehicle_count||w.current_monuments().size!=0x27a0||!w.current_monuments().data||alias(w,&facts,sizeof facts)||alias(w,&out,sizeof out)||detail::effects_range_overlap(&facts,sizeof facts,&out,sizeof out)||alias(out.frame_,&facts,sizeof facts))return WorldBookkeepingError::bad_state;
 if((uint32_t)facts.edition>1)return WorldBookkeepingError::bad_state;
 if((facts.edition==BookkeepingEdition::smax)!=(w.game().data_version>=11))return WorldBookkeepingError::unsupported;
 WorldBattleBookkeepingPlan next;const auto e=next.prepare(w,facts,limit);if(e==WorldBookkeepingError::ok)out.swap(next);return e;
}
}
