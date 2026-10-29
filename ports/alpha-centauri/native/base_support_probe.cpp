/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static bool read32(FILE*f,uint32_t&v){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;v=0;for(unsigned i=0;i<4;++i)v|=(uint32_t)b[i]<<(8*i);return true;}
static bool write32(FILE*f,uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(v>>(8*i));return fwrite(b,1,4,f)==4;}
static __attribute__((noinline)) SupportResult& overlapping_result(void*p){return *static_cast<SupportResult*>(p);}
static bool result(FILE*f,SupportError e,const SupportResult&r,const uint32_t*s,size_t n){
 const int32_t fields[]={r.supported,r.paid_units,r.mineral_cost,r.pacifism_units,r.drone_units,r.free_skips,r.free_allowance,r.cost_per_paid_unit};
 if(!write32(f,(uint32_t)e))return false;
 for(int32_t x:fields)if(!write32(f,(uint32_t)x))return false;
 for(size_t i=0;i<n;++i)if(!write32(f,s[i]))return false;
 return true;
}
static bool pure_checks(){
 SupportFacts f{-4,-3,4};SupportUnit u{0xffffffffu,0,1,0,true,false,false,true};SupportResult r{},old{};uint32_t state=0;
 if(base_support_policy(f,&u,1,r,&state,1)!=SupportError::ok||r.mineral_cost!=2||r.free_skips!=1||state!=(0xffffffffu&~0x200000u))return false;
 memset(&r,0x5a,sizeof r);memcpy(&old,&r,sizeof r);state=0x12345678;
 if(base_support_policy(f,&u,1,r,&state,0)!=SupportError::bad_buffer||memcmp(&r,&old,sizeof r)||state!=0x12345678)return false;
 f.population=-1;if(base_support_policy(f,&u,1,r,&state,1)!=SupportError::bad_state||memcmp(&r,&old,sizeof r)||state!=0x12345678)return false;f.population=4;
 if(base_support_policy(f,&u,1,r,&u.state,1)!=SupportError::bad_buffer||memcmp(&r,&old,sizeof r)||u.state!=0xffffffffu)return false;
 if(base_support_policy(f,&u,1,r,(uint32_t*)&r,1)!=SupportError::bad_buffer||memcmp(&r,&old,sizeof r))return false;
 if(base_support_policy(f,&u,1,overlapping_result(&f),&state,1)!=SupportError::bad_buffer||state!=0x12345678)return false;
 return true;
}
static int leaf(FILE*in,FILE*out){
 uint32_t number=0;if(!read32(in,number)||number>1000000u)return 20;
 for(uint32_t row=0;row<number;++row){
  uint32_t words[4];for(auto&x:words)if(!read32(in,x))return 20;const size_t count=words[3];if(count>256)return 20;
  SupportUnit u[256];uint32_t states[256];
  for(size_t i=0;i<count;++i){uint32_t x[5];for(auto&v:x)if(!read32(in,v))return 20;if(x[4]>15)return 20;u[i]={x[0],(int32_t)x[1],(int32_t)x[2],(int32_t)x[3],(x[4]&1u)!=0,(x[4]&2u)!=0,(x[4]&4u)!=0,(x[4]&8u)!=0};states[i]=0x5a5a5a5a;}
  SupportFacts f{(int32_t)words[0],(int32_t)words[1],(int32_t)words[2]};SupportResult r;memset(&r,0x5a,sizeof r);
  const auto e=base_support_policy(f,u,count,r,states,count);if(!result(out,e,r,states,count))return 20;
 }
 return fgetc(in)==EOF?0:20;
}
static unsigned char* file(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long len=ftell(f);if(len<=0||len>64*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)len);if(!b){fclose(f);return nullptr;}n=(size_t)len;bool ok=fread(b,1,n,f)==n;fclose(f);if(!ok){free(b);return nullptr;}return b;}
static OriginalWorld world;static Rules technologies;static UnitDefinitions definitions;
struct Range{const void*p;size_t n;};
static int world_case(const char*rules,const char*save,FILE*out){
 size_t n=0;auto*b=file(rules,n);if(!b)return 20;bool parsed=read_technologies(b,n,technologies)==Error::ok&&read_unit_definitions(b,n,technologies,Variant::base,definitions)==Error::ok;memset(b,0xff,n);free(b);if(!parsed)return 20;
 b=file(save,n);if(!b)return 20;const auto loaded=world.load(b,n,12345);memset(b,0xff,n);free(b);if(loaded!=WorldLoadError::ok)return 20;
 const auto&g=world.game();const size_t count=g.vehicle_count;auto*states=count?(uint32_t*)malloc(count*4):nullptr;if(count&&!states)return 20;
 Range ranges[]={{&world,sizeof world},{&definitions,sizeof definitions},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}auto*before=(unsigned char*)malloc(total);if(!before)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(before+at,r.p,r.n);at+=r.n;}
 SupportResult r;memset(&r,0x5a,sizeof r);SupportResult prior;memcpy(&prior,&r,sizeof r);for(size_t i=0;i<count;++i)states[i]=0x5a5a5a5a;
 if(world_base_support_policy(world,definitions,g.base_count,r,states,count)!=SupportError::bad_state||memcmp(&prior,&r,sizeof r))return 21;
 for(const auto&range:ranges){if(range.n<sizeof(SupportResult))continue;auto&aliased=*reinterpret_cast<SupportResult*>(const_cast<void*>(range.p));if(world_base_support_policy(world,definitions,0,aliased,states,count)!=SupportError::bad_buffer)return 21;if(count&&world_base_support_policy(world,definitions,0,r,(uint32_t*)const_cast<void*>(range.p),count)!=SupportError::bad_buffer)return 21;}
 if(count&&world_base_support_policy(world,definitions,0,r,states,count-1)!=SupportError::bad_buffer)return 21;
 const auto error=world_base_support_policy(world,definitions,0,r,states,count);
 if(error!=SupportError::ok&&(memcmp(&prior,&r,sizeof r)))return 21;
 at=0;for(const auto&range:ranges){if(range.n&&memcmp(before+at,range.p,range.n))return 21;at+=range.n;}free(before);
 bool ok=result(out,error,r,states,count);free(states);return ok?0:20;
}
int main(int argc,char**argv){
 if(!pure_checks())return 21;
 if(argc==4&&!strcmp(argv[1],"leaf")){FILE*in=fopen(argv[2],"rb"),*out=fopen(argv[3],"wb");if(!in||!out)return 20;int rc=leaf(in,out);if(fclose(in)||fclose(out))return 20;return rc;}
 if(argc==5&&!strcmp(argv[1],"world")){FILE*out=fopen(argv[4],"wb");if(!out)return 20;int rc=world_case(argv[2],argv[3],out);if(fclose(out))return 20;return rc;}
 return 20;
}
