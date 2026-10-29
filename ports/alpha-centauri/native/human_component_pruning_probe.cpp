/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "human_component_pruning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;static alpha::UnitDefinitions defs;
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sign(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static uint32_t whole(const alpha::OriginalWorld& w){uint32_t h=2166136261u;auto add=[&](const void* data,size_t n){const auto* p=(const uint8_t*)data;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;};add(&w,sizeof w);add(w.original_bytes(),w.original_size());add(w.current_monuments().data,w.current_monuments().size);add(w.tile(0),w.game().map.info.tile_count*sizeof(alpha::OriginalMapTile));add(w.vehicle(0),w.game().vehicle_count*sizeof(alpha::OriginalVehicle));add(w.base(0),w.game().base_count*sizeof(alpha::OriginalBase));add(w.prototype(0),512*sizeof(alpha::OriginalPrototype));add(w.faction(0),8*sizeof(alpha::OriginalFactionState));add(w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));add(&rules,sizeof rules);add(&defs,sizeof defs);return h;}
int main(int argc,char** argv){if(argc!=3)return 20;FILE* f=fopen(argv[2],"rb");uint8_t facts[960];if(!f || fread(facts,1,960,f)!=960)return 20;fclose(f);int32_t faction=sign(get(facts)),selected=sign(get(facts+4));auto runtime=(alpha::PruningRuntime)get(facts+8);auto variant=(alpha::Variant)get(facts+12);rules.count=89;defs.weapon_count=variant==alpha::Variant::base?21:26;defs.armor_count=variant==alpha::Variant::base?10:14;
 size_t at=16;for(unsigned i=0;i<89;++i)for(unsigned j=0;j<2;++j){rules.technologies[i].prerequisites[j]=sign(get(facts+at));at+=4;}
 for(unsigned i=0;i<9;++i){defs.chassis[i].speed=sign(get(facts+at));defs.chassis[i].triad=sign(get(facts+at+4));at+=8;}
 for(unsigned i=0;i<26;++i){defs.weapons[i].offense=sign(get(facts+at));at+=4;}for(unsigned i=0;i<14;++i){defs.armor[i].defense=sign(get(facts+at));at+=4;}
 f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0)return 20;auto* b=(uint8_t*)malloc((size_t)n);if(!b || fread(b,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);alpha::OriginalWorld w;if(w.load(b,(size_t)n,123)!=alpha::WorldLoadError::ok)return 20;free(b);uint32_t before=whole(w);alpha::HumanPruningResult out;memset(&out,0x5a,sizeof out);auto e=alpha::world_human_component_pruning(w,variant,rules,defs,runtime,faction,selected,out);if(!emit((uint32_t)e))return 20;
 if(e==alpha::HumanPruningError::ok){if(!emit(out.changed))return 20;for(auto flags:out.flags)if(!emit(flags))return 20;const auto previous=out;if(alpha::world_human_component_pruning(w,variant,rules,defs,alpha::PruningRuntime::unknown,faction,selected,out)!=alpha::HumanPruningError::unsupported_runtime || memcmp(&previous,&out,sizeof out))return 21;}
 else{const auto* p=(const uint8_t*)&out;for(size_t i=0;i<sizeof out;++i)if(p[i]!=0x5a)return 21;}
 if(whole(w)!=before)return 21;
 const void* aliases[]={&w,&rules,&defs,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0)};
 for(const auto* p:aliases)if(p){auto& fake=*(alpha::HumanPruningResult*)const_cast<void*>(p);if(alpha::world_human_component_pruning(w,variant,rules,defs,runtime,faction,selected,fake)!=alpha::HumanPruningError::bad_state)return 21;}
 return whole(w)==before?0:21;
}
