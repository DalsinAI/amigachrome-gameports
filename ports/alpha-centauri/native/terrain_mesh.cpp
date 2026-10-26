/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_mesh.h"
#include <limits.h>
namespace alpha {
bool build_terrain_mesh(const TerrainMeshInput&input,TerrainMesh&out){
 const TerrainMeshInput in=input;
 if(in.viewer_zoom!=1&&in.viewer_zoom!=2)return false;
 for(int32_t h:in.height)if(h<0||h>96)return false;
 const int source_zoom=in.viewer_zoom==1?-5:5;
 const int quarter=(((source_zoom+16)*50/16)+1)/4,half=quarter*2;
 const int64_t cx=(int64_t)in.caller_x+half*2;
 const int64_t cy=(int64_t)in.caller_y+(in.height[1]+in.height[2]+in.height[3]+in.height[4])/4+half;
 TerrainMesh result{};
 for(unsigned triangle=0;triangle<4;++triangle)for(unsigned vertex=0;vertex<3;++vertex){
  const unsigned corner=vertex==0?0:1+(triangle+vertex-1)%4;
  const int dx=corner==1?-2:corner==3?2:0;
  const int dy=corner==2?-2:corner==4?2:0;
  const int64_t x=cx+dx*half,y=cy+dy*quarter-in.height[corner];
  if(x<INT32_MIN||x>INT32_MAX||y<INT32_MIN||y>INT32_MAX)return false;
  result.triangles[triangle][vertex]={(int32_t)x,(int32_t)y,(uint8_t)corner};
 }
 out=result;return true;
}
}
