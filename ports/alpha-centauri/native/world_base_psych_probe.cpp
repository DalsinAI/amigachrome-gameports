/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_psych.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
using namespace alpha;
alignas(WorldPsychResult) static UnitDefinitions units;
alignas(WorldPsychResult) static BasicRules basic;
alignas(WorldPsychResult) static OriginalWorld world;
struct Range {const void*p;size_t n;};
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}n=(size_t)z;const bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
static bool number(const char*s,int32_t&out){char*end=nullptr;errno=0;const long v=strtol(s,&end,10);if(errno||!end||end==s||*end||v<(-2147483647L-1)||v>2147483647L)return false;out=(int32_t)v;return true;}
int main(int argc,char**argv){
 if(argc!=5)return 20;
 size_t n=0;auto*p=read(argv[1],n);if(!p)return 20;Rules tech{};WorldRules wr{};const bool parsed=read_technologies(p,n,tech)==Error::ok&&read_unit_definitions(p,n,tech,Variant::base,units)==Error::ok&&read_basic_rules(p,n,tech,Variant::base,basic,wr)==Error::ok;memset(p,0xff,n);free(p);if(!parsed)return 20;
 p=read(argv[2],n);if(!p)return 20;const auto loaded=world.load(p,n,0xabcde123);memset(p,0xff,n);free(p);if(loaded!=WorldLoadError::ok)return 20;
 WorldPsychStage stage{};if(!number(argv[3],stage.specialists)||!number(argv[4],stage.fresh_psych))return 20;
 const auto&g=world.game();const Range ranges[]={{&world,sizeof world},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&units,sizeof units},{&basic,sizeof basic},{&stage,sizeof stage}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}auto*before=(uint8_t*)malloc(total);if(!before)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(before+at,r.p,r.n);at+=r.n;}
 WorldPsychResult result;memset(&result,0x5a,sizeof result);WorldPsychResult sentinel;memcpy(&sentinel,&result,sizeof result);
 auto call=[&](size_t id,WorldPsychResult&out){return world_base_psych_policy(world,units,basic,id,stage,out);};
 for(const auto&r:ranges){if(!r.n)continue;const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets)if(call(0,*reinterpret_cast<WorldPsychResult*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset))!=UpkeepPsychError::bad_buffer)return 21;}
 if(call(g.base_count,result)!=UpkeepPsychError::bad_state||memcmp(&result,&sentinel,sizeof result))return 21;
 for(size_t id=0;id<g.base_count;++id){memset(&result,0x5a,sizeof result);const auto error=call(id,result);if(error!=UpkeepPsychError::ok&&memcmp(&result,&sentinel,sizeof result))return 21;printf("BASE %lu %d\n",(unsigned long)id,(int)error);if(error!=UpkeepPsychError::ok)continue;
  const auto&f=result.facts;const int32_t facts[]={f.population,f.specialists,f.content_population,f.base_limit,f.base_index,f.faction_base_count,f.difficulty,f.assimilation_turns,f.rule_drone,f.rule_talent,f.no_drone_bonus,f.pending_talent,f.pending_police,f.fresh_psych,f.genejack_drones,f.pacifism_units,f.police_effects[0],f.police_effects[1],f.police_effects[2],f.genejack,f.recreation_commons,f.hologram_or_virtual,f.research_hospital,f.nanohospital,f.paradise_garden,f.planetary_transit,f.nerve_stapled,f.punishment_sphere,f.telepathic_matrix,f.longevity_vaccine,f.human_genome,f.clinical_immortality,f.pending_economics};
  printf("FACTS %lu",(unsigned long)id);for(int32_t value:facts)printf(" %ld",(long)value);puts("");
  const auto&r=result.psych;printf("PSYCH %lu %ld %ld %ld %ld %ld %ld",(unsigned long)id,(long)r.talents,(long)r.drones,(long)r.superdrones,(long)r.efficiency_drones,(long)r.capture_drones,(long)r.police_total);for(const auto&row:r.rows)for(int32_t value:row)printf(" %ld",(long)value);printf(" %d %d\n",r.can_riot,r.will_riot);
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(before+at,r.p,r.n))return 21;at+=r.n;}free(before);return world.runtime_seed()==0xabcde123?0:21;
}
