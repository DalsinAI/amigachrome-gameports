/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "retreat_destination.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static uint32_t query_flags=0;static bool net=false;static PodCoefficients coeff{61,11};
static LogicError query(const OriginalWorld&w,const UnitDefinitions&d,const Rules&,const BasicRules&,size_t,size_t b,RetreatDestination&o){return world_land_retreat_destination(w,d,b,query_flags,net,coeff,o);}
static UnitDefinitions units;static Rules tech;static BasicRules basic;static WorldRules worldrules;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok||!w.game().vehicle_count)return 20;const auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;unsigned guards=0;RetreatDestination result{};memset(&result,0xa5,sizeof result);unsigned char initial[sizeof result];memcpy(initial,&result,sizeof result);
 CHECK(world_land_retreat_destination(w,units,0,8,false,coeff,result)==LogicError::bad_state);
 CHECK(query(w,units,tech,basic,0,w.game().vehicle_count,result)==LogicError::bad_state);
 CHECK(!memcmp(initial,&result,sizeof result));
 const uintptr_t aliases[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)&units,(uintptr_t)&units,(uintptr_t)&coeff};
 for(auto a:aliases)CHECK(query(w,units,tech,basic,0,0,alias<RetreatDestination>(a))==LogicError::bad_state);
 auto count=units.weapon_count;units.weapon_count=0;CHECK(query(w,units,tech,basic,0,0,result)==LogicError::bad_state);units.weapon_count=count;
 CHECK(world_land_retreat_destination(w,units,0,0,false,coeff,alias<RetreatDestination>((uintptr_t)&coeff))==LogicError::bad_state);

 const auto* design=w.prototype((size_t)w.vehicle(0)->unit_id);auto& offense=units.weapons[design->weapon_id].offense;const auto old_offense=offense;offense=-129;CHECK(query(w,units,tech,basic,0,0,result)==LogicError::bad_state);offense=128;CHECK(query(w,units,tech,basic,0,0,result)==LogicError::bad_state);offense=old_offense;

 for(int network=0;network<2;++network)for(query_flags=0;query_flags<8;++query_flags)for(size_t j=0;j<w.game().vehicle_count;++j){memset(&result,0xa5,sizeof result);unsigned char before[sizeof result];memcpy(before,&result,sizeof result);net=network!=0;auto e=query(w,units,tech,basic,0,j,result);if(e!=LogicError::ok&&memcmp(before,&result,sizeof result))return 22;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)result.direction:0)||!emit(e==LogicError::ok?(uint32_t)result.x:0)||!emit(e==LogicError::ok?(uint32_t)result.y:0)||!emit(e==LogicError::ok?result.eligible_mask:0))return 23;}
 CHECK(w.runtime_seed()==0x12345678&&w.original_size()==n&&!memcmp(w.original_bytes(),data,n));if(!emit(guards))return 24;free(data);free(text);return 0;}
