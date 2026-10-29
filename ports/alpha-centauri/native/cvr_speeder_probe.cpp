/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_infantry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void* file(const char* p,size_t& n){FILE*f=fopen(p,"rb");if(!f)return nullptr;fseek(f,0,2);long z=ftell(f);rewind(f);if(z<=0 || z>16*1024*1024){fclose(f);return nullptr;}void*b=malloc(z);if(!b||fread(b,1,z,f)!=(size_t)z){free(b);fclose(f);return nullptr;}fclose(f);n=z;return b;}
int main(int argc,char**argv){if(argc!=3)return 20;
 const char* names[]={"vrcp00.cvr","vgmc.cvr","vgmcp.cvr","vb.cvr","vbp.cvr","vsp.cvr","vsptb.cvr","vsptf.cvr","vlights.cvr","vr00.cvr"};
 alpha::CvrPartInput inputs[10]{};void* owned[10]{};bool valid=true;
 for(unsigned i=0;i<10;++i){char path[1024];if(snprintf(path,sizeof(path),"%s/%s",argv[1],names[i])>=(int)sizeof(path))return 20;owned[i]=file(path,inputs[i].size);inputs[i].data=owned[i];valid=valid&&owned[i];}
 alpha::CvrInfantry r;valid=valid&&r.load_speeder(inputs,10);for(auto*p:owned)free(p);if(!valid)return 20;
 const size_t size=512*512*3;auto*rgb=(uint8_t*)malloc(size);if(!rgb)return 20;
 for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){memset(rgb,19,size);if(!r.draw(rgb,size,512,512,256,300,zoom,facing))return 20;unsigned h=2166136261u,count=0;for(size_t i=0;i<size;++i){h=(h^rgb[i])*16777619u;if(rgb[i]!=19)++count;}if(!count)return 20;printf("SPEEDER zoom=%u facing=%u changed=%u rgb=%08x\n",zoom,facing,count,h);if(zoom==2&&facing==0){FILE*f=fopen(argv[2],"wb");if(!f)return 20;fprintf(f,"P6\n512 512\n255\n");bool ok=fwrite(rgb,1,size,f)==size;fclose(f);if(!ok)return 20;}}
 free(rgb);puts("SPEEDER RC 0");return 0;}
