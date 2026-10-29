/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_options.h"
#include "upstream_research_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;
static uint8_t* read(const char* path,size_t& n){FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long len=ftell(f);rewind(f);if(len<1||len>64*1024*1024){fclose(f);return nullptr;}auto* bytes=(uint8_t*)malloc((size_t)len);if(!bytes){fclose(f);return nullptr;}bool ok=fread(bytes,1,(size_t)len,f)==(size_t)len;fclose(f);if(!ok){free(bytes);return nullptr;}n=(size_t)len;return bytes;}
static uint32_t hash(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;
 size_t n=0;auto* bytes=read(argv[2],n);if(!bytes)return 20;auto error=alpha::read_technologies(bytes,n,rules);free(bytes);if(error!=alpha::Error::ok)return 20;
 bytes=read(argv[3],n);if(!bytes)return 20;alpha::OriginalWorld world;auto loaded=world.load(bytes,n,0x12345678u);free(bytes);if(loaded!=alpha::WorldLoadError::ok)return 20;
 const uint32_t before=hash(world.original_bytes(),world.original_size());research_reference::setup(world,rules,variant);
 alpha::ResearchOptions result{};unsigned cases=0;
 for(int faction=0;faction<8;++faction){
  if(alpha::world_research_options(world,rules,variant,faction,result)!=alpha::LogicError::ok)return 21;
  const auto* saved=world.faction((size_t)faction);
  if(result.current_id!=saved->tech_research_id || result.accumulated!=saved->tech_accumulated || result.cost!=saved->tech_cost || result.transcendent_count!=saved->tech_count_transcendent)return 21;
  printf("FACTION %d %ld %ld %ld %ld %d %lu %lu\n",faction,(long)result.current_id,(long)result.accumulated,(long)result.cost,(long)result.transcendent_count,result.current_id_supported,(unsigned long)result.known_count,(unsigned long)result.available_count);
  for(unsigned id=0;id<89;++id){const auto& item=result.technologies[id];bool available=item.availability==alpha::ResearchAvailability::available;
   if(available!=research_reference::tech_avail(id,faction) || item.known!=research_reference::has_tech((int)id,faction))return 21;
   printf("TECH %d %u %d %d %d %u %u\n",faction,id,available,item.known,item.repeatable,item.missing_prerequisites,(unsigned)item.availability);++cases;
  }
 }
 alpha::ResearchOptions sentinel;memset(&sentinel,0,sizeof(sentinel));sentinel.current_id=123;alpha::ResearchOptions copy;memcpy(&copy,&sentinel,sizeof(copy));
 const int invalid_factions[]={-1,8};const int invalid_prerequisites[]={-3,89};
 for(int invalid:invalid_factions){if(alpha::world_research_options(world,rules,variant,invalid,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&copy,sizeof(copy)))return 21;}
 alpha::OriginalWorld empty;if(alpha::world_research_options(empty,rules,variant,1,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&copy,sizeof(copy)))return 21;
 if(alpha::world_research_options(world,rules,(alpha::Variant)99,1,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&copy,sizeof(copy)))return 21;
 --rules.count;if(alpha::world_research_options(world,rules,variant,1,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&copy,sizeof(copy)))return 21;++rules.count;
 for(int invalid:invalid_prerequisites){int32_t old=rules.technologies[0].prerequisites[0];rules.technologies[0].prerequisites[0]=invalid;if(alpha::world_research_options(world,rules,variant,1,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&copy,sizeof(copy)))return 21;rules.technologies[0].prerequisites[0]=old;}
 if(before!=hash(world.original_bytes(),world.original_size()) || world.runtime_seed()!=0x12345678u)return 21;
 printf("RESEARCH queries=%u invalid=7 unchanged=1 RC=0\n",cases);return 0;
}
