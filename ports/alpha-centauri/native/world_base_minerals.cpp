/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned bindings for OpenSMACX base_minerals/ascending, pin
 * 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Save-version defaults derive from
 * Thinker savegame.cpp, MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#include "world_base_minerals.h"
#include "world_unit_stats.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const Rules&t,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,const UpkeepWorldMineralStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool built(const OriginalBase&b,unsigned id){return (b.facilities_built[id/8]&(1u<<(id%8)))!=0;}
bool project(const OriginalWorld&w,unsigned id,int32_t faction,bool&out){const int32_t location=w.game().preferences.SecretProjects[id];return location>=-2&&world_has_project(w,id,faction,out)==LogicError::ok;}
uint32_t le(const uint8_t*p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
}
StableIntakeError world_base_minerals(const OriginalWorld&w,const Rules&t,const BasicRules&b,const WorldRules&wr,const ResourceInfo&r,size_t id,const UpkeepWorldMineralStage&s,UpkeepWorldMineralResult&output){
 if(aliases(w,t,b,wr,r,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded()||s.forces_maintenance<0||wr.sea_level_rises<1||wr.sea_level_rises>100||
    (s.intake.variant!=Variant::base&&s.intake.variant!=Variant::alien_crossfire)||
    ((w.game().data_version<11)!=(s.intake.variant==Variant::base))||
    (s.intake.variant==Variant::base&&s.intake.expansion_enabled))return StableIntakeError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<0||base->faction_id>7)return StableIntakeError::bad_state;
 const auto*owner=w.faction((size_t)base->faction_id);if(!owner)return StableIntakeError::bad_state;
 UpkeepWorldMineralResult next{};
 auto error=world_convoy_flows(w,t,b,r,id,s.intake,next.convoys);if(error!=StableIntakeError::ok)return error;
 auto intake=s.intake;intake.terraform_reduction_before=next.convoys.terraform_reduction_after;intake.base_energy_before=next.convoys.base_energy_after;
 error=world_stable_intake(w,t,b,r,id,intake,next.fresh);if(error!=StableIntakeError::ok)return error;
 const CurrentMineralInputs inputs{s.intake.variant,next.fresh.intake[1],next.convoys.to[1],next.convoys.from[1],s.forces_maintenance,next.fresh.raw_eco_damage};WorldMineralAccounting account{};
 error=world_mineral_accounting(w,wr,id,inputs,account);if(error!=StableIntakeError::ok)return error;
 next.facts=account.facts;next.minerals=account.minerals;output=next;return StableIntakeError::ok;
}
StableIntakeError world_mineral_accounting(const OriginalWorld&w,const WorldRules&wr,size_t id,const CurrentMineralInputs&s,WorldMineralAccounting&output){
 const void*out=&output;const size_t n=sizeof output;
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&wr,sizeof wr)||overlap(out,n,&s,sizeof s))return StableIntakeError::bad_buffer;
 if(w.loaded()){
  const auto&g=w.game();if(overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return StableIntakeError::bad_buffer;
 }
 if(!w.loaded()||s.forces_maintenance<0||wr.sea_level_rises<1||wr.sea_level_rises>100||(s.variant!=Variant::base&&s.variant!=Variant::alien_crossfire)||((w.game().data_version<11)!=(s.variant==Variant::base)))return StableIntakeError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<0||base->faction_id>7)return StableIntakeError::bad_state;
 const auto*owner=w.faction((size_t)base->faction_id);if(!owner)return StableIntakeError::bad_state;
 WorldMineralAccounting next{};auto&f=next.facts;f.fresh_intake=s.fresh_intake;f.convoy_to=s.convoy_to;f.convoy_from=s.convoy_from;f.forces_maintenance=s.forces_maintenance;
 f.raw_eco_damage=s.raw_eco_damage;f.planet_ecology=owner->clean_minerals_modifier;f.satellites=owner->satellites_mineral;f.population=base->pop_size;
 f.major_atrocities=owner->major_atrocities;f.sea_level=w.game().map.info.sea_level;f.sea_level_rises=wr.sea_level_rises;
 f.faction_difficulty=owner->diff_level;f.game_difficulty=w.game().preferences.DiffLevel;f.tech_ranking=owner->tech_ranking;f.transcendent_techs=owner->tech_count_transcendent;f.planet_pending=owner->SE_planet_pending;f.native_life=w.game().map.info.world_settings[6];
 if(w.game().data_version>=12){const auto span=w.game().tectonic;if(!span.data||span.size!=32)return StableIntakeError::bad_state;const uint32_t value=le(span.data+(unsigned)base->faction_id*4);if(value>2147483647u)return StableIntakeError::overflow;f.tectonic_detonations=(int32_t)value;}
 f.quantum_converter=built(*base,21);f.robotic_assembly=built(*base,19);f.genejack=built(*base,22);f.nanoreplicator=built(*base,20);f.centauri_preserve=built(*base,31);f.temple=built(*base,32);
 if(!project(w,28,base->faction_id,f.singularity_inductor)||!project(w,29,base->faction_id,f.bulk_matter)||!project(w,16,base->faction_id,f.pholus))return StableIntakeError::bad_state;
 f.human=(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)base->faction_id))!=0;
 const int32_t voice=w.game().preferences.SecretProjects[31];if(voice< -2||(voice>=0&&!w.base((size_t)voice)))return StableIntakeError::bad_state;
 f.ascending_transcendence=voice!=-1&&((uint32_t)w.game().preferences.GameRules&0x800u);f.perihelion=((uint32_t)w.game().preferences.GameState&0x100000u)!=0;
 bool elevator=false;if(!project(w,27,base->faction_id,elevator))return StableIntakeError::bad_state;
 const int32_t item=base->queue_items[0];const int32_t first=s.variant==Variant::base?34:65;f.space_elevator_orbital=elevator&&item<=-first&&item>=-(first+3);
 const auto mineral=base_minerals(f,next.minerals);if(mineral==MineralError::overflow)return StableIntakeError::overflow;if(mineral!=MineralError::ok)return StableIntakeError::bad_state;
 output=next;return StableIntakeError::ok;
}
}
