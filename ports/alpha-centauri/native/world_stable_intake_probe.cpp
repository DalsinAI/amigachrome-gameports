/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_stable_intake.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
alignas(StableIntakeResult) static Rules technologies;
alignas(StableIntakeResult) static BasicRules basic;
alignas(StableIntakeResult) static ResourceInfo resources;
static WorldRules world_rules;
alignas(StableIntakeResult) static OriginalWorld world;
struct Range {const void*p;size_t n,offset;};
static Range ranges[12];static unsigned range_count;static unsigned char*baseline;
static unsigned char*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}const bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool capture(const StableIntakeStage&stage){const auto&g=world.game();size_t total=0;range_count=0;
 const Range inputs[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{&technologies,sizeof technologies,0},{&basic,sizeof basic,0},{&resources,sizeof resources,0},{&stage,sizeof stage,0}};
 for(const auto&r:inputs){if(!r.n)continue;if(total>SIZE_MAX-r.n)return false;ranges[range_count++]={r.p,r.n,total};total+=r.n;}baseline=(unsigned char*)malloc(total);if(!baseline)return false;for(unsigned i=0;i<range_count;++i)memcpy(baseline+ranges[i].offset,ranges[i].p,ranges[i].n);return true;
}
static bool unchanged(){for(unsigned i=0;i<range_count;++i)if(memcmp(baseline+ranges[i].offset,ranges[i].p,ranges[i].n))return false;return true;}
static bool reject(size_t id,const StableIntakeStage&s,StableIntakeError expected){StableIntakeResult r;memset(&r,0x5a,sizeof r);StableIntakeResult old;memcpy(&old,&r,sizeof r);return world_stable_intake(world,technologies,basic,resources,id,s,r)==expected && !memcmp(&old,&r,sizeof r);}
int main(int argc,char**argv){
 if((argc!=4&&argc!=5) || (strcmp(argv[1],"base")&&strcmp(argv[1],"expansion")))return 20;
 const Variant variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n=0;auto*b=read(argv[2],n);if(!b)return 20;const bool parsed=read_technologies(b,n,technologies)==Error::ok && read_basic_rules(b,n,technologies,variant,basic,world_rules)==Error::ok && read_resource_info(b,n,resources)==Error::ok;memset(b,0xff,n);free(b);if(!parsed)return 20;
 const unsigned mode=argc==5?(unsigned)(argv[4][0]-'0'):0;if(mode>3 || (argc==5&&argv[4][1]))return 20;
 StableIntakeStage stage{variant,false,(mode&1u)!=0,{0,false},mode==2?-2:mode==3?2147483647:0,mode>=2?-123456:0};if(!reject(0,stage,StableIntakeError::bad_state))return 21;
 b=read(argv[3],n);if(!b)return 20;const auto loaded=world.load(b,n,0xabcdef01);memset(b,0xff,n);free(b);if(loaded!=WorldLoadError::ok)return 20;
 if(!capture(stage) || !reject(world.game().base_count,stage,StableIntakeError::bad_state))return 21;
 auto invalid=stage;invalid.variant=(Variant)99;if(!reject(0,invalid,StableIntakeError::bad_state))return 21;invalid=stage;invalid.variant=Variant::base;invalid.expansion_enabled=true;if(!reject(0,invalid,StableIntakeError::bad_state))return 21;invalid=stage;invalid.worker.editor_active=true;if(!reject(0,invalid,StableIntakeError::unsupported_stage))return 21;invalid=stage;invalid.worker.upkeep_state=1;if(!reject(0,invalid,StableIntakeError::unsupported_stage))return 21;
 for(unsigned i=0;i<range_count;++i){const auto&r=ranges[i];const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets){auto&out=*reinterpret_cast<StableIntakeResult*>(const_cast<unsigned char*>(static_cast<const unsigned char*>(r.p))+offset);if(world_stable_intake(world,technologies,basic,resources,0,stage,out)!=StableIntakeError::bad_buffer || !unchanged())return 21;}}
 StableIntakeResult r;memset(&r,0x5a,sizeof r);StableIntakeResult old;memcpy(&old,&r,sizeof r);const auto error=world_stable_intake(world,technologies,basic,resources,0,stage,r);if(error!=StableIntakeError::ok && memcmp(&old,&r,sizeof r))return 21;
 uint32_t words[79];words[0]=(uint32_t)error;unsigned at=1;for(int32_t x:r.satellites)words[at++]=(uint32_t)x;for(int32_t x:r.tiles)words[at++]=(uint32_t)x;for(int32_t x:r.intake)words[at++]=(uint32_t)x;words[at++]=r.worked_tiles;words[at++]=r.state_flags;words[at++]=(uint32_t)r.raw_eco_damage;words[at++]=(uint32_t)r.terraform_reduction_after;words[at++]=(uint32_t)r.base_energy_after;for(const auto&tile:r.tile_yields)for(int32_t x:tile)words[at++]=(uint32_t)x;if(at!=79)return 21;
 for(uint32_t x:words){unsigned char value[4];for(unsigned i=0;i<4;++i)value[i]=(unsigned char)(x>>(8*i));if(fwrite(value,1,4,stdout)!=4)return 20;}
 const bool same=unchanged()&&world.runtime_seed()==0xabcdef01;free(baseline);return same?0:21;
}
