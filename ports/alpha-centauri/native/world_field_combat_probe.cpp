/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_field_combat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::UnitDefinitions definitions;
static alpha::Rules technologies;
static alpha::BasicRules basic;
static alpha::WorldRules world_rules;
static uint8_t* read_file(const char*path,size_t&size,size_t max){
 FILE*f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);
 if(n<=0||(size_t)n>max){fclose(f);return nullptr;}
 auto*p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}
 bool good=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);if(!good){free(p);return nullptr;}size=(size_t)n;return p;
}
static uint32_t hash(uint32_t h,const void*ptr,size_t n){const auto*p=(const uint8_t*)ptr;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static uint32_t digest(const alpha::OriginalWorld&w){
 const auto&g=w.game();uint32_t h=hash(2166136261u,w.original_bytes(),w.original_size());
 h=hash(h,w.tile(0),g.map.info.tile_count*sizeof(alpha::OriginalMapTile));
 h=hash(h,w.vehicle(0),g.vehicle_count*sizeof(alpha::OriginalVehicle));
 h=hash(h,w.base(0),g.base_count*sizeof(alpha::OriginalBase));
 h=hash(h,w.prototype(0),512*sizeof(alpha::OriginalPrototype));
 h=hash(h,w.faction(0),8*sizeof(alpha::OriginalFactionState));
 return hash(h,w.faction_definition(0),8*sizeof(alpha::OriginalFactionDefinition));
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
static bool alias_controls(const alpha::OriginalWorld&w){
 alpha::FieldCombatFacts facts;memset(&facts,0x5a,sizeof facts);const auto before=facts;
 alpha::CombatResult result;memset(&result,0x5a,sizeof result);const auto saved=result;
 uint16_t rolls[160]{};alpha::CombatRollStream rng{rolls,160,0};
 if(alpha::world_field_combat(w,definitions,technologies,basic,0,1,*reinterpret_cast<alpha::FieldCombatFacts*>(const_cast<alpha::OriginalPrototype*>(w.prototype(0))))!=alpha::CombatError::bad_buffer)return false;
 if(alpha::world_resolve_field_combat(w,definitions,technologies,basic,0,1,rng,facts,*reinterpret_cast<alpha::CombatResult*>(&facts))!=alpha::CombatError::bad_buffer)return false;
 if(alpha::world_resolve_field_combat(w,definitions,technologies,basic,0,1,rng,*reinterpret_cast<alpha::FieldCombatFacts*>(rolls),result)!=alpha::CombatError::bad_buffer)return false;
 return !memcmp(&before,&facts,sizeof facts)&&!memcmp(&saved,&result,sizeof result)&&!rng.cursor;
}
#pragma GCC diagnostic pop
int main(int argc,char**argv){
 if(argc!=3)return 20;
 size_t n=0;auto*bytes=read_file(argv[1],n,1048576);if(!bytes)return 20;
 const bool good=alpha::read_technologies(bytes,n,technologies)==alpha::Error::ok&&alpha::read_unit_definitions(bytes,n,technologies,alpha::Variant::base,definitions)==alpha::Error::ok&&alpha::read_basic_rules(bytes,n,technologies,alpha::Variant::base,basic,world_rules)==alpha::Error::ok;
 free(bytes);if(!good)return 20;
 bytes=read_file(argv[2],n,64u*1024u*1024u);if(!bytes)return 20;
 alpha::OriginalWorld w;auto error=w.load(bytes,n,0x12345678u);free(bytes);if(error!=alpha::WorldLoadError::ok)return 20;
 const uint32_t before=digest(w);if(!alias_controls(w)||digest(w)!=before)return 21;
 uint16_t rolls[160];for(unsigned i=0;i<160;++i)rolls[i]=(uint16_t)((i*7919u+17u)&32767u);
 alpha::CombatRollStream rng{rolls,160,0};alpha::FieldCombatFacts facts;memset(&facts,0x5a,sizeof facts);const auto saved=facts;
 alpha::CombatResult result;memset(&result,0x5a,sizeof result);const auto result_before=result;
 const auto e=alpha::world_resolve_field_combat(w,definitions,technologies,basic,0,1,rng,facts,result);
 if(digest(w)!=before)return 21;
 if(e!=alpha::CombatError::ok&&(rng.cursor||memcmp(&facts,&saved,sizeof facts)||memcmp(&result,&result_before,sizeof result)))return 21;
 printf("error=%s cursor=%lu",alpha::combat_error_name(e),(unsigned long)rng.cursor);
 if(e==alpha::CombatError::ok)printf(" offense=%ld defense=%ld ar=%ld dr=%ld ad=%ld dd=%ld am=%lu dm=%lu terrain=%ld left=%ld after_ad=%ld after_dd=%ld ahp=%ld dhp=%ld rounds=%lu ah=%lu dh=%lu draws=%lu win=%d",(long)facts.duel.offense,(long)facts.duel.defense,(long)facts.duel.attacker_reactor,(long)facts.duel.defender_reactor,(long)facts.duel.attacker_damage,(long)facts.duel.defender_damage,(unsigned long)facts.attacker_morale,(unsigned long)facts.defender_morale,(long)facts.terrain_defense,(long)facts.attacker_moves_left,(long)result.attacker_damage,(long)result.defender_damage,(long)result.attacker_hp,(long)result.defender_hp,(unsigned long)result.rounds,(unsigned long)result.attacker_hits,(unsigned long)result.defender_hits,(unsigned long)result.random_draws,(int)result.attacker_won);
 puts(" unchanged=1 rc=0");return 0;
}
