/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_model.h"
#include "cvr_fixed_transform.h"
#include "cvr_lighting.h"
#include "cvr_raster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t pixels[65536],lattice[alpha::cvr_shade_lattice_size],shades[2048],palette[6144];static uint16_t depths[65536];
int main(int argc,char**argv){if(argc!=2&&argc!=3)return 20;if(argc==3&&strcmp(argv[2],"clipped"))return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);const long n=ftell(f);rewind(f);if(n<=0||n>16*1024*1024){fclose(f);return 20;}void*data=malloc(n);if(!data){fclose(f);return 20;}bool ok=fread(data,1,n,f)==(size_t)n;fclose(f);alpha::CvrModel model;ok=ok&&model.load_at_scale(data,n,.15f);memset(data,0xa5,n);free(data);if(!ok)return 20;
 for(auto&d:depths){d=65535;}for(unsigned s=0;s<24;++s)for(unsigned c=0;c<256;++c)palette[s*256+c]=(uint8_t)(c+13*s);
 if(!alpha::build_cvr_shade_lattice(lattice,sizeof(lattice))||!alpha::cvr_shades_for_light(lattice,sizeof(lattice),.2f,-.5f,.84f,shades,sizeof(shades)))return 20;
 const float origin[]={128,128,32000};alpha::CvrRasterSurface surface{pixels,depths,65536,256,256,256};const alpha::CvrRasterClip clip=argc==3?alpha::CvrRasterClip{64,64,160,160}:alpha::CvrRasterClip{0,0,255,255};
 for(unsigned k=0;k<model.object_count();++k){const auto*o=model.object(k);const float scale=.15f*o->units,basis[]={scale,0,0,0,scale,0,0,0,scale};alpha::CvrFixedBasis b{};if(!alpha::prepare_cvr_fixed_basis(basis,b))return 20;
  for(unsigned g=0;g<o->group_count;++g){const auto&group=o->groups[g];alpha::CvrFixedSeed seed{};if(!alpha::prepare_cvr_fixed_group_seed(origin,basis,group.seed,seed))return 20;auto*points=(alpha::CvrRasterPoint*)malloc((size_t)group.count*sizeof(alpha::CvrRasterPoint));if(!points&&group.count)return 20;
   for(unsigned i=0;i<group.count;++i){const auto&v=o->voxels[group.first+i];const int16_t raw[]={v.z,v.x,v.y};alpha::cvr_fixed_position(b,seed,raw,points[i].x,points[i].y,points[i].z);points[i].normal=v.normal;points[i].color=v.color;}
   ok=alpha::raster_cvr_points(points,group.count,shades,sizeof(shades),palette,sizeof(palette),clip,scale>.5f,surface);free(points);if(!ok)return 20;
  }
 }
 fwrite(pixels,1,sizeof(pixels),stdout);for(auto d:depths){putchar(d&255);putchar(d>>8);}return ferror(stdout)?20:0;
}
