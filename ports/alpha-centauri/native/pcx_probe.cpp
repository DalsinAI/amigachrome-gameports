/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pcx_image.h"
#include <stdio.h>
#include <stdlib.h>
static uint32_t hash(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END)){fclose(f);return 20;}long n=ftell(f);rewind(f);
 if(n<=0 || n>16*1024*1024){fclose(f);return 20;}auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return 20;}bool ok=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);
 alpha::PcxImage image;if(!ok || !image.load(p,(size_t)n)){free(p);return 20;}free(p);
 const auto pixels=hash(image.pixels(),(size_t)image.width()*image.height()),palette=hash(image.palette(),768);
 if(image.load(nullptr,0) || image.width()==0 || image.height()==0 || hash(image.pixels(),(size_t)image.width()*image.height())!=pixels || hash(image.palette(),768)!=palette || image.rgb(image.width(),0) || image.rgb(0,image.height()))return 21;
 printf("width=%lu height=%lu pixels=%08lx palette=%08lx rc=0\n",(unsigned long)image.width(),(unsigned long)image.height(),(unsigned long)pixels,(unsigned long)palette);return 0;
}
