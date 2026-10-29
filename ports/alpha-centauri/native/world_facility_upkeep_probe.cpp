/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_facility_upkeep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world;static Rules tech;static UnitDefinitions units;static FacilityDefinitions facilities;
static unsigned char*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);n=(size_t)z;return ok?b:nullptr;}
static void word(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static void bytes(const uint8_t*b){for(unsigned j=0;j<3;++j){const auto*p=b+4*j;word((uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24);}}
struct Range {const void*p;size_t n,off;};static Range ranges[12];static unsigned count;static unsigned char*copy;
static bool capture(const FacilityUpkeepStage&s){const auto&g=world.game();Range inputs[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{&s,sizeof s,0},{&tech,sizeof tech,0},{&units,sizeof units,0},{&facilities,sizeof facilities,0}};size_t total=0;for(const auto&r:inputs)if(r.n){ranges[count++]={r.p,r.n,total};total+=r.n;}copy=(unsigned char*)malloc(total);if(!copy)return false;for(unsigned i=0;i<count;++i)memcpy(copy+ranges[i].off,ranges[i].p,ranges[i].n);return true;}
static bool unchanged(){for(unsigned i=0;i<count;++i)if(memcmp(copy+ranges[i].off,ranges[i].p,ranges[i].n))return false;return true;}
int main(int argc,char**argv){
 if(argc!=5&&argc!=6)return 20;
 const Variant variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n=0;auto*b=read(argv[2],n);if(!b)return 20;
 bool ok=read_technologies(b,n,tech)==Error::ok&&read_unit_definitions(b,n,tech,variant,units)==Error::ok&&read_facility_definitions(b,n,tech,variant,facilities)==Error::ok;free(b);if(!ok)return 20;
 b=read(argv[3],n);if(!b)return 20;ok=world.load(b,n,0xabcdef01)==WorldLoadError::ok;free(b);if(!ok)return 20;
 const int mode=atoi(argv[4]);if(mode<0||mode>3)return 20;const size_t id=argc==6?(size_t)strtoul(argv[5],nullptr,10):0;FacilityUpkeepStage stage{(mode&1)?100:0,7,10,0x200u|((mode&2)?32u:0u)};
 if(!capture(stage))return 21;
 for(unsigned i=0;i<count;++i){const auto&r=ranges[i];const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t delta:offsets){auto*out=const_cast<unsigned char*>(static_cast<const unsigned char*>(r.p))+delta;if(world_facility_upkeep(world,tech,units,facilities,id,stage,*reinterpret_cast<FacilityUpkeepResult*>(out))!=EnergyError::bad_buffer||!unchanged())return 21;}}
 FacilityUpkeepResult result;memset(&result,0x5a,sizeof result);auto error=world_facility_upkeep(world,tech,units,facilities,id,stage,result);word((uint32_t)error);word((uint32_t)result.credits_after);word((uint32_t)result.maintenance_after);word(result.player_flags_after);bytes(result.built_after);bytes(result.scrapped);bytes(result.shortage);for(int32_t x:result.charged)word((uint32_t)x);
 const bool same=unchanged()&&world.runtime_seed()==0xabcdef01;free(copy);return same&&!ferror(stdout)?0:21;
}
