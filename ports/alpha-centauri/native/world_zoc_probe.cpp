/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_zoc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1||n>64000000)return 20;auto*b=(unsigned char*)malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);OriginalWorld w;if(w.load(b,(size_t)n,0x12345678)!=WorldLoadError::ok)return 20;
 unsigned guards=0;uint32_t out=0xa5a5a5a5;
 for(int faction=-1;faction<=8;faction+=9){if(world_zone_of_control(w,0,0,faction,false,false,out)!=LogicError::bad_state||out!=0xa5a5a5a5)return 21;++guards;}
 const uintptr_t aliases[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0)};
 for(auto p:aliases){if(world_zone_of_control(w,0,0,1,false,false,*reinterpret_cast<uint32_t*>(p))!=LogicError::bad_state)return 22;++guards;}
 if(world_zone_of_control(w,1,0,1,false,false,out)!=LogicError::bad_state||out!=0xa5a5a5a5)return 23;
 ++guards;
 for(int network=0;network<2;++network)for(int move=0;move<2;++move)for(int faction=0;faction<8;++faction)for(uint32_t y=0;y<w.game().map.info.height;++y)for(uint32_t x=y&1;x<w.game().map.info.width;x+=2){out=0xa5a5a5a5;const auto e=world_zone_of_control(w,(int32_t)x,(int32_t)y,faction,network!=0,move!=0,out);if(e!=LogicError::ok&&out!=0xa5a5a5a5)return 24;if(!emit(e==LogicError::ok?0:1)||!emit(e==LogicError::ok?out:0))return 25;}
 if(w.runtime_seed()!=0x12345678||w.original_size()!=(size_t)n||memcmp(w.original_bytes(),b,(size_t)n))return 26;
 if(!emit(guards))return 27;
 free(b);return 0;}
