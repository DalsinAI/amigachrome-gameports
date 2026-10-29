/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_energy.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules,old_rules;static alpha::UnitDefinitions units,old_units;
static alpha::BasicRules basic,old_basic;static alpha::WorldRules world_rules,old_world_rules;
static alpha::ResourceInfo resources,old_resources;static alpha::CitizenDefinitions citizens,old_citizens;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t b[4];for (unsigned i=0;i<4;++i) { b[i]=(uint8_t)n;n>>=8; }return fwrite(b,1,4,stdout)==4; }
static bool mineral_words(const alpha::WorldMineralResult&r,uint32_t*words){
 const auto&v=r.food.fresh.intake;unsigned at=0;for(int32_t x:r.food.convoys.to)words[at++]=(uint32_t)x;for(int32_t x:r.food.convoys.from)words[at++]=(uint32_t)x;words[at++]=(uint32_t)r.food.convoys.harvesters;words[at++]=(uint32_t)r.food.convoys.terraform_reduction_after;words[at++]=(uint32_t)r.food.convoys.base_energy_after;for(int32_t x:v.satellites)words[at++]=(uint32_t)x;for(int32_t x:v.tiles)words[at++]=(uint32_t)x;for(int32_t x:v.intake)words[at++]=(uint32_t)x;words[at++]=v.worked_tiles;words[at++]=v.state_flags;words[at++]=(uint32_t)v.raw_eco_damage;words[at++]=(uint32_t)v.terraform_reduction_after;words[at++]=(uint32_t)v.base_energy_after;for(const auto&tile:v.tile_yields)for(int32_t x:tile)words[at++]=(uint32_t)x;const auto&nutrition=r.food.fresh.nutrients;const int32_t fields[]={nutrition.growth,nutrition.intake,nutrition.consumption,nutrition.surplus,nutrition.stored,nutrition.faction_surplus};for(int32_t x:fields)words[at++]=(uint32_t)x;
const int32_t support_values[]={r.support.support_rating,r.support.free_units,r.support.cost_per_unit,r.support.supported_units,r.support.maintained_units,r.support.clean_exempt,r.support.native_fungus_exempt,r.support.mineral_cost};for(int32_t x:support_values)words[at++]=(uint32_t)x;const int32_t fact_values[]={r.facts.intake,r.facts.convoy_to,r.facts.convoy_from,r.facts.maintenance,r.facts.raw_eco_damage,r.facts.population,r.facts.clean_minerals_modifier,r.facts.satellites,r.facts.atrocities,r.facts.tectonic_detonations,r.facts.sea_level,r.facts.sea_level_rises,r.facts.tech_ranking,r.facts.transcendent_techs,r.facts.planet,r.facts.native_life,r.facts.faction_difficulty,r.facts.global_difficulty,r.facts.queue_id};for(int32_t x:fact_values)words[at++]=(uint32_t)x;uint32_t flags=0;if(r.facts.human)flags|=1u<<0;if(r.facts.ascending)flags|=1u<<1;if(r.facts.transcendence_enabled)flags|=1u<<2;if(r.facts.perihelion)flags|=1u<<3;if(r.facts.quantum_converter)flags|=1u<<4;if(r.facts.robotic_assembly)flags|=1u<<5;if(r.facts.genejack)flags|=1u<<6;if(r.facts.nanoreplicator)flags|=1u<<7;if(r.facts.singularity_inductor)flags|=1u<<8;if(r.facts.bulk_matter)flags|=1u<<9;if(r.facts.centauri_preserve)flags|=1u<<10;if(r.facts.temple)flags|=1u<<11;if(r.facts.pholus_mutagen)flags|=1u<<12;if(r.facts.space_elevator)flags|=1u<<13;if(r.facts.expansion)flags|=1u<<14;words[at++]=flags;const int32_t minerals[]={r.minerals.intake,r.minerals.consumption,r.minerals.surplus,r.minerals.final_surplus,r.minerals.eco_damage,r.minerals.bonus,r.minerals.inefficiency};for(int32_t x:minerals)words[at++]=(uint32_t)x;return at==130;
}
int main(int argc,char**argv){
 using namespace alpha;if(argc!=4)return 20;const auto variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;
 size_t n;auto*data=read(argv[2],n);if(!data)return 20;
 const bool parsed=read_technologies(data,n,rules)==Error::ok&&read_unit_definitions(data,n,rules,variant,units)==Error::ok&&read_basic_rules(data,n,rules,variant,basic,world_rules)==Error::ok&&read_resource_info(data,n,resources)==Error::ok&&read_citizen_definitions(data,n,rules,variant,citizens)==Error::ok;free(data);if(!parsed)return 20;
 old_rules=rules;old_units=units;old_basic=basic;old_world_rules=world_rules;old_resources=resources;old_citizens=citizens;
 data=read(argv[3],n);if(!data)return 20;OriginalWorld w,baseline;if(w.load(data,n,123)!=WorldLoadError::ok||baseline.load(data,n,123)!=WorldLoadError::ok)return 20;
 unsigned guards=0;
 for(unsigned projection=0;projection<2;++projection)for(unsigned policy=0;policy<2;++policy){
  WorldEnergyStage s{{{variant,variant==Variant::alien_crossfire,false,{0,false},0,0},true,0},PoliceSnapshotProfile::offline_validated,CommerceEnergyStage::current_first_field_fresh_technology,projection,(AlienGridPolicy)policy};
  WorldEnergyResult out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);const auto error=world_energy(w,rules,basic,world_rules,resources,units,citizens,0,s,out);
  if(!emit((uint32_t)error))return 21;
  if(error!=StableIntakeError::ok){if(memcmp(&out,&before,sizeof out))return 21;for(unsigned i=0;i<278;++i)if(!emit(0x5a5a5a5a))return 21;}
  else{uint32_t words[130];if(!mineral_words(out.resources,words))return 21;for(auto v:words)if(!emit(v))return 21;
   const auto&c=out.commerce;if(!emit((uint32_t)c.variant)||!emit(c.base)||!emit(c.rank)||!emit(c.global_technology_count))return 21;for(auto v:c.partner)if(!emit(v))return 21;for(auto v:c.imports)if(!emit(v))return 21;for(auto v:c.exports)if(!emit(v))return 21;if(!emit(c.total)||!emit(c.qualified_foreign_commerce)||!emit(out.hq_distance))return 21;
   const auto&a=out.energy.accounting;const int32_t values[]={a.intake,a.consumption,a.inefficiency,a.surplus,a.economy,a.psych,a.labs,a.unknown_total,a.faction_surplus};for(auto v:values)if(!emit(v))return 21;for(auto v:a.counters)if(!emit(v))return 21;
   for(const auto&row:out.energy.population.rows)for(auto v:row)if(!emit(v))return 21;
   if(!emit(out.energy.population.talents)||!emit(out.energy.population.drones)||!emit(out.energy.population.superdrones))return 21;
   for(auto v:out.energy.population.social_psych)if(!emit(v))return 21;
   for(auto v:out.energy.specialist_types)if(!emit(v))return 21;
  }
  auto bad=[&](const OriginalWorld&src,size_t id,const WorldEnergyStage&stage,StableIntakeError expected){++guards;WorldEnergyResult r,old;memset(&r,0x5a,sizeof r);memcpy(&old,&r,sizeof r);return world_energy(src,rules,basic,world_rules,resources,units,citizens,id,stage,r)==expected&&!memcmp(&r,&old,sizeof r);};
  OriginalWorld absent;if(!bad(absent,0,s,StableIntakeError::bad_state)||!bad(w,w.game().base_count,s,StableIntakeError::bad_state))return 21;
  auto invalid=s;invalid.upkeep_stage=2;if(!bad(w,0,invalid,StableIntakeError::bad_state))return 21;invalid=s;invalid.commerce=(CommerceEnergyStage)1;if(!bad(w,0,invalid,StableIntakeError::bad_state))return 21;invalid=s;invalid.snapshot=(PoliceSnapshotProfile)1;if(!bad(w,0,invalid,StableIntakeError::bad_state))return 21;invalid=s;invalid.alien_grid_policy=(AlienGridPolicy)2;if(!bad(w,0,invalid,StableIntakeError::bad_state))return 21;
  const void*ranges[]={&w,&rules,&basic,&world_rules,&resources,&units,&citizens,&s,w.original_bytes(),w.tile(0),w.base(0),w.vehicle(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
  for(const void*p:ranges)if(p){++guards;auto&r=*reinterpret_cast<WorldEnergyResult*>(const_cast<void*>(p));if(world_energy(w,rules,basic,world_rules,resources,units,citizens,0,s,r)!=StableIntakeError::bad_buffer)return 21;}
 }
 if(!emit(guards)||!alpha_test::same_world(w,baseline)||memcmp(w.original_bytes(),data,n)||w.runtime_seed()!=123||memcmp(&rules,&old_rules,sizeof rules)||memcmp(&basic,&old_basic,sizeof basic)||memcmp(&world_rules,&old_world_rules,sizeof world_rules)||memcmp(&resources,&old_resources,sizeof resources)||memcmp(&units,&old_units,sizeof units)||memcmp(&citizens,&old_citizens,sizeof citizens))return 21;
 free(data);return 0;
}
