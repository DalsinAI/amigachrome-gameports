/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_psych.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;static alpha::UnitDefinitions units,old_units;static alpha::BasicRules basic;static alpha::WorldRules world_rules;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool emit(uint32_t n) { uint8_t b[4];for (unsigned i=0;i<4;++i) { b[i]=(uint8_t)n;n>>=8; }return fwrite(b,1,4,stdout)==4; }
int main(int argc,char** argv) {
 using namespace alpha;if(argc!=4)return 20;const auto variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;
 size_t n;auto* bytes=read(argv[2],n);if(!bytes)return 20;
 if(read_technologies(bytes,n,rules)!=Error::ok || read_unit_definitions(bytes,n,rules,variant,units)!=Error::ok || read_basic_rules(bytes,n,rules,variant,basic,world_rules)!=Error::ok){free(bytes);return 20;}free(bytes);
 const auto old_rules=rules;memcpy(&old_units,&units,sizeof units);const auto old_basic=basic;
 bytes=read(argv[3],n);if(!bytes)return 20;OriginalWorld w,baseline;
 if(w.load(bytes,n,123)!=WorldLoadError::ok || baseline.load(bytes,n,123)!=WorldLoadError::ok)return 20;
 const size_t count=w.game().base_count<8?w.game().base_count:8;
 for(size_t base=0;base<count;++base)for(unsigned stage=0;stage<2;++stage) {
  WorldPsychContext context{PoliceSnapshotProfile::offline_validated,stage,(int32_t)(base*13+stage*7)};WorldPsych out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);
  auto e=world_psych(w,rules,units,basic,base,context,out);if(!emit((uint32_t)e))return 21;
  if(e!=WorldPsychError::ok){if(memcmp(&out,&before,sizeof out))return 21;for(unsigned i=0;i<105;++i)if(!emit(0x5a5a5a5a))return 21;continue;}
  if(!emit((uint32_t)out.variant)||!emit(out.base)||!emit(out.map_factor)||!emit(out.pacifism)||!emit(out.garrison_count))return 21;
  for(const auto& row:out.population.rows)for(int32_t v:row)if(!emit((uint32_t)v))return 21;
  if(!emit(out.population.talents)||!emit(out.population.drones)||!emit(out.population.superdrones))return 21;
  for(int32_t v:out.population.social_psych)if(!emit((uint32_t)v))return 21;
 }
 WorldPsych out,before;memset(&out,0x5a,sizeof out);memcpy(&before,&out,sizeof out);WorldPsychContext c{PoliceSnapshotProfile::offline_validated,0,0};unsigned guards=0;
 auto bad=[&](const OriginalWorld& source,size_t base,WorldPsychError wanted){++guards;return world_psych(source,rules,units,basic,base,c,out)==wanted&&!memcmp(&out,&before,sizeof out);};
 OriginalWorld absent;if(!bad(absent,0,WorldPsychError::bad_state)||!bad(w,w.game().base_count,WorldPsychError::bad_state)||!bad(w,SIZE_MAX,WorldPsychError::bad_state))return 21;
 if(count){c.upkeep_stage=2;if(!bad(w,0,WorldPsychError::bad_state))return 21;c.upkeep_stage=0;c.snapshot=(PoliceSnapshotProfile)1;if(!bad(w,0,WorldPsychError::bad_state))return 21;c.snapshot=PoliceSnapshotProfile::offline_validated;
  const void* ranges[]={&w,&rules,&units,&basic,&c,w.original_bytes(),w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
  for(const void* p:ranges)if(p){++guards;auto& alias=*reinterpret_cast<WorldPsych*>(const_cast<void*>(p));if(world_psych(w,rules,units,basic,0,c,alias)!=WorldPsychError::bad_buffer)return 21;}
 }
 if(!emit(guards)||!alpha_test::same_world(w,baseline)||w.runtime_seed()!=123||memcmp(w.original_bytes(),bytes,n)||memcmp(&old_rules,&rules,sizeof rules)||memcmp(&old_units,&units,sizeof units)||memcmp(&old_basic,&basic,sizeof basic))return 21;
 free(bytes);return 0;
}
