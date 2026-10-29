/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_cost_factor.h"
#include "upstream_cost_factor_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static bool read32(FILE*f,uint32_t&v){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;v=0;for(unsigned i=0;i<4;++i)v|=(uint32_t)b[i]<<(8*i);return true;}
static bool write32(FILE*f,uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(v>>(8*i));return fwrite(b,1,4,f)==4;}
static bool leaf_checks(){
 CostFactorFacts f{1,10,0,0,0,0,true,false,false,false,true,false,false};int32_t out=0x5a5a5a5a;
 if(cost_factor(f,out)!=CostFactorError::ok||out!=8)return false;
 if(cost_factor(f,f.industry_pending)!=CostFactorError::bad_buffer||f.industry_pending)return false;
 f.multiplier=0;out=12345;if(cost_factor(f,out)!=CostFactorError::bad_state||out!=12345)return false;
 f.multiplier=10;f.resource=0;f.growth_pending=2147483647;f.children_creche=true;
 if(cost_factor(f,out)!=CostFactorError::overflow||out!=12345)return false;
 return true;
}
static int leaf(FILE*in,FILE*out){
 uint32_t count;if(!read32(in,count)||count>1000000)return 20;
 for(uint32_t i=0;i<count;++i){
  uint32_t x[7];for(auto&v:x)if(!read32(in,v))return 20;if(x[6]>127)return 20;
  const CostFactorFacts f{x[0],x[1],x[2],x[3],(int32_t)x[4],(int32_t)x[5],(x[6]&1u)!=0,(x[6]&2u)!=0,(x[6]&4u)!=0,(x[6]&8u)!=0,(x[6]&16u)!=0,(x[6]&32u)!=0,(x[6]&64u)!=0};
  int32_t result=0x5a5a5a5a;const auto error=cost_factor(f,result);
  if(error==CostFactorError::ok&&result!=cost_factor_reference::evaluate(f))return 21;
  if(!write32(out,(uint32_t)error)||!write32(out,(uint32_t)result))return 20;
 }
 return fgetc(in)==EOF?0:20;
}
static unsigned char*read_file(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long len=ftell(f);if(len<=0||len>64*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)len);if(!b){fclose(f);return nullptr;}n=(size_t)len;bool ok=fread(b,1,n,f)==n;fclose(f);if(!ok){free(b);return nullptr;}return b;}
static OriginalWorld world;static Rules technologies;static BasicRules basic;static WorldRules world_rules;
static int world_case(const char*rules,const char*save,FILE*out){
 size_t n=0;auto*b=read_file(rules,n);if(!b)return 20;bool parsed=read_technologies(b,n,technologies)==Error::ok&&read_basic_rules(b,n,technologies,Variant::base,basic,world_rules)==Error::ok;memset(b,0xff,n);free(b);if(!parsed)return 20;
 b=read_file(save,n);if(!b)return 20;const auto loaded=world.load(b,n,12345);memset(b,0xff,n);free(b);if(loaded!=WorldLoadError::ok)return 20;
 const auto&g=world.game();int32_t result=12345;
 struct Range{const void*p;size_t n;};
 const Range ranges[]={{&world,sizeof world},{&basic,sizeof basic},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}
 auto*before=(unsigned char*)malloc(total);if(!before)return 20;size_t at=0;
 for(const auto&r:ranges){if(r.n)memcpy(before+at,r.p,r.n);at+=r.n;}
 if(world_base_cost_factor(world,basic,g.base_count,1,result)!=CostFactorError::bad_state||result!=12345)return 21;
 const void*regions[]={&world,&basic,world.original_bytes(),world.tile(0),world.base(0),world.vehicle(0),world.prototype(0),world.faction(0),world.faction_definition(0)};
 for(const void*p:regions){if(!p)continue;auto&aliased=*static_cast<int32_t*>(const_cast<void*>(p));if(world_base_cost_factor(world,basic,0,1,aliased)!=CostFactorError::bad_buffer)return 21;}
 for(size_t id=0;id<g.base_count;++id)for(uint32_t resource=0;resource<4;++resource){
  result=0x5a5a5a5a;const auto error=world_base_cost_factor(world,basic,id,resource,result);
  if(error!=CostFactorError::ok&&result!=0x5a5a5a5a)return 21;
  fprintf(out,"FACTOR %lu %lu %u %ld\n",(unsigned long)id,(unsigned long)resource,(unsigned)error,(long)result);
 }
 for(int faction=1;faction<=7;++faction)for(uint32_t resource=0;resource<2;++resource){
  result=0x5a5a5a5a;const auto error=world_cost_factor(world,basic,faction,resource,-1,result);
  if(error!=CostFactorError::ok&&result!=0x5a5a5a5a)return 21;
  fprintf(out,"FACTION %d %lu %u %ld\n",faction,(unsigned long)resource,(unsigned)error,(long)result);
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(before+at,r.p,r.n))return 21;at+=r.n;}free(before);
 return ferror(out)?20:0;
}
int main(int argc,char**argv){
 if(!leaf_checks())return 21;
 if(argc==4&&!strcmp(argv[1],"leaf")){FILE*in=fopen(argv[2],"rb"),*out=fopen(argv[3],"wb");if(!in||!out)return 20;int rc=leaf(in,out);if(fclose(in)||fclose(out))return 20;return rc;}
 if(argc==5&&!strcmp(argv[1],"world")){FILE*out=fopen(argv[4],"wb");if(!out)return 20;int rc=world_case(argv[2],argv[3],out);if(fclose(out))return 20;return rc;}
 return 20;
}
