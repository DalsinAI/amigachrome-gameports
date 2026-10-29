/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_population.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static Rules technologies;static BasicRules basic;static WorldRules wr;static ResourceInfo resources;
static unsigned char*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool load(const char*path,OriginalWorld&w){size_t n=0;auto*b=read(path,n);if(!b)return false;const auto e=w.load(b,n,0xabcdef01);memset(b,0xee,n);free(b);return e==WorldLoadError::ok;}
static uint32_t hash(const OriginalWorld&w){uint32_t h=2166136261u;const auto&g=w.game();const void*p[]={w.original_bytes(),w.tile(0),w.base(0),w.vehicle(0),w.prototype(0),w.faction(0),w.faction_definition(0)};const size_t n[]={w.original_size(),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),(size_t)g.base_count*sizeof(OriginalBase),(size_t)g.vehicle_count*sizeof(OriginalVehicle),512*sizeof(OriginalPrototype),8*sizeof(OriginalFactionState),8*sizeof(OriginalFactionDefinition)};for(unsigned k=0;k<7;++k)for(size_t i=0;i<n[k];++i)h=(h^((const uint8_t*)p[k])[i])*16777619u;return h;}
static WorldPopulationProposal&aliased(void*p){return *static_cast<WorldPopulationProposal*>(p);}
int main(int argc,char**argv){
 if(argc<5||argc>7)return 20;
 size_t n=0;auto*raw=read(argv[1],n);if(!raw)return 20;
 const bool parsed=read_technologies(raw,n,technologies)==Error::ok&&read_basic_rules(raw,n,technologies,Variant::base,basic,wr)==Error::ok&&read_resource_info(raw,n,resources)==Error::ok;free(raw);if(!parsed)return 20;
 OriginalWorld before,after;if(!load(argv[2],before))return 20;const size_t id=(size_t)strtoul(argv[3],nullptr,10);const bool full=atoi(argv[4])!=0;
 WorldPopulationStage stage{{Variant::base,false,false,{0,false},0,0},full};if(argc==7){if(!strcmp(argv[6],"upkeep"))stage.intake.worker.upkeep_state=1;else if(!strcmp(argv[6],"editor"))stage.intake.worker.editor_active=true;else return 20;}
 WorldPopulationProposal out,old;memset(&out,0x5a,sizeof out);memcpy(&old,&out,sizeof out);const auto a=hash(before);
 StableIntakeError e;if(argc>=6&&strcmp(argv[5],"-")){if(!load(argv[5],after))return 20;const auto h=hash(after);e=world_population_finish(before,after,technologies,basic,resources,id,stage,out);if(hash(after)!=h)return 21;}else e=world_population_begin(before,technologies,basic,resources,id,stage,out);
 if(hash(before)!=a||(e!=StableIntakeError::ok&&memcmp(&out,&old,sizeof out)))return 21;
 const auto&g=before.game();
 void*aliases[]={&stage,&technologies,&basic,&resources,&before,const_cast<uint8_t*>(before.original_bytes()),const_cast<OriginalBase*>(before.base(0)),const_cast<OriginalFactionState*>(before.faction(0)),const_cast<OriginalFactionDefinition*>(before.faction_definition(0)),const_cast<OriginalPrototype*>(before.prototype(0)),const_cast<OriginalMapTile*>(before.tile(0))};
 for(void*p:aliases){if(!p)continue;if(world_population_begin(before,technologies,basic,resources,id,stage,aliased(p))!=StableIntakeError::bad_buffer||hash(before)!=a)return 21;if(after.loaded()&&world_population_finish(before,after,technologies,basic,resources,id,stage,aliased(p))!=StableIntakeError::bad_buffer)return 21;}
 if(g.vehicle_count&&world_population_begin(before,technologies,basic,resources,id,stage,aliased(const_cast<OriginalVehicle*>(before.vehicle(0))))!=StableIntakeError::bad_buffer)return 21;
 if(after.loaded()&&world_population_finish(before,after,technologies,basic,resources,id,stage,aliased(&after))!=StableIntakeError::bad_buffer)return 21;
 printf("%u",(unsigned)e);if(e==StableIntakeError::ok){
 const auto&f=out.facts;const auto&p=out.transition;const auto&food=out.food.fresh.nutrients;
 const int32_t fields[]={f.population,f.accumulated,f.surplus,f.nutrient_factor,f.growth,f.population_limit,f.nutrient_per_citizen,f.habitation_dome,f.cloning_vats,f.full_turn,f.objective,p.population,p.accumulated,p.recompute,p.append_recomputed_surplus,p.removed,p.recomputed_accumulation_ceiling,out.recompute_required,out.starvation,(int32_t)out.convoy_cancellation_count,food.growth,food.intake,food.consumption,food.surplus,food.stored,food.faction_surplus};for(auto v:fields)printf(" %ld",(long)v);for(uint32_t v:out.convoy_cancellations)printf(" %lu",(unsigned long)v);
 }puts("");return 0;
}
