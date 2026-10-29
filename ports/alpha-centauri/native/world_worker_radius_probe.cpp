/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_worker_radius.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
alignas(WorkerRadiusResult) static OriginalWorld world;
struct ByteRange {const void* data;size_t size,offset;};
static ByteRange ranges[9];static unsigned range_count;static unsigned char* baseline;
static bool capture(){
 const auto&g=world.game();range_count=0;size_t total=0;
 const ByteRange inputs[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0}};
 for(const auto&r:inputs){if(!r.size)continue;if(total>SIZE_MAX-r.size)return false;ranges[range_count++]={r.data,r.size,total};total+=r.size;}
 baseline=(unsigned char*)malloc(total);if(!baseline)return false;
 for(unsigned i=0;i<range_count;++i)memcpy(baseline+ranges[i].offset,ranges[i].data,ranges[i].size);
 return true;
}
static bool unchanged(){for(unsigned i=0;i<range_count;++i)if(memcmp(baseline+ranges[i].offset,ranges[i].data,ranges[i].size))return false;return true;}
static bool unchanged_rejection(size_t id,const WorkerRadiusStage&s,WorkerRadiusError expected){WorkerRadiusResult r;memset(&r,0x5a,sizeof r);WorkerRadiusResult before;memcpy(&before,&r,sizeof r);return world_worker_radius(world,id,s,r)==expected && !memcmp(&r,&before,sizeof r);}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*bytes=(unsigned char*)malloc((size_t)n);if(!bytes){fclose(f);return 20;}bool ok=fread(bytes,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(bytes);return 20;}auto load=world.load(bytes,(size_t)n,0xabcdef01);memset(bytes,0xff,(size_t)n);free(bytes);if(load!=WorldLoadError::ok)return 20;
 if(!capture())return 20;
 alignas(WorkerRadiusResult) WorkerRadiusStage stage{0,false};
 if(!unchanged_rejection(world.game().base_count,stage,WorkerRadiusError::bad_state) || !unchanged_rejection(0,{1,false},WorkerRadiusError::unsupported_stage) || !unchanged_rejection(0,{0,true},WorkerRadiusError::unsupported_stage))return 21;
 const void*targets[]={&world,world.original_bytes(),world.tile(0),world.vehicle(0),world.base(0),world.prototype(0),world.faction(0),world.faction_definition(0),&stage};
 for(const void*p:targets){if(!p)continue;WorkerRadiusStage prior;memcpy(&prior,&stage,sizeof stage);auto&r=*reinterpret_cast<WorkerRadiusResult*>(const_cast<void*>(p));if(world_worker_radius(world,0,stage,r)!=WorkerRadiusError::bad_buffer || !unchanged() || memcmp(&prior,&stage,sizeof stage))return 21;}
 WorkerRadiusResult r;memset(&r,0x5a,sizeof r);WorkerRadiusResult old;memcpy(&old,&r,sizeof r);const auto error=world_worker_radius(world,0,stage,r);if(error!=WorkerRadiusError::ok && memcmp(&r,&old,sizeof r))return 21;
 uint32_t fields[27];fields[0]=(uint32_t)error;fields[1]=r.unavailable;for(unsigned i=0;i<25;++i)fields[2+i]=r.flags[i];
 for(uint32_t word:fields){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(word>>(8*i));if(fwrite(b,1,4,stdout)!=4)return 20;}
 const bool same=unchanged() && world.runtime_seed()==0xabcdef01;free(baseline);return same ? 0 : 21;
}
