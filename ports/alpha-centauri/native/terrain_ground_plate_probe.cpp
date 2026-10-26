/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_ground_plate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint32_t get(const uint8_t*b){return(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}

bool controls(const alpha::PreparedTerrainAtlas&atlas,uint8_t*pixels){
 alpha::TerrainGroundPlateInput input{};input.mesh={0,20,{0,0,0,0,0},1};input.material={123,1,1,42,false,false,true,true,true};
 alpha::PreparedTerrainAtlas a=atlas;alpha::TerrainTextureSurface surface{pixels,16384,128,128,128};memset(pixels,199,16384);
 auto clean=[&](){for(unsigned i=0;i<16384;++i)if(pixels[i]!=199)return false;return true;};
 auto reject=[&](){return !alpha::render_terrain_ground_plate(input,a,surface)&&clean();};
 input.material.source_address_supplied=false;if(!reject())return false;input.material.source_address_supplied=true;
 input.mesh.viewer_zoom=3;if(!reject())return false;input.mesh.viewer_zoom=1;
 input.mesh.height[4]=97;if(!reject())return false;input.mesh.height[4]=0;
 a.width=1;if(!reject())return false;a=atlas;
 surface.bytes=16383;if(!reject())return false;surface.bytes=16384;
 surface.width=257;if(!reject())return false;surface.width=128;
 surface.pixels=(uint8_t*)&input+4;if(!reject())return false;surface.pixels=pixels;
 surface.pixels=(uint8_t*)&a+1;if(!reject())return false;surface.pixels=pixels;
 surface.pixels=(uint8_t*)&surface+sizeof(surface)-1;if(!reject())return false;surface.pixels=pixels;
 surface.pixels=const_cast<uint8_t*>(a.pixels)+a.bytes-1;if(!reject())return false;surface.pixels=pixels;
 input.mesh.caller_x=4096;if(!reject())return false;input.mesh.caller_x=0;
 a.pixels=nullptr;if(!reject())return false;
 return true;
}
int main(){uint8_t*atlas=(uint8_t*)malloc(786432);if(!atlas)return 20;for(unsigned y=0;y<768;++y)for(unsigned x=0;x<1024;++x)atlas[y*1024+x]=(x+y*1024)%7==0?9:(uint8_t)((x*17+y*31)%236+10);alpha::PreparedTerrainAtlas a{atlas,786432,1024,768,1024};uint8_t rec[84];auto*pixels=(uint8_t*)malloc(16384);if(!pixels){free(atlas);return 20;}if(!controls(a,pixels)){free(pixels);free(atlas);return 20;}for(;;){size_t n=fread(rec,1,84,stdin);if(!n)break;if(n!=84){free(pixels);free(atlas);return 20;}memset(pixels,199,16384);alpha::TerrainGroundPlateInput in{};in.material={get(rec),(uint8_t)get(rec+4),(uint8_t)get(rec+8),(uint8_t)get(rec+12),get(rec+16)!=0,get(rec+20)!=0,get(rec+24)!=0,get(rec+28)!=0,true};in.mesh.caller_x=(int32_t)get(rec+32);in.mesh.caller_y=(int32_t)get(rec+36);for(unsigned k=0;k<5;++k){in.mesh.height[k]=(int32_t)get(rec+40+4*k);in.light.point[k]=(int8_t)get(rec+64+4*k);}in.mesh.viewer_zoom=get(rec+60);alpha::TerrainTextureSurface s{pixels,16384,128,128,128};if(!alpha::render_terrain_ground_plate(in,a,s)){free(pixels);free(atlas);return 20;}fwrite(pixels,1,16384,stdout);}free(pixels);free(atlas);return 0;}
