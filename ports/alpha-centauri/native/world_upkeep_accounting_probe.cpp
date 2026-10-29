/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_upkeep_accounting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
using namespace alpha;
alignas(WorldUpkeepAccountingResult) static Rules technologies;
alignas(WorldUpkeepAccountingResult) static UnitDefinitions units;
alignas(WorldUpkeepAccountingResult) static BasicRules basic;
alignas(WorldUpkeepAccountingResult) static WorldRules world_rules;
alignas(WorldUpkeepAccountingResult) static ResourceInfo resources;
alignas(WorldUpkeepAccountingResult) static UpkeepCitizenDefinitions citizens;
alignas(WorldUpkeepAccountingResult) static OriginalWorld world;
struct Range {const void*p;size_t n,offset;};
static Range ranges[16];static size_t range_count;static uint8_t*baseline;
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}const bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool number(const char*s,int32_t&out){char*end=nullptr;errno=0;const long value=strtol(s,&end,10);if(errno||!end||end==s||*end||value<(-2147483647L-1)||value>2147483647L)return false;out=(int32_t)value;return true;}
static WorldUpkeepAccountingResult&alias(void*p){return *static_cast<WorldUpkeepAccountingResult*>(p);}
static bool unchanged(){for(size_t i=0;i<range_count;++i)if(memcmp(baseline+ranges[i].offset,ranges[i].p,ranges[i].n))return false;return true;}
static void values(const char*label,const int32_t*p,size_t n){printf("%s",label);for(size_t i=0;i<n;++i)printf(" %ld",(long)p[i]);puts("");}
int main(int argc,char**argv){
 if(argc!=6&&argc!=7)return 20;
 size_t n=0;auto*p=read(argv[1],n);if(!p)return 20;
 const bool parsed=read_technologies(p,n,technologies)==Error::ok&&read_unit_definitions(p,n,technologies,Variant::base,units)==Error::ok&&read_basic_rules(p,n,technologies,Variant::base,basic,world_rules)==Error::ok&&read_resource_info(p,n,resources)==Error::ok&&read_citizens(p,n,technologies,citizens)==Error::ok;memset(p,0xff,n);free(p);if(!parsed)return 20;
 int32_t base,commerce,grid;if(!number(argv[3],base)||base<0||!number(argv[4],commerce)||!number(argv[5],grid))return 20;
 WorkerResolutionStage stage{{1,false,false},commerce,grid};if(argc==7){if(!strcmp(argv[6],"inspection"))stage.worker.upkeep_state=0;else if(!strcmp(argv[6],"editor"))stage.worker.editor_active=true;else if(!strcmp(argv[6],"control"))stage.worker.control_upkeep=true;else return 20;}
 WorldUpkeepAccountingResult out,sentinel;memset(&out,0x5a,sizeof out);memcpy(&sentinel,&out,sizeof out);
 auto call=[&](size_t id,WorldUpkeepAccountingResult&result,uint32_t*states,size_t capacity){return world_upkeep_accounting(world,technologies,units,basic,world_rules,resources,citizens,id,stage,result,states,capacity);};
 if(call(0,out,nullptr,0)!=StableIntakeError::bad_state||memcmp(&out,&sentinel,sizeof out))return 21;
 p=read(argv[2],n);if(!p)return 20;const auto loaded=world.load(p,n,0xabcde123);memset(p,0xff,n);free(p);if(loaded!=WorldLoadError::ok)return 20;
 const auto&g=world.game();const Range input[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{&technologies,sizeof technologies,0},{&units,sizeof units,0},{&basic,sizeof basic,0},{&world_rules,sizeof world_rules,0},{&resources,sizeof resources,0},{&citizens,sizeof citizens,0},{&stage,sizeof stage,0}};
 size_t total=0;for(const auto&r:input){if(!r.n)continue;if(r.n>SIZE_MAX-total)return 20;ranges[range_count++]={r.p,r.n,total};total+=r.n;}baseline=(uint8_t*)malloc(total);if(!baseline)return 20;for(size_t i=0;i<range_count;++i)memcpy(baseline+ranges[i].offset,ranges[i].p,ranges[i].n);
 const size_t count=g.vehicle_count;auto*states=(uint32_t*)malloc((count+3)*sizeof(uint32_t));if(!states)return 20;for(size_t i=0;i<count+3;++i)states[i]=0x6b6b6b6bu;
 auto reject=[&](size_t id,uint32_t*actors,size_t capacity,StableIntakeError want){const auto e=call(id,out,actors,capacity);if(e!=want||memcmp(&out,&sentinel,sizeof out)||!unchanged())return false;for(size_t i=0;i<count+3;++i)if(states[i]!=0x6b6b6b6bu)return false;return true;};
 if(stage.worker.upkeep_state==1&&!stage.worker.editor_active&&!reject(g.base_count,states,count,StableIntakeError::bad_state))return 21;
 if(!reject((size_t)base,states,SIZE_MAX,StableIntakeError::bad_buffer))return 21;
 if(count&&(!reject((size_t)base,nullptr,count,StableIntakeError::bad_buffer)||!reject((size_t)base,states,count-1,StableIntakeError::bad_buffer)))return 21;
 for(size_t i=0;i<range_count;++i){const auto&r=ranges[i];const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets){void*where=const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset;if(call((size_t)base,alias(where),states,count)!=StableIntakeError::bad_buffer||!unchanged())return 21;if(call((size_t)base,out,static_cast<uint32_t*>(where),1)!=StableIntakeError::bad_buffer||!unchanged())return 21;}}
 if(call((size_t)base,alias(states),states,count+3)!=StableIntakeError::bad_buffer||!unchanged())return 21;
 const auto error=call((size_t)base,out,states,count+3);if(error!=StableIntakeError::ok){if(memcmp(&out,&sentinel,sizeof out))return 21;for(size_t i=0;i<count+3;++i)if(states[i]!=0x6b6b6b6bu)return 21;}for(size_t i=count;i<count+3;++i)if(states[i]!=0x6b6b6b6bu)return 21;
 if(!count){WorldUpkeepAccountingResult null_result;memcpy(&null_result,&sentinel,sizeof null_result);if(call((size_t)base,null_result,nullptr,0)!=error||memcmp(&null_result,&out,sizeof out)||!unchanged())return 21;}
 printf("BASE %u\n",(unsigned)error);
 if(error==StableIntakeError::ok){const auto&r=out.resolved;const auto&s=r.candidate.support;const int32_t support[]={s.supported,s.paid_units,s.mineral_cost,s.pacifism_units,s.drone_units,s.free_skips,s.free_allowance,s.cost_per_paid_unit};values("SUPPORT",support,8);
  printf("ASSIGN %ld %ld %lu %lu %lu %lu %lu %u\n",(long)r.assignment.population,(long)r.assignment.specialist_count,(unsigned long)r.assignment.worked_tiles,(unsigned long)r.assignment.specialist_types[0],(unsigned long)r.assignment.specialist_types[1],(unsigned long)r.specialist_adjust,(unsigned long)r.trimmed_workers,(unsigned)r.requires_riot_event);
  const auto&i=r.intake;printf("INTAKE");for(int32_t v:i.satellites)printf(" %ld",(long)v);for(int32_t v:i.tiles)printf(" %ld",(long)v);for(int32_t v:i.intake)printf(" %ld",(long)v);printf(" %lu %lu %ld %ld %ld",(unsigned long)i.worked_tiles,(unsigned long)i.state_flags,(long)i.raw_eco_damage,(long)i.terraform_reduction_after,(long)i.base_energy_after);for(const auto&tile:i.tile_yields)for(int32_t v:tile)printf(" %ld",(long)v);puts("");
  const auto&m=r.mineral.minerals;const int32_t minerals[]={m.intake,m.consumption,m.surplus,m.surplus_final,m.eco_damage};values("MINERAL",minerals,5);
  const auto&f=r.nutrient;const int32_t food[]={f.growth,f.intake,f.consumption,f.surplus,f.stored,f.faction_surplus};values("NUTRIENT",food,6);
  const auto&e=r.energy;printf("ENERGY %ld %ld %u %ld %ld %ld %ld",(long)e.headquarters_distance,(long)e.pending_efficiency,(unsigned)e.physical_creche,(long)e.accounting.intake,(long)e.accounting.consumption,(long)e.accounting.inefficiency,(long)e.accounting.surplus);for(int32_t v:e.accounting.social_efficiency_after)printf(" %ld",(long)v);puts("");
  const auto&a=r.allocation.allocation;printf("ALLOCATION %ld %ld %ld %ld %ld %ld %ld %ld %lu %lu %ld %ld %ld\n",(long)a.distributed_energy,(long)a.economy,(long)a.psych,(long)a.labs,(long)a.unknown_total,(long)a.coefficient_economy,(long)a.coefficient_psych,(long)a.coefficient_labs,(unsigned long)a.specialist_types[0],(unsigned long)a.specialist_types[1],(long)a.specialist_economy,(long)a.specialist_psych,(long)a.specialist_labs);
  const auto&d=r.psych.psych;printf("PSYCH %ld %ld %ld %ld %ld %ld",(long)d.talents,(long)d.drones,(long)d.superdrones,(long)d.efficiency_drones,(long)d.capture_drones,(long)d.police_total);for(const auto&row:d.rows)for(int32_t v:row)printf(" %ld",(long)v);printf(" %u %u\n",(unsigned)d.can_riot,(unsigned)d.will_riot);
  printf("SHORTFALL %lu",(unsigned long)out.energy_shortfall.count);for(uint32_t v:out.energy_shortfall.cancel)printf(" %lu",(unsigned long)v);puts("");printf("STATES %lu",(unsigned long)count);for(size_t i=0;i<count;++i)printf(" %lu",(unsigned long)states[i]);puts("");
 }
 const bool same=unchanged()&&world.runtime_seed()==0xabcde123;free(states);free(baseline);return same?0:21;
}
