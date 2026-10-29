/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void byte(uint32_t&h,unsigned x){h=(h^(x&255))*16777619u;}
static void word(uint32_t&h,uint32_t x){for(unsigned i=0;i<4;++i)byte(h,x>>(8*i));}
static void real(uint32_t&h,float x){uint32_t b;memcpy(&b,&x,4);word(h,b);}
static void hash(const alpha::CvrModel&m,uint32_t&g,uint32_t&a,uint32_t&p){g=a=p=2166136261u;for(unsigned i=0;i<m.object_count();++i){const auto*o=m.object(i);for(const char*s=o->name;*s;++s)byte(g,(unsigned char)*s);byte(g,0);real(g,o->units);word(g,o->count);for(unsigned j=0;j<o->count;++j){const auto&v=o->voxels[j];word(g,(uint32_t)(int32_t)v.z);word(g,(uint32_t)(int32_t)v.x);word(g,(uint32_t)(int32_t)v.y);word(g,v.normal);byte(g,v.color);}for(unsigned j=0;j<m.frame_count();++j){const auto&f=o->frames[j];byte(a,f.flags);for(float x:f.location)real(a,x);for(float x:f.matrix)real(a,x);}}
 for(unsigned shade=0;shade<24;++shade)for(unsigned c=0;c<256;++c){const auto*rgb=m.rgb((uint8_t)c,shade);for(int j=0;j<3;++j)byte(p,rgb[j]);}}
int main(int argc,char**argv){if(argc!=2 && argc!=3)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0 || n>16*1024*1024){fclose(f);return 20;}void*p=malloc(n);if(!p){fclose(f);return 20;}bool read=fread(p,1,n,f)==(size_t)n;fclose(f);alpha::CvrModel model;bool ok=read&&(argc==3?model.load_at_scale(p,n,strtof(argv[2],nullptr)):model.load(p,n));memset(p,0xa5,n);free(p);if(!ok){puts("CVR rejected");return 20;}uint32_t g,a,pal,g2,a2,p2;hash(model,g,a,pal);if(model.load("BAD",3))return 20;hash(model,g2,a2,p2);if(g!=g2 || a!=a2 || pal!=p2)return 20;printf("objects=%u frames=%u voxels=%u geometry=%08x animation=%08x palette=%08x owned=1 transactional=1 rc=0\n",model.object_count(),model.frame_count(),model.voxel_count(),g,a,pal);return 0;}
