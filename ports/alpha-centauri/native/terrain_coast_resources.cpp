/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_coast_resources.h"
namespace alpha {namespace {
bool overlaps(const void*a,size_t n,const void*b,size_t m){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(n&&m&&x<y+m&&y<x+n);}
bool metadata(const TerrainCoastMetadata&m){if(!m.supplied||!m.classes||!m.rules||m.class_bytes<53||m.rule_bytes<53*8)return false;for(unsigned i=0;i<53;++i){for(unsigned j=0;j<i;++j)if(m.classes[i]==m.classes[j])return false;for(unsigned k=0;k<8;++k)if(m.rules[i*8+k]>5)return false;}return true;}
int32_t midpoint(int32_t a,int32_t b){int32_t n=a+b;return n>=0?n/2:-((-n+1)/2);}
}
bool prepare_terrain_coast_resources(const uint8_t*rain,size_t bytes,unsigned w,unsigned h,const TerrainCoastMetadata&m,TerrainCoastResources&out){
 if(!metadata(m)||!rain||w<186||h<395||w>4096||h>4096||bytes<(size_t)w*h||overlaps(&out,sizeof out,rain,(size_t)w*h)||overlaps(&out,sizeof out,&m,sizeof m)||overlaps(&out,sizeof out,m.classes,53)||overlaps(&out,sizeof out,m.rules,424))return false;
 TerrainCoastResources result{};
 for(unsigned y=0;y<56;++y)for(unsigned x=0;x<56;++x){uint8_t p=rain[(size_t)(333+y)*w+2+x];if(p==255||p==252||p==253)return false;uint8_t code=0;for(unsigned i=0;i<53;++i)if(m.classes[i]==p)code=(uint8_t)i;result.mask[y*56+x]=code;}
 for(unsigned i=0;i<6;++i){unsigned j=i+2;uint8_t p=rain[(size_t)(344+(j/4)*17)*w+130+(j%4)*14];result.palette[i]=(p==252||p==253)?9:(uint8_t)(p+10);}
 out=result;return true;
}
bool prepare_terrain_coast_sector(const TerrainMesh&mesh,unsigned sector,uint8_t shore,int32_t x,int32_t y,const TerrainCoastMetadata&m,const TerrainCoastResources&r,TerrainCoastSector&out){
 if(sector>=4||shore>7||!metadata(m)||overlaps(&out,sizeof out,&mesh,sizeof mesh)||overlaps(&out,sizeof out,&m,sizeof m)||overlaps(&out,sizeof out,&r,sizeof r)||overlaps(&out,sizeof out,m.classes,53)||overlaps(&out,sizeof out,m.rules,424))return false;
 TerrainCoastSector v{};const unsigned previous=(sector+3)&3;
 const auto*centre=&mesh.triangles[previous][0];const auto*before=&mesh.triangles[previous][1];const auto*corner=&mesh.triangles[previous][2];const auto*after=&mesh.triangles[sector][2];
 for(const auto&p:mesh.triangles)for(const auto&q:p)if(q.x< -4096||q.x>4096||q.y< -4096||q.y>4096)return false;
 // Sector0 is west: original mesh triangle3 ends at W. Sector1 is north.
 v.xy[0][0]=centre->x;v.xy[0][1]=centre->y;v.xy[1][0]=midpoint(before->x,corner->x);v.xy[1][1]=midpoint(before->y,corner->y);v.xy[2][0]=corner->x;v.xy[2][1]=corner->y;v.xy[3][0]=midpoint(corner->x,after->x);v.xy[3][1]=midpoint(corner->y,after->y);
 const int u0=(sector==1||sector==2)?28:27,v0=sector>=2?28:27;
 const int du[4]={-27,0,27,0},dv[4]={0,-27,0,27};
 v.uv[0]={u0,v0};v.uv[1]={u0+du[sector],v0+dv[sector]};v.uv[2]={v.uv[1].u+du[(sector+1)&3],v.uv[1].v+dv[(sector+1)&3]};v.uv[3]={u0+du[(sector+1)&3],v0+dv[(sector+1)&3]};
 for(unsigned i=0;i<53;++i){uint8_t value=m.rules[i*8+shore];v.lookup[i]=value>1?r.palette[value]:value;}
 if(shore==7&&(((uint32_t)y-(uint32_t)x)&2)){v.lookup[5]=0;for(unsigned i=14;i<18;++i)v.lookup[i]=0;for(unsigned base=18;base<=26;base+=4)for(unsigned i=0;i<4;++i)v.lookup[base+i]=r.palette[5-i];}
 out=v;return true;
}
}
