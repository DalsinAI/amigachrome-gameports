/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_commerce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world;static Rules technologies;static int32_t energy[512];static size_t energy_count;
static unsigned char*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);n=(size_t)z;if(!ok){free(b);return nullptr;}return b;}
static uint32_t le(const unsigned char*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
struct Range{const void*p;size_t n,offset;};static Range ranges[10];static unsigned count;static unsigned char*copy;
static bool capture(){const auto&g=world.game();Range in[]={{&world,sizeof world,0},{&technologies,sizeof technologies,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0},{energy,sizeof energy,0}};size_t total=0;for(const auto&r:in)if(r.n){ranges[count++]={r.p,r.n,total};total+=r.n;}copy=(unsigned char*)malloc(total);if(!copy)return false;for(unsigned i=0;i<count;++i)memcpy(copy+ranges[i].offset,ranges[i].p,ranges[i].n);return true;}
static bool unchanged(){for(unsigned i=0;i<count;++i)if(memcmp(copy+ranges[i].offset,ranges[i].p,ranges[i].n))return false;return true;}
int main(int argc,char**argv){if(argc!=5)return 20;size_t n=0;auto*p=read(argv[1],n);if(!p)return 20;bool ok=read_technologies(p,n,technologies)==Error::ok;free(p);if(!ok)return 20;p=read(argv[2],n);if(!p)return 20;ok=world.load(p,n,0x12345678)==WorldLoadError::ok;free(p);if(!ok)return 20;p=read(argv[3],n);if(!p||n<4)return 20;energy_count=le(p);if(energy_count>512||n!=4+4*energy_count){free(p);return 20;}for(size_t i=0;i<energy_count;++i)energy[i]=(int32_t)le(p+4+4*i);free(p);const size_t base=(size_t)strtoul(argv[4],nullptr,10);if(!capture())return 21;
 for(unsigned i=0;i<count;++i){const auto&r=ranges[i];const size_t at[]={0,4,(r.n-1)&~size_t(3)};for(size_t delta:at){auto*out=const_cast<unsigned char*>(static_cast<const unsigned char*>(r.p))+delta;
  // The input vector occupies only its count, the remaining static capacity is
  // not an input alias. Other world/rules ranges are completely protected.
  if(i==count-1&&delta>=energy_count*sizeof(int32_t))continue;
  if(world_base_commerce(world,technologies,base,energy,energy_count,*reinterpret_cast<UpkeepWorldCommerceResult*>(out))!=CommerceError::bad_buffer||!unchanged())return 21;
 }}
 UpkeepWorldCommerceResult result,before;memset(&result,0x5a,sizeof result);memcpy(&before,&result,sizeof result);
 if(world_base_commerce(world,technologies,base,reinterpret_cast<const int32_t*>(&technologies),energy_count,result)!=CommerceError::bad_buffer||memcmp(&result,&before,sizeof result)||!unchanged())return 21;
 if(world_base_commerce(world,technologies,base,&world.base(0)->energy_intake,energy_count,result)!=CommerceError::bad_buffer||memcmp(&result,&before,sizeof result)||!unchanged())return 21;
 const auto error=world_base_commerce(world,technologies,base,energy,energy_count,result);if(error!=CommerceError::ok&&memcmp(&result,&before,sizeof result))return 21;printf("%d %ld %ld %ld",(int)error,(long)result.rank,(long)result.technology_count,(long)result.total);for(int32_t v:result.partner_base)printf(" %ld",(long)v);for(int32_t v:result.import_value)printf(" %ld",(long)v);for(int32_t v:result.export_value)printf(" %ld",(long)v);for(int32_t v:result.fresh_technology_bonus)printf(" %ld",(long)v);puts("");const bool same=unchanged();free(copy);return same?0:21;
}
