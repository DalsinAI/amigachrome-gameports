/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_lighting.h"
namespace alpha {namespace {
unsigned root(unsigned n){unsigned r=0,bit=1u<<30;while(bit>n)bit>>=2;while(bit){if(n>=r+bit){n-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}return r;}
int floor_shift(int n,unsigned bits){return n>=0?n/(1<<bits):-((-n+(1<<bits)-1)/(1<<bits));}
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
const OriginalMapTile* tile(const OriginalWorld&w,int x,int y){const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;return w.tile(((size_t)y*m.width+(unsigned)x)/2);}
bool bright(const OriginalWorld&w,int x,int y,unsigned triangle,int&result){
 const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}
 const auto*t=tile(w,x,y);if(!t||(t->climate>>5)<3){result=0;return true;}
 CvrTerrainCorners c{};TerrainBrightness b{};if(!cvr_terrain_corners(w,x,y,1,c)||!terrain_brightness(c.relative,b))return false;result=b.triangle[triangle];return true;
}
}
bool terrain_brightness(const uint8_t input[4],TerrainBrightness&out){
 if(!input)return false;
 uint8_t r[4];for(unsigned i=0;i<4;++i){r[i]=input[i];if(r[i]<1||r[i]>7)return false;}
 TerrainBrightness result{};
 for(unsigned k=0;k<4;++k){
  const int x=(k==1||k==2?4-r[2]:r[0]-4)*100;
  const int y=(k>=2?4-r[3]:r[1]-4)*100;
  const unsigned length=root((unsigned)(x*x+y*y));const int n=-3*(x+y);result.triangle[k]=(int8_t)(length?n/(int)length:n);
 }
 out=result;return true;
}
bool world_terrain_lights(const OriginalWorld&w,int x,int y,TerrainPointLights&out){
 const auto&m=w.game().map.info;if(!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return false;
 const auto*t=tile(w,x,y);if(!t||(t->landmarks&0x203u))return false;
 if(overlap(&out,sizeof out,&w,sizeof w)||overlap(&out,sizeof out,w.original_bytes(),w.original_size())||overlap(&out,sizeof out,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(&out,sizeof out,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(&out,sizeof out,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(&out,sizeof out,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(&out,sizeof out,w.faction(0),8*sizeof(OriginalFactionState))||overlap(&out,sizeof out,w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 TerrainPointLights result{};int sum=0,b=0;for(unsigned k=0;k<4;++k){if(!bright(w,x,y,k,b))return false;sum+=b;}result.point[0]=(int8_t)floor_shift(sum,2);
 const int dx[4][4]={{-2,-1,0,-1},{-1,0,1,0},{0,1,2,1},{-1,0,1,0}};
 const int dy[4][4]={{0,-1,0,1},{-1,-2,-1,0},{0,-1,0,1},{1,0,1,2}};
 for(unsigned c=0;c<4;++c){sum=0;for(unsigned k=0;k<4;++k){for(unsigned j=0;j<2;++j){if(!bright(w,x+dx[c][k],y+dy[c][k],(k+1+j)&3,b))return false;sum+=b;}}result.point[c+1]=(int8_t)floor_shift(sum,3);}
 out=result;return true;
}
}
