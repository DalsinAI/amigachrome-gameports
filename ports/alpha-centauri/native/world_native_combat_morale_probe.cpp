/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "native_combat_morale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;

static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){if(argc!=2)return 20;size_t n=0;auto*data=read(argv[1],n);if(!data)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok||!w.game().vehicle_count)return 20;unsigned guards=0;uint32_t out=0x12345678;
 CHECK(world_native_combat_morale(w,w.game().vehicle_count,1,out)==LogicError::bad_state&&out==0x12345678);
 CHECK(world_native_combat_morale(w,0,8,out)==LogicError::bad_state&&out==0x12345678);
 CHECK(world_native_combat_morale(w,0,-2,out)==LogicError::bad_state&&out==0x12345678);
 CHECK(world_native_combat_morale(w,0,0,alias<uint32_t>((uintptr_t)w.original_bytes()))==LogicError::bad_state);
 CHECK(world_native_combat_morale(w,0,0,alias<uint32_t>((uintptr_t)w.vehicle(0)))==LogicError::bad_state);
 CHECK(world_native_combat_morale(w,0,0,alias<uint32_t>((uintptr_t)w.faction(0)))==LogicError::bad_state);
 CHECK(world_native_combat_morale(w,0,0,alias<uint32_t>((uintptr_t)w.tile(0)))==LogicError::bad_state);
 for(size_t i=0;i<w.game().vehicle_count;++i)for(int32_t opponent=-1;opponent<8;++opponent){out=0x12345678;auto e=world_native_combat_morale(w,i,opponent,out);if(!emit(e==LogicError::ok?0:1)||!emit(out))return 22;}
 CHECK(w.runtime_seed()==0x12345678&&w.original_size()==n&&!memcmp(w.original_bytes(),data,n));if(!emit(guards))return 23;free(data);return 0;}
