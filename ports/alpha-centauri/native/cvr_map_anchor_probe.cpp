/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_map_anchor.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <initializer_list>
static bool controls(){
 for(int z=-8;z<=8;++z){alpha::CvrMapScale s{z,0,16};alpha::CvrMapMetrics m{};const int n=(50*(z+16)/16+1)/4;if(!alpha::cvr_map_metrics(s,m)||m.scaled50!=50*(z+16)/16||m.half_x!=4*n||m.half_y!=2*n)return false;}
 for(unsigned z=1;z<=2;++z)for(bool infantry:{false,true}){alpha::CvrViewerUnitAnchor a{};if(!alpha::cvr_viewer_flat_anchor(z,464,240,infantry,a)||a.caller.x!=464-(int)z*32||a.caller.y!=240-(int)z*32||a.scene_centre.x!=464+(z==1?2:1)||a.scene_centre.y!=240+(z==1?2:1)||a.original_zoom!=(z==1?-5:5))return false;}
 alpha::CvrViewerUnitAnchor a{},before=a;if(alpha::cvr_viewer_flat_anchor(0,0,0,false,a)||alpha::cvr_viewer_flat_anchor(3,0,0,false,a)||alpha::cvr_viewer_flat_anchor(1,INT_MIN,0,false,a)||memcmp(&a,&before,sizeof(a)))return false;
 alpha::CvrMapPlacement c{48,48,false,0,0,10,20,48,24,10,10,10,10};int32_t heights[4]={0,0,0,0};alpha::CvrMapPoint p{7,9},saved=p;
 auto bad=c;bad.width=3;if(alpha::cvr_map_tile_origin(bad,2,2,heights,p)||alpha::cvr_map_tile_origin(c,1,2,heights,p)||alpha::cvr_map_tile_origin(c,2,2,nullptr,p)||memcmp(&p,&saved,sizeof(p)))return false;
 const auto prior=c;if(alpha::cvr_map_tile_origin(c,2,2,heights,*reinterpret_cast<alpha::CvrMapPoint*>((uint8_t*)&c+sizeof(c)-4))||memcmp(&c,&prior,sizeof(c)))return false;
 const int32_t hs[]={0,0,0,0};if(alpha::cvr_map_tile_origin(c,2,2,heights,*reinterpret_cast<alpha::CvrMapPoint*>(heights+3))||memcmp(heights,hs,sizeof(hs)))return false;
 return true;
}
static int metrics(const char*path){FILE*f=fopen(path,"rb");if(!f)return 20;uint8_t b[12];unsigned count=0;while(true){const size_t n=fread(b,1,12,f);if(!n)break;if(n!=12){fclose(f);return 20;}int32_t v[3];for(unsigned i=0;i<3;++i){const uint32_t w=(uint32_t)b[4*i]|(uint32_t)b[4*i+1]<<8|(uint32_t)b[4*i+2]<<16|(uint32_t)b[4*i+3]<<24;memcpy(v+i,&w,4);}alpha::CvrMapScale scale{v[0],v[1],v[2]};alpha::CvrMapMetrics m{};if(!alpha::cvr_map_metrics(scale,m)){fclose(f);return 20;}for(int32_t q:{m.tile_x,m.half_x,m.tile_y,m.half_y,m.half_y,m.half_y/2})for(unsigned j=0;j<4;++j)putchar((uint32_t)q>>(j*8)&255);++count;}const bool ok=!ferror(f)&&!ferror(stdout);fclose(f);fprintf(stderr,"MAP METRICS %u vectors PASS\n",count);return ok?0:20;}
int main(int argc,char**argv){if(!controls())return 20;if(argc==3&&!strcmp(argv[1],"--metrics"))return metrics(argv[2]);if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;unsigned count=0;uint8_t bytes[72];while(true){const size_t n=fread(bytes,1,sizeof(bytes),f);if(!n)break;if(n!=sizeof(bytes)){fclose(f);return 20;}int32_t v[18];for(unsigned i=0;i<18;++i){const uint32_t w=(uint32_t)bytes[i*4]|(uint32_t)bytes[i*4+1]<<8|(uint32_t)bytes[i*4+2]<<16|(uint32_t)bytes[i*4+3]<<24;memcpy(v+i,&w,4);}alpha::CvrMapPlacement c{48,48,v[0]!=0,v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11]};alpha::CvrMapPoint p{};if(!alpha::cvr_map_tile_origin(c,v[12],v[13],v+14,p)){fclose(f);return 20;}for(int32_t q:{p.x,p.y})for(unsigned j=0;j<4;++j)putchar((uint32_t)q>>(j*8)&255);++count;}const bool ok=!ferror(f)&&!ferror(stdout);fclose(f);fprintf(stderr,"MAP ANCHOR %u vectors + metric/anchor/invalid/partial-alias controls PASS\n",count);return ok?0:20;}
