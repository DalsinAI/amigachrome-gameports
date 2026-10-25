/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_scene_blit.h"
#include "pcx_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static uint8_t source[65536],palette[768],dest[300*1216];
static uint32_t digest(){uint32_t h=2166136261u;for(auto v:dest)h=(h^v)*16777619u;return h;}
int main(int argc,char**argv){for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x)source[y*256+x]=(uint8_t)((x+y*3)%255);for(unsigned i=0;i<256;++i){palette[i*3]=(uint8_t)i;palette[i*3+1]=(uint8_t)(i*3);palette[i*3+2]=(uint8_t)(i*7);}const int32_t positions[]={-300,0,64,200,500};const alpha::CvrSceneRect clips[]={{0,0,400,300},{53,49,311,251},{0,0,0,0}};alpha::CvrSceneRgbTarget target{dest,sizeof(dest),400,300,1216};
 for(auto cx:positions)for(auto cy:positions)for(const auto&clip:clips){memset(dest,85,sizeof(dest));if(!alpha::composite_cvr_scene_1x(source,sizeof(source),palette,sizeof(palette),cx,cy,clip,target))return 20;printf("%d %d %d %08x\n",cx,cy,clip.right,digest());}
 const uint32_t before=digest();const alpha::CvrSceneRect clip{0,0,400,300};if(alpha::composite_cvr_scene_1x(source,65535,palette,768,0,0,clip,target)||alpha::composite_cvr_scene_1x(source,65536,palette,767,0,0,clip,target))return 20;auto bad=target;bad.capacity=1;if(alpha::composite_cvr_scene_1x(source,65536,palette,768,0,0,clip,bad))return 20;bad=target;bad.pixels=source;if(alpha::composite_cvr_scene_1x(source,65536,palette,768,0,0,clip,bad)||digest()!=before)return 20;int32_t x=7,y=9;if(!alpha::cvr_unit_anchor_zero_zoom(100,200,13,12,x,y)||x!=163||y!=262)return 20;x=7;y=9;if(alpha::cvr_unit_anchor_zero_zoom(INT32_MAX,0,13,12,x,y)||x!=7||y!=9)return 20;
 if(argc==2){FILE*f=fopen(argv[1],"rb");if(!f)return 20;fseek(f,0,SEEK_END);const long n=ftell(f);rewind(f);if(n<=0||n>16*1024*1024){fclose(f);return 20;}auto*data=(uint8_t*)malloc(n);if(!data){fclose(f);return 20;}bool ok=fread(data,1,n,f)==(size_t)n;fclose(f);alpha::PcxImage image;ok=ok&&image.load(data,n);memset(data,0xa5,n);free(data);if(!ok)return 20;for(unsigned chassis=0;chassis<2;++chassis){if(!alpha::cvr_basic_unit_marker(image,chassis,x,y)||x!=(chassis?14:13)||y!=(chassis?14:12))return 20;}x=7;y=9;if(alpha::cvr_basic_unit_marker(image,2,x,y)||x!=7||y!=9)return 20;}
 puts("BLIT 75clip/key/padding +4invalid +2anchor +optional2marker controls PASS");return 0;}
