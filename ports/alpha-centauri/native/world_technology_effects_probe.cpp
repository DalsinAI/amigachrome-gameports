/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_technology_effects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world;static Rules rules;
struct Range{const void*p;size_t n,offset;};static Range ranges[11];
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long z=ftell(f);rewind(f);
 if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);
 if(!b){fclose(f);return nullptr;}const bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);
 if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static int run(int argc,char**argv){
 if(argc!=3&&argc!=4)return 20;
 world.clear();
 size_t n=0;auto*b=read(argv[1],n);if(!b)return 20;
 const auto parsed=read_technologies(b,n,rules);free(b);if(parsed!=Error::ok)return 20;
 TechnologyEffects out;memset(&out,0x5a,sizeof out);TechnologyEffects old;memcpy(&old,&out,sizeof out);
 if(world_technology_effects(world,rules,1,out)!=TechnologyEffectsError::bad_state||memcmp(&old,&out,sizeof out))return 21;
 b=read(argv[2],n);if(!b)return 20;const auto loaded=world.load(b,n,0x12345678);memset(b,0xff,n);free(b);if(loaded!=WorldLoadError::ok)return 20;
 if(argc==4){if(strcmp(argv[3],"bad-count"))return 20;rules.count=88;}
 const auto&g=world.game();Range inputs[]={{&world,sizeof world,0},{&rules,sizeof rules,0},
 {world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},
 {world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},
 {world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},
 {world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0}};
 size_t total=0;unsigned count=0;for(const auto&r:inputs)if(r.n){ranges[count++]={r.p,r.n,total};total+=r.n;}
 auto*baseline=(unsigned char*)malloc(total);if(!baseline)return 20;
 for(unsigned i=0;i<count;++i)memcpy(baseline+ranges[i].offset,ranges[i].p,ranges[i].n);
 for(unsigned i=0;i<count;++i){auto&alias=*reinterpret_cast<TechnologyEffects*>(const_cast<void*>(ranges[i].p));
  if(world_technology_effects(world,rules,1,alias)!=TechnologyEffectsError::bad_buffer)return 21;
  for(unsigned j=0;j<count;++j)if(memcmp(baseline+ranges[j].offset,ranges[j].p,ranges[j].n))return 21;
 }
 for(int faction=-1;faction<=8;++faction){
  memset(&out,0x5a,sizeof out);memcpy(&old,&out,sizeof out);
  const auto error=world_technology_effects(world,rules,faction,out);
  if(error!=TechnologyEffectsError::ok&&memcmp(&old,&out,sizeof out))return 21;
  uint32_t words[6]={(uint32_t)error,(uint32_t)out.fungus[0],(uint32_t)out.fungus[1],(uint32_t)out.fungus[2],(uint32_t)out.fungus[3],(uint32_t)out.commerce};
  for(uint32_t value:words){unsigned char bytes[4];for(unsigned i=0;i<4;++i)bytes[i]=(unsigned char)(value>>(8*i));if(fwrite(bytes,1,4,stdout)!=4)return 20;}
 }
 for(unsigned j=0;j<count;++j)if(memcmp(baseline+ranges[j].offset,ranges[j].p,ranges[j].n))return 21;
 free(baseline);return world.runtime_seed()==0x12345678?0:21;
}
int main(int argc,char**argv){
 if(argc==3&&!strcmp(argv[1],"--batch")){
  FILE*manifest=fopen(argv[2],"r");if(!manifest)return 20;
  char rule[256],save[256],mode[]="bad-count";int bad=0,status=0;
  while((status=fscanf(manifest,"%255s %255s %d",rule,save,&bad))==3){
   if(bad!=0&&bad!=1){fclose(manifest);return 20;}
   char*args[]={argv[0],rule,save,mode};const int rc=run(bad?4:3,args);
   if(rc){fclose(manifest);return rc;}
  }
  const bool ok=status==EOF&&!ferror(manifest);fclose(manifest);return ok?0:20;
 }
 return run(argc,argv);
}
