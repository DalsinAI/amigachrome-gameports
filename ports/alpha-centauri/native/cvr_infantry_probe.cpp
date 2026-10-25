/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_infantry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void* file(const char* p,size_t& n){FILE*f=fopen(p,"rb");if(!f)return nullptr;fseek(f,0,2);long z=ftell(f);rewind(f);if(z<=0 || z>16*1024*1024){fclose(f);return nullptr;}void*b=malloc(z);if(!b||fread(b,1,z,f)!=(size_t)z){free(b);fclose(f);return nullptr;}fclose(f);n=z;return b;}
int main(int argc,char**argv){if(argc!=4)return 20;size_t n=0,m=0;auto*a=file(argv[1],n);auto*b=file(argv[2],m);alpha::CvrInfantry r;if(!a||!b||!r.load(a,n,b,m))return 20;free(a);free(b);const size_t size=512*512*3;auto*rgb=(uint8_t*)malloc(size);if(!rgb)return 20;
 for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){memset(rgb,19,size);if(!r.draw(rgb,size,512,512,256,300,zoom,facing))return 20;unsigned h=2166136261u,count=0;for(size_t i=0;i<size;++i){h=(h^rgb[i])*16777619u;if(rgb[i]!=19)++count;}if(!count)return 20;printf("INFANTRY zoom=%u facing=%u changed=%u rgb=%08x\n",zoom,facing,count,h);if(zoom==2&&facing==0){FILE*f=fopen(argv[3],"wb");if(!f)return 20;fprintf(f,"P6\n512 512\n255\n");bool ok=fwrite(rgb,1,size,f)==size;fclose(f);if(!ok)return 20;}}
 memset(rgb,19,size);if(r.draw(rgb,size,512,512,256,300,1,8))return 20;for(size_t i=0;i<size;++i)if(rgb[i]!=19)return 20;free(rgb);puts("INFANTRY RC 0");return 0;}
