/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_infantry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t* read(const char* path,size_t& size){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0 || n>16*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc(n);if(!b || fread(b,1,n,f)!=(size_t)n){free(b);fclose(f);return nullptr;}fclose(f);size=(size_t)n;return b;}
int main(int argc,char**argv){if(argc!=6)return 20;size_t n=0,m=0;auto*a=read(argv[1],n);auto*b=read(argv[2],m);if(!a || !b){free(a);free(b);return 20;}alpha::CvrInfantry renderer;const bool loaded=renderer.load(a,n,b,m);free(a);free(b);const size_t bytes=256*256*3;auto*rgb=(uint8_t*)malloc(bytes);if(!rgb)return 20;memset(rgb,19,bytes);
 const int expect_red=atoi(argv[3]),expect_blue=atoi(argv[4]);const bool want_valid=atoi(argv[5])!=0;
 const bool drawn=loaded && renderer.draw(rgb,bytes,256,256,128,200,1,3);
 if(drawn!=want_valid){free(rgb);return 20;}
 int red=0,blue=0,other=0;
 for(size_t i=0;i<bytes;i+=3){if(rgb[i]==255 && rgb[i+1]==0 && rgb[i+2]==0)++red;else if(rgb[i]==0 && rgb[i+1]==0 && rgb[i+2]==255)++blue;else if(rgb[i]!=19 || rgb[i+1]!=19 || rgb[i+2]!=19)++other;}
 if(red!=expect_red || blue!=expect_blue || other){free(rgb);return 20;}
 if(drawn){
  for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){memset(rgb,19,bytes);if(!renderer.draw(rgb,bytes,256,256,128,200,zoom,facing)){free(rgb);return 20;}}
  memset(rgb,47,bytes);if(!renderer.draw(rgb,bytes,256,256,128,200,1,3)){free(rgb);return 20;}
  int cached_red=0,cached_blue=0;
  for(size_t i=0;i<bytes;i+=3){if(rgb[i]==255 && rgb[i+1]==0 && rgb[i+2]==0)++cached_red;else if(rgb[i]==0 && rgb[i+1]==0 && rgb[i+2]==255)++cached_blue;else if(rgb[i]!=47 || rgb[i+1]!=47 || rgb[i+2]!=47){free(rgb);return 20;}}
  if(cached_red!=expect_red || cached_blue!=expect_blue){free(rgb);return 20;}
  struct Bad{size_t capacity;unsigned w,h,zoom,facing;int x,y;};
  const Bad invalid[]={{bytes-1,256,256,1,0,128,200},{bytes,256,256,0,0,128,200},{bytes,256,256,3,0,128,200},{bytes,256,256,1,8,128,200},{bytes,0,256,1,0,128,200},{bytes,256,256,1,0,-1,200},{bytes,256,256,1,0,128,256}};
  for(const auto&v:invalid){memset(rgb,19,bytes);if(renderer.draw(rgb,v.capacity,v.w,v.h,v.x,v.y,v.zoom,v.facing)){free(rgb);return 20;}for(size_t i=0;i<bytes;++i)if(rgb[i]!=19){free(rgb);return 20;}}
 }
 free(rgb);printf("red=%d blue=%d other=%d valid=%d facings=%d invalid=%d rc=0\n",red,blue,other,drawn,drawn?16:0,drawn?7:0);return 0;
}
