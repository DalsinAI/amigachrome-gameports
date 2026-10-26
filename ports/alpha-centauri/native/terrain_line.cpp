/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_line.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
}
bool draw_terrain_line(const TerrainLineInput&in,const TerrainTextureSurface&s){
 if(!s.pixels||!s.width||!s.height||s.width>4096||s.height>4096||s.pitch<s.width||s.pitch>65536||in.colour>255||in.x0< -4096||in.x0>4096||in.y0< -4096||in.y0>4096||in.x1< -4096||in.x1>4096||in.y1< -4096||in.y1>4096)return false;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;if(used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&s,sizeof s))return false;
 int x=in.x0,y=in.y0,ex=in.x1,ey=in.y1;if(x==ex&&y==ey)return true;
 if(ex<x){int t=x;x=ex;ex=t;t=y;y=ey;ey=t;}
 const int dx=ex-x,dy=ey-y,sy=dy<0?-1:1,ady=dy<0?-dy:dy;
 auto plot=[&](){if(x>=0&&y>=0&&x<(int)s.width&&y<(int)s.height)s.pixels[(size_t)y*s.pitch+x]=(uint8_t)in.colour;};
 if(dx<ady){int error=2*dx-ady;for(int k=0;k<=ady;++k){plot();if(error>0){++x;error+=2*(dx-ady);}else error+=2*dx;y+=sy;}}
 else{int error=2*ady-dx;for(int k=0;k<=dx;++k){plot();if(error>0){y+=sy;error+=2*(ady-dx);}else error+=2*ady;++x;}}
 return true;
}
}
