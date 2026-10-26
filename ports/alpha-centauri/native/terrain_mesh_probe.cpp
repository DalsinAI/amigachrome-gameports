/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_mesh.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char*p){return(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void emit(uint32_t x){for(unsigned i=0;i<4;++i)putchar((int)((x>>(i*8))&255));}
static bool aliases(){
 alpha::TerrainMeshInput source{-123,45,{20,0,32,12,17},2};alpha::TerrainMesh expected{};if(!alpha::build_terrain_mesh(source,expected))return false;
 alignas(alpha::TerrainMesh) unsigned char arena[sizeof(alpha::TerrainMesh)+96];
 for(unsigned k=0;k<3;++k){const unsigned off=k*32;memset(arena,0xa5,sizeof(arena));memcpy(arena+32,&source,sizeof(source));auto&in=*reinterpret_cast<alpha::TerrainMeshInput*>(arena+32);auto&out=*reinterpret_cast<alpha::TerrainMesh*>(arena+off);if(!alpha::build_terrain_mesh(in,out))return false;for(unsigned t=0;t<4;++t)for(unsigned v=0;v<3;++v){const auto&a=out.triangles[t][v];const auto&b=expected.triangles[t][v];if(a.x!=b.x||a.y!=b.y||a.corner!=b.corner)return false;}}
 return true;
}
int main(){if(!aliases())return 20;unsigned char bytes[32];size_t n;while((n=fread(bytes,1,sizeof(bytes),stdin))==sizeof(bytes)){
 alpha::TerrainMeshInput in{};in.caller_x=(int32_t)word(bytes);in.caller_y=(int32_t)word(bytes+4);for(unsigned i=0;i<5;++i)in.height[i]=(int32_t)word(bytes+8+i*4);in.viewer_zoom=word(bytes+28);
 alpha::TerrainMesh result;memset(&result,0xa5,sizeof(result));unsigned char before[sizeof(result)];memcpy(before,&result,sizeof(result));const bool ok=alpha::build_terrain_mesh(in,result);emit(ok?1:0);if(!ok){if(memcmp(before,&result,sizeof(result)))return 20;continue;}
 for(const auto&triangle:result.triangles)for(const auto&v:triangle){emit((uint32_t)v.x);emit((uint32_t)v.y);emit(v.corner);}
 }return n||ferror(stdin)||ferror(stdout)?20:0;}
