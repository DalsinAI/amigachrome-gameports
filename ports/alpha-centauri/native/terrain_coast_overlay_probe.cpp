/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_coast_overlay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t get(const uint8_t*p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool load(const char*name,uint8_t*p,size_t n){FILE*f=fopen(name,"rb");if(!f)return false;bool ok=fread(p,1,n,f)==n&&fgetc(f)==EOF;fclose(f);return ok;}
static bool controls(alpha::PreparedTerrainAtlas a,const alpha::TerrainCoastMetadata&m,const alpha::TerrainCoastResources&r,uint8_t*pixels){
 alpha::TerrainCoastOverlayInput in{};in.mesh={0,20,{0,0,0,0,0},1};in.fog_colour=-1;in.source_coast_context_supplied=true;for(auto&v:in.shore_mask)v=7;alpha::TerrainTextureSurface s{pixels,16384,128,128,128};memset(pixels,199,16384);
 auto clean=[&](){for(unsigned i=0;i<16384;++i)if(pixels[i]!=199)return false;return true;};auto reject=[&](){return !alpha::render_terrain_coast_overlay(in,m,r,a,s)&&clean();};
 in.source_coast_context_supplied=false;if(!reject())return false;in.source_coast_context_supplied=true;in.fog_colour=66;if(!reject())return false;in.fog_colour=-1;in.mesh.viewer_zoom=0;if(!reject())return false;in.mesh.viewer_zoom=1;in.mesh.height[0]=97;if(!reject())return false;in.mesh.height[0]=0;in.shore_mask[3]=8;if(!reject())return false;in.shore_mask[3]=7;in.light.point[3][3]=128;if(!reject())return false;in.light.point[3][3]=0;s.bytes=16383;if(!reject())return false;s.bytes=16384;
 const void*p[]={&in,&m,&r,&a,&s,a.pixels+a.bytes-1,m.classes+52,m.rules+423};for(auto q:p){s.pixels=(uint8_t*)const_cast<void*>(q);if(!reject())return false;}s.pixels=pixels;
 auto broken=r;memset(broken.mask,255,sizeof broken.mask);if(alpha::render_terrain_coast_overlay(in,m,broken,a,s)||!clean())return false;return true;
}
int main(int argc,char**argv){if(argc!=1&&argc!=4)return 20;auto*atlas=(uint8_t*)malloc(786432);auto*rain=(uint8_t*)malloc(307200);auto*pixels=(uint8_t*)malloc(16384);uint8_t meta[477];if(!atlas||!rain||!pixels){free(atlas);free(rain);free(pixels);return 20;}bool ok=true;
 if(argc==4)ok=load(argv[1],atlas,786432)&&load(argv[2],rain,307200)&&load(argv[3],meta,477);else{for(unsigned y=0;y<768;++y)for(unsigned x=0;x<1024;++x)atlas[y*1024+x]=(uint8_t)((x*17+y*31)%236+10);for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x)rain[y*640+x]=(uint8_t)((x*3+y*5)%53);for(unsigned i=0;i<53;++i){meta[i]=(uint8_t)i;for(unsigned k=0;k<8;++k)meta[53+i*8+k]=(uint8_t)((i*5+k)%6);}}
 alpha::PreparedTerrainAtlas a{atlas,786432,1024,768,1024};alpha::TerrainCoastMetadata m{meta,53,meta+53,424,true};alpha::TerrainCoastResources r{};ok=ok&&alpha::prepare_terrain_coast_resources(rain,307200,640,480,m,r)&&controls(a,m,r,pixels);
 uint8_t b[124];while(ok){size_t n=fread(b,1,sizeof b,stdin);if(!n)break;if(n!=sizeof b){ok=false;break;}alpha::TerrainCoastOverlayInput in{};in.mesh.caller_x=(int32_t)get(b);in.mesh.caller_y=(int32_t)get(b+4);for(unsigned k=0;k<5;++k)in.mesh.height[k]=(int32_t)get(b+8+k*4);in.mesh.viewer_zoom=get(b+28);in.map_x=(int32_t)get(b+32);in.map_y=(int32_t)get(b+36);in.fog_colour=(int32_t)get(b+40);for(unsigned k=0;k<4;++k){in.shore_mask[k]=(uint8_t)get(b+44+k*4);for(unsigned v=0;v<4;++v)in.light.point[k][v]=(int32_t)get(b+60+(k*4+v)*4);}in.source_coast_context_supplied=true;memset(pixels,199,16384);alpha::TerrainTextureSurface s{pixels,16384,128,128,128};ok=alpha::render_terrain_coast_overlay(in,m,r,a,s);if(ok)fwrite(pixels,1,16384,stdout);}
 free(atlas);free(rain);free(pixels);return ok?0:20;
}
