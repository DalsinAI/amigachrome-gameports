/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(n>>(8*i));fwrite(b,1,4,stdout);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);const long size=ftell(f);rewind(f);if(size<=0||size>16*1024*1024){fclose(f);return 20;}auto*data=(unsigned char*)malloc(size);if(!data){fclose(f);return 20;}bool ok=fread(data,1,size,f)==(size_t)size;fclose(f);alpha::CvrModel m;ok=ok&&m.load_at_scale(data,size,.15f);memset(data,0xa5,size);free(data);if(!ok)return 20;if(m.load("BAD",3))return 20;
 word(m.object_count());for(unsigned i=0;i<m.object_count();++i){const auto*o=m.object(i);word(o->geometry_flags);word(o->group_count);for(unsigned j=0;j<3;++j){word((uint32_t)(int32_t)o->dimensions[j]);word((uint32_t)(int32_t)o->position[j]);word((uint32_t)(int32_t)o->rotation_center[j]);}for(unsigned g=0;g<o->group_count;++g){const auto&v=o->groups[g];for(auto n:v.seed)word((uint32_t)n);word(v.first);word(v.count);word(v.stream_entries);for(auto n:v.jump_header)word((uint32_t)(int32_t)n);}}
 return ferror(stdout)?20:0;}
