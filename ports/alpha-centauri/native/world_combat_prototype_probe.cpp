/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_prototype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static UnitDefinitions units;static Rules tech;static BasicRules basic;static WorldRules worldrules;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 21;}++guards;}while(0)
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok||!w.game().vehicle_count)return 20;const auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;unsigned guards=0;
 PrototypeCombatRatings result{};int32_t factor=123;
 CHECK(world_prototype_combat_ratings(w,units,basic,w.game().vehicle_count,0,false,result)==LogicError::bad_state);
 CHECK(world_prototype_combat_ratings(w,units,basic,0,w.game().vehicle_count,false,result)==LogicError::bad_state);
 CHECK(world_combat_psi_factor(w,w.game().vehicle_count,1,false,factor)==LogicError::bad_state&&factor==123);
 CHECK(world_prototype_combat_ratings(w,units,basic,0,0,false,alias<PrototypeCombatRatings>((uintptr_t)w.original_bytes()))==LogicError::bad_state);
 CHECK(world_combat_psi_factor(w,0,1,false,alias<int32_t>((uintptr_t)w.faction_definition(0)))==LogicError::bad_state);
 const auto count=units.weapon_count;units.weapon_count=0;CHECK(world_prototype_combat_ratings(w,units,basic,0,0,false,result)==LogicError::bad_state);units.weapon_count=count;
 for(size_t i=0;i<w.game().vehicle_count;++i)for(size_t j=0;j<w.game().vehicle_count;++j)for(unsigned bombard=0;bombard<2;++bombard){
  if(world_prototype_combat_ratings(w,units,basic,i,j,bombard!=0,result)!=LogicError::ok)return 22;
  int32_t atk=0,def=0;if(world_combat_psi_factor(w,i,result.offense,true,atk)!=LogicError::ok||world_combat_psi_factor(w,i,result.defense,false,def)!=LogicError::ok)return 23;
  const unsigned flags=(result.offense_psi?1u:0u)|(result.defense_psi?2u:0u)|(result.probe_duel?4u:0u);
  if(!emit((uint32_t)result.offense)||!emit((uint32_t)result.defense)||!emit(flags)||!emit((uint32_t)atk)||!emit((uint32_t)def))return 24;
 }
 CHECK(w.runtime_seed()==0x12345678&&w.original_size()==n&&!memcmp(w.original_bytes(),data,n));
 if(!emit(guards))return 25;
 free(data);free(text);return 0;}
