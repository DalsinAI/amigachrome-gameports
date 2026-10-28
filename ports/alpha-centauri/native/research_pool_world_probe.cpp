/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<1||size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;size_t n=0;auto* data=read(argv[2],n);if(!data)return 20;auto error=alpha::read_technologies(data,n,rules);free(data);if(error!=alpha::Error::ok)return 20;
 data=read(argv[3],n);if(!data)return 20;alpha::OriginalWorld world;auto loaded=world.load(data,n,0xabcdefu);free(data);if(loaded!=alpha::WorldLoadError::ok)return 20;
 for(int faction=0;faction<8;++faction){alpha::HumanResearchPool pool{};auto e=alpha::world_human_research_pool(world,rules,variant,faction,pool);printf("POOL %d %d %u %lu",faction,(int)e,(unsigned)pool.path,(unsigned long)pool.count);for(size_t i=0;e==alpha::LogicError::ok && i<pool.count;++i)printf(" %u:%u",pool.ids[i],pool.categories[i]);puts("");}
 alpha::OriginalWorld absent;alpha::HumanResearchPool sentinel{};sentinel.count=7;
 if(alpha::world_human_research_pool(absent,rules,variant,1,sentinel)!=alpha::LogicError::bad_state || sentinel.count!=7)return 21;
 if(alpha::world_human_research_pool(world,rules,variant,-1,sentinel)!=alpha::LogicError::bad_state || sentinel.count!=7 || alpha::world_human_research_pool(world,rules,variant,8,sentinel)!=alpha::LogicError::bad_state || sentinel.count!=7)return 21;
 if(world.runtime_seed()!=0xabcdefu)return 21;
 return 0;
}
