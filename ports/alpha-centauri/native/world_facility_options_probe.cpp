/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_facility_options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char* read(const char* path,size_t& length){length=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);if(n<0 || n>32*1024*1024 || fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}auto* b=(unsigned char*)malloc((size_t)n+1);if(!b){fclose(f);return nullptr;}if(fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return nullptr;}fclose(f);length=(size_t)n;return b;}
static bool emit(int32_t value){uint32_t n=(uint32_t)value;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static alpha::Rules tech;static alpha::FacilityDefinitions definitions;static alpha::OriginalWorld world;
int main(int argc,char** argv){
 if(argc!=5)return 20;
 alpha::Variant profile;if(!strcmp(argv[1],"base"))profile=alpha::Variant::base;else if(!strcmp(argv[1],"expansion"))profile=alpha::Variant::alien_crossfire;else return 20;
 if(strcmp(argv[2],"0") && strcmp(argv[2],"1"))return 20;
 const bool expansion=argv[2][0]=='1';size_t n;unsigned char* data=read(argv[3],n);if(!data)return 20;
 if(alpha::read_technologies(data,n,tech)!=alpha::Error::ok || alpha::read_facility_definitions(data,n,tech,profile,definitions)!=alpha::Error::ok){free(data);return 20;}free(data);
 alpha::OriginalWorld empty;alpha::WorldFacilityOptions untouched;memset(&untouched,0x5a,sizeof untouched);unsigned char previous[sizeof untouched];memcpy(previous,&untouched,sizeof untouched);if(alpha::world_facility_options(empty,tech,definitions,profile,expansion,1,-1,0,untouched)!=alpha::LogicError::bad_state || memcmp(previous,&untouched,sizeof untouched))return 21;
 data=read(argv[4],n);if(!data)return 20;
 if(world.load(data,n,0x13579bdf)!=alpha::WorldLoadError::ok){free(data);return 20;}
 const uint32_t seed=world.runtime_seed();const int32_t queues[]={0,1,10};
 for(int32_t faction=1;faction<=7;++faction)for(int32_t base=-1;base<(int32_t)world.game().base_count;++base)for(unsigned j=0;j<3;++j){
  alpha::WorldFacilityOptions value;memset(&value,0x5a,sizeof value);unsigned char before[sizeof value];memcpy(before,&value,sizeof value);
  auto error=alpha::world_facility_options(world,tech,definitions,profile,expansion,faction,base,queues[j],value);
  if(error!=alpha::LogicError::ok && memcmp(&value,before,sizeof value)){free(data);return 21;}
  if(memcmp(world.original_bytes(),data,n) || world.runtime_seed()!=seed){free(data);return 21;}
  if(!emit(faction)||!emit(base)||!emit(queues[j])||!emit((int32_t)error)){free(data);return 20;}
  if(error==alpha::LogicError::ok){if(!emit((int32_t)value.count)||!emit(value.adjacent_ocean?1:0)||fwrite(value.available,1,134,stdout)!=134){free(data);return 20;}}
  else {unsigned char zero[134]{};if(!emit(0)||!emit(0)||fwrite(zero,1,134,stdout)!=134){free(data);return 20;}}
 }
 const int32_t bad[][3]={{0,0,0},{8,0,0},{1,-2,0},{1,2147483647,0},{1,-1,-1},{1,-1,11}};
 for(unsigned i=0;i<6;++i){alpha::WorldFacilityOptions value;memset(&value,0x5a,sizeof value);unsigned char before[sizeof value];memcpy(before,&value,sizeof value);auto e=alpha::world_facility_options(world,tech,definitions,profile,expansion,bad[i][0],bad[i][1],bad[i][2],value);if(e!=alpha::LogicError::bad_state || memcmp(&value,before,sizeof value)){free(data);return 21;}}
 free(data);return 0;
}
