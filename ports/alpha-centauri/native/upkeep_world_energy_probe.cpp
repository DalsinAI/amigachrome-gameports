/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_energy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world;static Rules technologies;static BasicRules basic;static WorldRules world_rules;static ResourceInfo resources;
static unsigned char*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);n=(size_t)z;return ok?b:nullptr;}
static void result(const UpkeepWorldEnergyResult&r){printf(" %ld %ld %d %ld %ld %ld %ld",(long)r.headquarters_distance,(long)r.pending_efficiency,r.physical_creche,(long)r.accounting.intake,(long)r.accounting.consumption,(long)r.accounting.inefficiency,(long)r.accounting.surplus);for(int32_t x:r.accounting.social_efficiency_after)printf(" %ld",(long)x);}
struct Range {const void*p;size_t n,off;};static Range ranges[12];static unsigned count;static unsigned char*copy;
static bool capture(const FreshEnergyStage&s){const auto&g=world.game();Range inputs[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{&s,sizeof s,0},{&technologies,sizeof technologies,0},{&basic,sizeof basic,0},{&resources,sizeof resources,0}};size_t total=0;for(const auto&r:inputs)if(r.n){ranges[count++]={r.p,r.n,total};total+=r.n;}copy=(unsigned char*)malloc(total);if(!copy)return false;for(unsigned i=0;i<count;++i)memcpy(copy+ranges[i].off,ranges[i].p,ranges[i].n);return true;}
static bool unchanged(){for(unsigned i=0;i<count;++i)if(memcmp(copy+ranges[i].off,ranges[i].p,ranges[i].n))return false;return true;}
int main(int argc,char**argv){
 if(argc!=4&&argc!=5)return 20;
 const Variant variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n=0;auto*b=read(argv[2],n);if(!b)return 20;
 bool ok=read_technologies(b,n,technologies)==Error::ok&&read_basic_rules(b,n,technologies,variant,basic,world_rules)==Error::ok&&read_resource_info(b,n,resources)==Error::ok;free(b);if(!ok)return 20;
 b=read(argv[3],n);if(!b)return 20;ok=world.load(b,n,0xabcdef01)==WorldLoadError::ok;free(b);if(!ok)return 20;
 const size_t id=argc==5?(size_t)strtoul(argv[4],nullptr,10):0;UpkeepWorldEnergyStage direct{23,2,3,0,7,1};FreshEnergyStage stage{{variant,false,false,{0,false},0,0},0,0};
 if(!capture(stage))return 21;
 for(unsigned i=0;i<count;++i){const auto&r=ranges[i];const size_t at[]={0,4,(r.n-1)&~size_t(3)};for(size_t delta:at){auto*out=const_cast<unsigned char*>(static_cast<const unsigned char*>(r.p))+delta;
  if(world_fresh_energy(world,technologies,basic,resources,id,stage,*reinterpret_cast<FreshEnergyResult*>(out))!=StableIntakeError::bad_buffer||!unchanged())return 21;
  if(i<8&&(world_energy_accounting(world,id,direct,*reinterpret_cast<UpkeepWorldEnergyResult*>(out))!=EnergyError::bad_buffer||world_energy_shortfall_orders(world,id,-1,*reinterpret_cast<EnergyShortfallOrders*>(out))!=EnergyError::bad_buffer||!unchanged()))return 21;
 }}
 UpkeepWorldEnergyResult accounting;memset(&accounting,0x5a,sizeof accounting);auto e=world_energy_accounting(world,id,direct,accounting);printf("account %d",(int)e);if(e==EnergyError::ok)result(accounting);puts("");
 EnergyShortfallOrders orders;memset(&orders,0x5a,sizeof orders);e=world_energy_shortfall_orders(world,id,-1,orders);printf("shortfall %d %lu",(int)e,(unsigned long)orders.count);for(uint32_t x:orders.cancel)printf(" %lu",(unsigned long)x);puts("");
 FreshEnergyResult fresh;memset(&fresh,0x5a,sizeof fresh);auto fe=world_fresh_energy(world,technologies,basic,resources,id,stage,fresh);printf("fresh %d",(int)fe);if(fe==StableIntakeError::ok){result(fresh.energy);for(int32_t x:fresh.convoys.to)printf(" %ld",(long)x);for(int32_t x:fresh.convoys.from)printf(" %ld",(long)x);printf(" %lu",(unsigned long)fresh.shortfall.count);}puts("");
 const bool same=unchanged()&&world.runtime_seed()==0xabcdef01;free(copy);return same?0:21;
}
