/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_minerals.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const UnitDefinitions&d,const WorldMineralStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||overlap(out,n,&d,sizeof d)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return w.overlaps_byte_storage(out,n)||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
StableIntakeError logic(LogicError e){return e==LogicError::overflow?StableIntakeError::overflow:StableIntakeError::bad_state;}
bool raw_counter(const OriginalWorld&w,unsigned faction,int32_t&out){
 const auto&span=w.game().tectonic;const uintptr_t a=(uintptr_t)w.original_bytes(),p=(uintptr_t)span.data;
 if(span.size!=32||p<a||p-a>w.original_size()||span.size>w.original_size()-(p-a))return false;
 const uint8_t*b=span.data+4*faction;const uint32_t n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
 if(n>2147483647u)return false;
 out=(int32_t)n;return true;
}
}
StableIntakeError world_fresh_minerals(const OriginalWorld&w,const Rules&t,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const UnitDefinitions&d,size_t id,const WorldMineralStage&s,WorldMineralResult&output){
 if(aliases(w,t,b,wr,r,d,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded())return StableIntakeError::bad_state;
 if((s.intake.variant!=Variant::base&&s.intake.variant!=Variant::alien_crossfire)||(s.intake.variant==Variant::base&&s.intake.expansion_enabled))return StableIntakeError::bad_state;
 // The inherited support aggregate qualifies matching wire/component editions,
 // not the original expansion loader's legacy-save fixup path.
 if((w.game().data_version<11)!=(s.intake.variant==Variant::base))return StableIntakeError::unsupported_stage;
 WorldMineralResult next{};auto e=world_convoy_nutrients(w,t,b,r,id,s.intake,next.food);if(e!=StableIntakeError::ok)return e;
 auto supported=world_unit_support(w,d,t,id,next.support);if(supported!=LogicError::ok)return logic(supported);
 const auto*base=w.base(id);const auto*owner=base?w.faction((size_t)base->faction_id):nullptr;if(!base||!owner)return StableIntakeError::bad_state;
 const auto&g=w.game();const auto&p=g.preferences;auto&f=next.facts;
 f.intake=next.food.fresh.intake.intake[1];f.raw_eco_damage=next.food.fresh.intake.raw_eco_damage;
 f.convoy_to=next.food.convoys.to[1];f.convoy_from=next.food.convoys.from[1];f.maintenance=next.support.mineral_cost;
 f.population=base->pop_size;f.clean_minerals_modifier=owner->clean_minerals_modifier;f.satellites=owner->satellites_mineral;f.atrocities=owner->major_atrocities;
 f.expansion=s.intake.variant==Variant::alien_crossfire;
 if(s.runtime_tectonic){if(s.tectonic_count<0||(!f.expansion&&s.tectonic_count))return StableIntakeError::bad_state;f.tectonic_detonations=s.tectonic_count;}
 else if(f.expansion){if(g.data_version<12)return StableIntakeError::unsupported_stage;if(!raw_counter(w,(unsigned)base->faction_id,f.tectonic_detonations))return StableIntakeError::bad_state;}
 f.sea_level=g.map.info.sea_level;f.sea_level_rises=wr.sea_level_rises;f.native_life=g.map.info.world_settings[6];
 f.tech_ranking=owner->tech_ranking;f.transcendent_techs=owner->tech_count_transcendent;f.planet=owner->SE_planet_pending;f.faction_difficulty=owner->diff_level;f.global_difficulty=p.DiffLevel;f.queue_id=base->queue_items[0];
 f.human=(p.FactionStatus[0]&(1u<<(unsigned)base->faction_id))!=0;f.ascending=p.SecretProjects[31]!=-1;f.transcendence_enabled=((uint32_t)p.GameRules&0x800u)!=0;f.perihelion=((uint32_t)p.GameState&0x100000u)!=0;
 auto facility=[&](unsigned bit){return (base->facilities_built[bit/8]&(1u<<(bit%8)))!=0;};
 f.quantum_converter=facility(21);f.robotic_assembly=facility(19);f.genejack=facility(22);f.nanoreplicator=facility(20);f.centauri_preserve=facility(31);f.temple=facility(32);
 if(world_has_project(w,28,base->faction_id,f.singularity_inductor)!=LogicError::ok||world_has_project(w,29,base->faction_id,f.bulk_matter)!=LogicError::ok||world_has_project(w,16,base->faction_id,f.pholus_mutagen)!=LogicError::ok||world_has_project(w,27,base->faction_id,f.space_elevator)!=LogicError::ok)return StableIntakeError::bad_state;
 const auto mineral=calculate_minerals(f,next.minerals);if(mineral!=LogicError::ok)return logic(mineral);
 output=next;return StableIntakeError::ok;
}
}
