/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_ocean_detail.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
const OriginalMapTile* tile(const OriginalWorld&w,int x,int y){const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;return w.tile(((size_t)y*m.width+(unsigned)x)/2);}
unsigned capped(const OriginalMapTile*t){return t?(t->contour>79?79:t->contour):0;}
const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};
void corner_offset(unsigned s,unsigned k,int&x,int&y){
 // Four tiles around the W,N,E,S vertex, in original slot order.
 const int cx[4]={-1,0,1,0},cy[4]={0,-1,0,1};
 const int ox[4]={-1,0,1,0},oy[4]={0,-1,0,1};
 x=cx[s]+ox[k];y=cy[s]+oy[k];
}
unsigned detail(const OriginalWorld&w,int x,int y,unsigned s,unsigned v){
 const auto*centre=tile(w,x,y);int n=centre->contour;
 if(v==2){n=0;for(unsigned k=0;k<4;++k){int a,b;corner_offset(s,k,a,b);n+=(int)capped(tile(w,x+a,y+b));}n/=4;int a,b;corner_offset(s,1,a,b);auto*t=tile(w,x+a,y+b);if(t&&(t->climate>>5)>=3)n+=7;corner_offset(s,3,a,b);t=tile(w,x+a,y+b);if(t&&(t->climate>>5)>=3)n-=3;}
 else if(v&1){unsigned d=(v+3+s*2)&7;n=((int)capped(centre)+(int)capped(tile(w,x+dx[d],y+dy[d])))/2;unsigned boundary=(s*2+4+(v==3?2:0))&7;auto*t=tile(w,x+dx[boundary],y+dy[boundary]);bool land=(centre->climate>>5)>=3;if(t&&((t->climate>>5)>=3)!=land){bool low=v==1?(s==0||s==3):(s==2||s==3);if(land)low=!low;return low?57:67;}}
 return n<0?0:n>79?79:(unsigned)n;
}
}
bool world_ocean_details(const OriginalWorld&w,int x,int y,TerrainOceanDetails&out){
 const auto&m=w.game().map.info;if(!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return false;
 if(overlap(&out,sizeof out,&w,sizeof w)||overlap(&out,sizeof out,w.original_bytes(),w.original_size())||overlap(&out,sizeof out,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(&out,sizeof out,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(&out,sizeof out,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(&out,sizeof out,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(&out,sizeof out,w.faction(0),8*sizeof(OriginalFactionState))||overlap(&out,sizeof out,w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 TerrainOceanDetails result{};for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v)result.value[s][v]=(uint8_t)detail(w,x,y,s,v);out=result;return true;
}
bool light_ocean_details(const TerrainOceanDetails&in,const TerrainOceanLightTable&t,TerrainOceanLights&out){
 if(!t.supplied||!t.by_detail||t.count<80||overlap(&out,sizeof out,&in,sizeof in)||overlap(&out,sizeof out,&t,sizeof t)||overlap(&out,sizeof out,t.by_detail,80*sizeof(int32_t)))return false;
 for(unsigned i=0;i<80;++i)if(t.by_detail[i]< -127||t.by_detail[i]>127)return false;
 TerrainOceanLights result{};for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v){unsigned d=in.value[s][v];if(d>=80)return false;result.point[s][v]=t.by_detail[d];}out=result;return true;
}
}
