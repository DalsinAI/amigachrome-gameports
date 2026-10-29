/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_drone_transition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world;
static Rules technologies;static BasicRules basic;static WorldRules world_rules;
static UnitDefinitions units;static FacilityDefinitions facilities;
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed32(uint32_t n){return n<=0x7fffffffu?(int32_t)n:-1-(int32_t)~n;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static uint8_t* read(const char* name,size_t& n){FILE*f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long size=ftell(f);rewind(f);if(size<=0||size>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
static bool write(const char* name,const uint8_t*p,size_t n){FILE*f=fopen(name,"wb");if(!f)return false;const bool ok=fwrite(p,1,n,f)==n;return fclose(f)==0&&ok;}
static uint32_t hash(const void*p,size_t n){const auto*b=(const uint8_t*)p;uint32_t h=2166136261u;while(n--)h=(h^*b++)*16777619u;return h;}
static DroneTransitionResult* result_alias(void*p){return static_cast<DroneTransitionResult*>(p);}
static WorldDroneResult* world_alias(void*p){return static_cast<WorldDroneResult*>(p);}
static CombatRandomState* state_alias(void*p){return static_cast<CombatRandomState*>(p);}
static void vector(uint32_t*v,DroneTransitionError e,uint32_t seed,const DroneTransitionResult&r){
 v[0]=(uint32_t)e;v[1]=seed;
 if(e!=DroneTransitionError::ok){for(unsigned i=2;i<16;++i)v[i]=0x5a5a5a5a;return;}
 v[2]=r.state_flags;v[3]=r.random_draws;v[4]=r.facility_attempts;v[5]=(uint32_t)r.built;v[6]=(uint32_t)(r.built>>32);v[7]=(uint32_t)r.destroyed_facility;
 v[8]=r.notice_count;for(unsigned i=0;i<3;++i)v[9+i]=(uint32_t)r.notices[i];
 v[12]=r.golden_age;v[13]=r.rioting;v[14]=r.recompute_required;v[15]=r.repaint_required;
}
static void facts_vector(uint32_t*v,const DroneTransitionFacts&f,uint32_t seed){
 const int32_t scalars[]={f.population,f.talents,f.drones,f.superdrones,f.pending_police,f.assimilation_turns,f.owned_bases,f.difficulty,f.ranking};
 for(unsigned i=0;i<9;++i)v[i]=(uint32_t)scalars[i];
 v[9]=f.state_flags;v[10]=f.governor_flags;
 v[11]=(uint32_t)f.built;v[12]=(uint32_t)(f.built>>32);v[13]=(uint32_t)f.protected_facilities;v[14]=(uint32_t)(f.protected_facilities>>32);v[15]=f.nerve_stapled;v[16]=f.telepathic_matrix;v[17]=seed;
}
static int leaf(const char*input,const char*output){
 size_t n=0;auto*p=read(input,n);if(!p||n%72)return 20;const size_t count=n/72;
 auto*bytes=(uint8_t*)malloc(count*64);if(!bytes)return 20;
 for(size_t i=0;i<count;++i){
  uint32_t v[18];for(unsigned j=0;j<18;++j)v[j]=get(p+i*72+j*4);
  DroneTransitionFacts f{};f.population=signed32(v[0]);f.talents=signed32(v[1]);f.drones=signed32(v[2]);f.superdrones=signed32(v[3]);
  f.pending_police=signed32(v[4]);f.assimilation_turns=signed32(v[5]);f.owned_bases=signed32(v[6]);f.difficulty=signed32(v[7]);f.ranking=signed32(v[8]);
  f.state_flags=v[9];f.governor_flags=v[10];f.built=(uint64_t)v[11]|((uint64_t)v[12]<<32);f.protected_facilities=(uint64_t)v[13]|((uint64_t)v[14]<<32);f.nerve_stapled=v[15]!=0;f.telepathic_matrix=v[16]!=0;
  const auto before=f;CombatRandomState rng{v[17]};DroneTransitionResult result{};memset(&result,0x5a,sizeof result);uint8_t old[sizeof result];memcpy(old,&result,sizeof result);
  if(drone_transition(f,rng,*result_alias(&f))!=DroneTransitionError::bad_buffer ||
     drone_transition(f,rng,*result_alias((uint8_t*)&f+8))!=DroneTransitionError::bad_buffer ||
     drone_transition(f,*state_alias(&f),result)!=DroneTransitionError::bad_buffer ||
     drone_transition(f,*state_alias(&result),result)!=DroneTransitionError::bad_buffer || memcmp(&f,&before,sizeof f)||rng.state!=v[17]||memcmp(&result,old,sizeof result))return 21;
  const auto error=drone_transition(f,rng,result);
  if(memcmp(&f,&before,sizeof f)||(error!=DroneTransitionError::ok&&(rng.state!=v[17]||memcmp(&result,old,sizeof result))))return 21;
  uint32_t values[16];vector(values,error,rng.state,result);for(unsigned j=0;j<16;++j)put(bytes+i*64+j*4,values[j]);
 }
 const bool ok=write(output,bytes,count*64);free(p);free(bytes);return ok?0:21;
}
static int connected(char**argv){
 size_t rn=0,n=0;auto*text=read(argv[2],rn);auto*data=read(argv[3],n);if(!text||!data)return 20;
 if(read_technologies(text,rn,technologies)!=Error::ok||read_basic_rules(text,rn,technologies,Variant::base,basic,world_rules)!=Error::ok||
    read_unit_definitions(text,rn,technologies,Variant::base,units)!=Error::ok||read_facility_definitions(text,rn,technologies,Variant::base,facilities)!=Error::ok||
    world.load(data,n,0x11223344u)!=WorldLoadError::ok)return 20;
 const size_t base=(size_t)strtoul(argv[4],nullptr,10);const uint32_t seed=(uint32_t)strtoul(argv[5],nullptr,10);
 WorldDroneStage stage{};const auto*b=world.base(base);stage.assignment.population=b?b->pop_size:0;stage.fresh_psych=atoi(argv[6]);stage.assignment.specialist_count=atoi(argv[7]);
 stage.assignment.worked_tiles=(uint32_t)strtoul(argv[8],nullptr,10);stage.assignment.specialist_types[0]=(uint32_t)strtoul(argv[9],nullptr,10);stage.assignment.specialist_types[1]=(uint32_t)strtoul(argv[10],nullptr,10);
 CombatRandomState rng{seed};WorldDroneResult result{};memset(&result,0x5a,sizeof result);uint8_t old[sizeof result];memcpy(old,&result,sizeof result);
 const void* objects[]={&technologies,&basic,&units,&facilities,&stage,world.original_bytes(),world.base(0),world.vehicle(0),world.faction(0),world.faction_definition(0),world.tile(0),world.prototype(0),&world};
 const size_t sizes[]={sizeof technologies,sizeof basic,sizeof units,sizeof facilities,sizeof stage,n,world.game().base_count*sizeof(OriginalBase),world.game().vehicle_count*sizeof(OriginalVehicle),8*sizeof(OriginalFactionState),8*sizeof(OriginalFactionDefinition),world.game().map.info.tile_count*sizeof(OriginalMapTile),512*sizeof(OriginalPrototype),sizeof world};
 uint32_t hashes[13];for(unsigned i=0;i<13;++i)hashes[i]=hash(objects[i],sizes[i]);
 for(unsigned i=0;i<13;++i){if(world_drone_transition(world,technologies,units,basic,facilities,base,stage,rng,*world_alias(const_cast<void*>(objects[i])))!=DroneTransitionError::bad_buffer ||
    world_drone_transition(world,technologies,units,basic,facilities,base,stage,*state_alias(const_cast<void*>(objects[i])),result)!=DroneTransitionError::bad_buffer)return 21;}
 for(unsigned i=0;i<13;++i)if(hashes[i]!=hash(objects[i],sizes[i]))return 21;
 if(rng.state!=seed||memcmp(&result,old,sizeof result))return 21;
 const auto error=world_drone_transition(world,technologies,units,basic,facilities,base,stage,rng,result);
 if(error!=DroneTransitionError::ok&&(rng.state!=seed||memcmp(&result,old,sizeof result)))return 21;
 for(unsigned i=0;i<13;++i)if(hashes[i]!=hash(objects[i],sizes[i]))return 21;
 FILE*f=fopen(argv[11],"wb");if(!f)return 21;fprintf(f,"WORLD %u %lu\n",(unsigned)error,(unsigned long)rng.state);
 if(error==DroneTransitionError::ok){
  uint32_t facts[18];facts_vector(facts,result.facts,seed);fputs("FACTS",f);for(auto value:facts)fprintf(f," %lu",(unsigned long)value);fputc('\n',f);
  uint32_t values[16];vector(values,error,rng.state,result.transition);fputs("TRANS",f);for(auto value:values)fprintf(f," %lu",(unsigned long)value);fputc('\n',f);
 }
 const bool ok=fclose(f)==0;free(text);free(data);return ok?0:21;
}
int main(int argc,char**argv){
 const bool pure=argc==4&&!strcmp(argv[1],"leaf"),query=argc==12&&!strcmp(argv[1],"world");
 if(!pure&&!query){puts("Usage: drone-probe leaf INPUT NEW_OUTPUT | world RULES WORLD BASE CRT_SEED PSYCH SPECIALISTS MASK TYPES0 TYPES1 NEW_OUTPUT");return 20;}
 const char*name=argv[pure?3:11];FILE*exists=fopen(name,"rb");if(exists){fclose(exists);return 20;}
 return pure?leaf(argv[2],argv[3]):connected(argv);
}
