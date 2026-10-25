/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_infantry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t* read(const char* path,size_t& size){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0 || n>16*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc(n);if(!b || fread(b,1,n,f)!=(size_t)n){free(b);fclose(f);return nullptr;}fclose(f);size=(size_t)n;return b;}
static alpha::CvrUnitAssembly renderer;
int main(int argc,char**argv){if(argc!=2 && argc!=3)return 20;
 const char* names[]={"vrcp00.cvr","vgmc.cvr","vgmcp.cvr","vb.cvr","vbp.cvr","vsp.cvr","vsptb.cvr","vsptf.cvr","vlights.cvr","vr00.cvr"};
 alpha::CvrPartInput inputs[10]{};uint8_t* data[10]{};bool present=true;
 for(unsigned i=0;i<10;++i){char path[1024];if(snprintf(path,sizeof(path),"%s/%s",argv[1],names[i])>=(int)sizeof(path))return 20;data[i]=read(path,inputs[i].size);inputs[i].data=data[i];present=present&&data[i];}
 const bool loaded=present&&renderer.load_speeder(inputs,10);for(auto*p:data)free(p);if(!loaded)return 20;
 const size_t bytes=256*256*3;auto*rgb=(uint8_t*)malloc(bytes);if(!rgb)return 20;
 if(argc==3){
  uint8_t palette[768]{};palette[27]=9;if(!renderer.set_game_palette(palette,sizeof(palette)))return 20;
  for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){memset(rgb,19,bytes);if(!renderer.draw(rgb,bytes,256,256,128,200,zoom,facing))return 20;for(size_t i=0;i<bytes;++i)if(rgb[i]!=19)return 20;}
  free(rgb);puts("PALETTE RENDER original-key9 transparent16draws PASS");return 0;
 }
 auto check=[&](int red,int green,int blue){
  for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){
   memset(rgb,19,bytes);if(!renderer.draw(rgb,bytes,256,256,128,200,zoom,facing))return false;unsigned changed=0;
   for(size_t i=0;i<bytes;i+=3)if(rgb[i]!=19 || rgb[i+1]!=19 || rgb[i+2]!=19){if(rgb[i]!=red || rgb[i+1]!=green || rgb[i+2]!=blue)return false;++changed;}
   if(changed!=1)return false;
  }
  return true;
 };
 uint8_t palette[768]{};palette[16]=255;const uint8_t red_tint[3]={255,0,0},white_tint[3]={255,255,255};
 if(!check(255,0,0))return 20;
 if(!renderer.set_game_palette(palette,sizeof(palette)) || !check(0,255,0))return 20;
 if(!renderer.set_game_palette(palette,sizeof(palette)) || !check(0,255,0))return 20;
 if(!renderer.set_game_palette(palette,sizeof(palette),red_tint,3) || !check(0,0,0))return 20;
 if(!renderer.set_game_palette(palette,sizeof(palette),white_tint,3) || !check(0,255,0))return 20;
 if(renderer.set_game_palette(palette,sizeof(palette),red_tint,2) || !check(0,255,0))return 20;
 palette[16]=0;palette[17]=255;
 if(!renderer.set_game_palette(palette,sizeof(palette)) || !check(0,0,255))return 20;
 renderer.clear_game_palette();if(!check(255,0,0))return 20;
 renderer.clear_game_palette();if(!check(255,0,0))return 20;
 free(rgb);puts("PALETTE RENDER transitions9 facings16 cache-invalidations PASS");return 0;
}
