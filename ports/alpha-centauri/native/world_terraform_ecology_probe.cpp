/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_terraform_ecology.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
struct Frame {TerraformEcologyResult before;OriginalWorld world;TerraformEcologyResult after;};
static Frame frame;static OriginalWorld&world=frame.world;
struct Range {const void*p;size_t n,offset;};
static Range ranges[8];static unsigned range_count;static unsigned char*baseline;
static bool capture(){
 const auto&g=world.game();size_t total=0;range_count=0;
 const Range inputs[]={{&world,sizeof world,0},{world.original_bytes(),world.original_size(),0},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile),0},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase),0},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle),0},{world.prototype(0),512*sizeof(OriginalPrototype),0},{world.faction(0),8*sizeof(OriginalFactionState),0},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition),0}};
 for(const auto&r:inputs){if(!r.n)continue;if(total>SIZE_MAX-r.n)return false;ranges[range_count++]={r.p,r.n,total};total+=r.n;}
 baseline=(unsigned char*)malloc(total);if(!baseline)return false;for(unsigned i=0;i<range_count;++i)memcpy(baseline+ranges[i].offset,ranges[i].p,ranges[i].n);return true;
}
static bool unchanged(){for(unsigned i=0;i<range_count;++i)if(memcmp(baseline+ranges[i].offset,ranges[i].p,ranges[i].n))return false;return true;}
static bool reject(size_t id,TerraformEcologyError expected){TerraformEcologyResult r;memset(&r,0x5a,sizeof r);TerraformEcologyResult old;memcpy(&old,&r,sizeof r);return world_terraform_ecology(world,id,r)==expected && !memcmp(&old,&r,sizeof r);}
int main(int argc,char**argv){
 if(argc!=2)return 20;
 if(!reject(0,TerraformEcologyError::bad_state))return 21;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return 20;}auto*bytes=(unsigned char*)malloc((size_t)n);if(!bytes){fclose(f);return 20;}const bool read=fread(bytes,1,(size_t)n,f)==(size_t)n;fclose(f);if(!read){free(bytes);return 20;}const auto loaded=world.load(bytes,(size_t)n,0xabcdef01);memset(bytes,0xff,(size_t)n);free(bytes);if(loaded!=WorldLoadError::ok)return 20;
 if(!capture() || !reject(world.game().base_count,TerraformEcologyError::bad_state))return 21;
 for(unsigned i=0;i<range_count;++i){const auto&r=ranges[i];const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets){auto&out=*reinterpret_cast<TerraformEcologyResult*>(const_cast<unsigned char*>(static_cast<const unsigned char*>(r.p))+offset);if(world_terraform_ecology(world,0,out)!=TerraformEcologyError::bad_buffer || !unchanged())return 21;}}
 TerraformEcologyResult result;memset(&result,0x5a,sizeof result);TerraformEcologyResult old;memcpy(&old,&result,sizeof result);const auto error=world_terraform_ecology(world,0,result);
 if(error!=TerraformEcologyError::ok && memcmp(&old,&result,sizeof result))return 21;
 if(error==TerraformEcologyError::ok){
  if(world_terraform_ecology(world,0,frame.before)!=error || world_terraform_ecology(world,0,frame.after)!=error || memcmp(&frame.before,&result,sizeof result) || memcmp(&frame.after,&result,sizeof result))return 21;
  auto*b=const_cast<OriginalBase*>(world.base(0));const int16_t x=b->x;b->x=-1;if(!reject(0,TerraformEcologyError::bad_state))return 21;b->x=x;
 }
 uint32_t fields[24];fields[0]=(uint32_t)error;fields[1]=(uint32_t)result.raw_damage;fields[2]=result.valid_tiles;for(unsigned i=0;i<21;++i)fields[3+i]=(uint32_t)result.contributions[i];
 for(uint32_t word:fields){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(word>>(8*i));if(fwrite(b,1,4,stdout)!=4)return 20;}
 const bool same=unchanged() && world.runtime_seed()==0xabcdef01;free(baseline);return same?0:21;
}
