/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_faction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint32_t& h,uint32_t n){for(unsigned s=0;s<32;s+=8)h=(h^((n>>s)&255u))*16777619u;}
static uint32_t hash(const alpha::OriginalFactionDefinition& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned i=0;i<rows;++i)for(unsigned j=0;j<cols;++j)word(h,(uint32_t)r.name[i][j]);
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned i=0;i<rows;++i){for(unsigned j=0;j<cols;++j)h=(h^(uint8_t)r.name[i][j])*16777619u;if(r.name[i][cols]!=0)return 0;}
#define WIRE_GOALS(name,count,off) for(unsigned i=0;i<count;++i){word(h,(uint32_t)r.name[i].type);word(h,(uint32_t)r.name[i].priority);word(h,(uint32_t)r.name[i].x);word(h,(uint32_t)r.name[i].y);word(h,(uint32_t)r.name[i].base_id);}
#include "original_faction_definition_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
return h;}
static uint32_t hash(const alpha::OriginalFactionState& r){uint32_t h=2166136261u;
#define WIRE_SCALAR(type,name,off) word(h,(uint32_t)r.name);
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)word(h,(uint32_t)r.name[i]);
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)h=(h^(uint8_t)r.name[i])*16777619u;if(r.name[count]!=0)return 0;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned i=0;i<rows;++i)for(unsigned j=0;j<cols;++j)word(h,(uint32_t)r.name[i][j]);
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned i=0;i<rows;++i){for(unsigned j=0;j<cols;++j)h=(h^(uint8_t)r.name[i][j])*16777619u;if(r.name[i][cols]!=0)return 0;}
#define WIRE_GOALS(name,count,off) for(unsigned i=0;i<count;++i){word(h,(uint32_t)r.name[i].type);word(h,(uint32_t)r.name[i].priority);word(h,(uint32_t)r.name[i].x);word(h,(uint32_t)r.name[i].y);word(h,(uint32_t)r.name[i].base_id);}
#include "original_faction_state_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
return h;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);
 if(n<0 || n>512L*1024*1024){fclose(f);return 20;}
 uint8_t* data=(uint8_t*)malloc((size_t)n+1);if(!data){fclose(f);return 20;}
 bool read=fread(data,1,(size_t)n,f)==(size_t)n;fclose(f);alpha::OriginalGameView v;
 auto error=read?alpha::read_original_game(data,(size_t)n,v):alpha::GameReadError::truncated;
 if(error!=alpha::GameReadError::ok){printf("ERROR %s\n",alpha::game_read_error_name(error));free(data);return 20;}
 uint32_t states=2166136261u,defs=states;alpha::OriginalFactionDefinition d,before_d{};alpha::OriginalFactionState r,before_r{};
 for(unsigned i=0;i<8;++i){
  if(alpha::original_faction_state(v,i,r)!=alpha::GameReadError::ok || alpha::original_faction_definition(v,i,d)!=alpha::GameReadError::ok)return 21;
  word(states,hash(r));word(defs,hash(d));
 }
 before_r.energy_credits=-123;before_d.rule_techcost=-123;memcpy(&r,&before_r,sizeof(r));memcpy(&d,&before_d,sizeof(d));
 if(alpha::original_faction_state(v,8,r)!=alpha::GameReadError::bad_index || memcmp(&r,&before_r,sizeof(r)) ||
    alpha::original_faction_definition(v,8,d)!=alpha::GameReadError::bad_index || memcmp(&d,&before_d,sizeof(d)))return 21;
 auto bad=v;bad.factions.size=(v.data_version>=11?0x20ccu:0x20bcu)-1;bad.faction_definitions.size=0x59b;
 if(alpha::original_faction_state(bad,0,r)!=alpha::GameReadError::bad_index || memcmp(&r,&before_r,sizeof(r)) ||
    alpha::original_faction_definition(bad,0,d)!=alpha::GameReadError::bad_index || memcmp(&d,&before_d,sizeof(d)))return 21;
 printf("version=%lu states=%08lx definitions=%08lx rc=0\n",(unsigned long)v.data_version,(unsigned long)states,(unsigned long)defs);
 free(data);return 0;
}
