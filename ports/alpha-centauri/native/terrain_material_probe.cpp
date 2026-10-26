/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_material.h"
#include <stdio.h>
#include <string.h>
int main(){
 uint8_t raw[256],palette[768],pixels[256],rgb[768];for(unsigned i=0;i<256;++i)raw[i]=(uint8_t)i;for(unsigned i=0;i<768;++i)palette[i]=(uint8_t)(i*37+13);memset(pixels,199,sizeof pixels);memset(rgb,177,sizeof rgb);
 alpha::TerrainMaterialSource in{raw,sizeof raw,palette,sizeof palette};
 if(!alpha::prepare_terrain_material(in,pixels,sizeof pixels,rgb,sizeof rgb))return 20;
 for(unsigned i=0;i<256;++i)if(pixels[i]!=(uint8_t)(i+10))return 20;
 for(unsigned i=0;i<768;++i)if(rgb[i]!=(i>=30&&i<738?palette[i-30]:177))return 20;
 uint8_t p_before[256],r_before[768];memcpy(p_before,pixels,256);memcpy(r_before,rgb,768);
 auto unchanged=[&](){return !memcmp(pixels,p_before,256)&&!memcmp(rgb,r_before,768);};
 auto reject=[&](uint8_t*p,size_t n,uint8_t*r,size_t z){return !alpha::prepare_terrain_material(in,p,n,r,z)&&unchanged();};
 if(!reject(pixels,255,rgb,768)||!reject(pixels,256,rgb,767)||!reject(rgb+767,256,rgb,768)||!reject(raw+255,256,rgb,768)||!reject(pixels,256,palette+767,768)||!reject((uint8_t*)&in+1,256,rgb,768)||!reject(pixels,256,(uint8_t*)&in+sizeof in-1,768))return 20;
 in.rgb_bytes=767;if(!reject(pixels,256,rgb,768))return 20;in.rgb_bytes=768;
 in.pixel_count=0;if(!reject(pixels,256,rgb,768))return 20;in.pixel_count=256;
 fwrite(pixels,1,256,stdout);fwrite(rgb,1,768,stdout);return 0;
}
