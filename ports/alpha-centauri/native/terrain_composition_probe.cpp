/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_texture.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <initializer_list>
bool failed=false;
int32_t read32(){unsigned char b[4];if(fread(b,1,4,stdin)!=4){failed=true;return 0;}return (int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);}
bool reject_controls(){
 unsigned char tex[3136],pixels[4096],before[4096];memset(tex,77,sizeof tex);memset(pixels,99,sizeof pixels);memcpy(before,pixels,sizeof pixels);
 alpha::TerrainTextureInput in{{{32,32,27,28,0},{0,32,0,55,0},{32,0,0,1,0}},tex,sizeof tex,56,56};alpha::TerrainTextureSurface s{pixels,sizeof pixels,64,64,64};
 auto reject=[&](){return !alpha::draw_terrain_texture(in,s)&&memcmp(before,pixels,sizeof pixels)==0;};
 in.texels=pixels+4095;if(!reject())return false;in.texels=tex;
 s.pixels=(unsigned char*)&in+1;if(alpha::draw_terrain_texture(in,s))return false;s.pixels=pixels;
 s.pixels=(unsigned char*)&s+sizeof(s)-1;if(alpha::draw_terrain_texture(in,s))return false;s.pixels=pixels;
 in.texture_bytes=3135;if(!reject())return false;in.texture_bytes=3136;
 s.bytes=4095;if(!reject())return false;s.bytes=4096;
 in.vertex[2].u=4096;if(!reject())return false;in.vertex[2].u=0;
 in.vertex[0].u=255;if(!reject())return false;in.vertex[0].u=27;
 in.vertex[2].x=4097;if(!reject())return false;in.vertex[2].x=32;
 for(int key: {-2,256})if(alpha::draw_terrain_masked_texture(in,s,key)||memcmp(before,pixels,sizeof pixels))return false;
 for(int fog: {-2,256})if(alpha::draw_terrain_fog_texture(in,s,fog)||memcmp(before,pixels,sizeof pixels))return false;
 return true;
}
int main(){if(!reject_controls())return 20;unsigned char tex[3136],pixels[4096];for(unsigned i=0;i<3136;++i)tex[i]=i%200+20;int key=read32(),fog=read32();if(key>=0)for(unsigned i=0;i<3136;++i)if(i%3==0)tex[i]=9;for(int n=read32();n>0;--n){memset(pixels,199,sizeof pixels);alpha::TerrainTextureInput in{};in.texels=tex;in.texture_bytes=sizeof tex;in.texture_width=in.texture_height=56;for(auto&v:in.vertex){v.x=read32();v.y=read32();v.u=read32();v.v=read32();v.light=read32();}alpha::TerrainTextureSurface s{pixels,sizeof pixels,64,64,64};if(failed)return 20;putchar(alpha::draw_terrain_masked_texture(in,s,key,fog));fwrite(pixels,1,sizeof pixels,stdout);}return failed?20:0;}
