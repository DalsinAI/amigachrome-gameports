/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_keyed_quad.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
unsigned bits(uint64_t x){unsigned n=0;while(x){++n;x>>=1;}return n;}
int64_t gradient(int64_t delta,unsigned width,unsigned scale){
 const uint64_t d=(uint64_t)width<<16;
 const unsigned top=23+bits(d-1);
 const uint64_t mant=(UINT64_C(1)<<top)/d;
 uint64_t p=(uint64_t)(delta<0?-delta:delta)*mant;
 const unsigned b=bits(p),cut=b>24?b-24:0;
 p=(p>>cut)<<cut;
 const int64_t result=(int64_t)(p>>(top-scale));
 return delta<0?-result:result;
}
int64_t floor_div(int64_t n,int64_t d){return n>=0?n/d:-((-n+d-1)/d);}
struct Edge {int64_t x,u,v;};
bool edge_at(const TerrainKeyedPoint*v,unsigned top,int direction,int y,Edge&e){
 unsigned a=top;
 for(unsigned i=0;i<4;++i){
  const unsigned b=(a+4+direction)%4;
  const int dy=v[b].y-v[a].y;
  if(dy>0&&y>=v[a].y&&y<v[b].y){
   const int t=y-v[a].y;
   const int64_t dx=(int64_t)(v[b].x-v[a].x)*t;
   e.x=v[a].x-floor_div(-dx,dy);
   const int64_t u=(int64_t)v[a].u*65536;
   const int64_t w=(int64_t)v[a].v*65536;
   e.u=u+(((int64_t)v[b].u*65536-u)/dy)*t;
   e.v=w+(((int64_t)v[b].v*65536-w)/dy)*t;
   return true;
  }
  if(dy<0)return false;
  a=b;
 }
 return false;
}
bool pass(const TerrainKeyedQuadInput&in,const TerrainTextureSurface&s,bool write,int key,int fog){
 unsigned top=0;int bottom=in.vertex[0].y;for(unsigned k=1;k<4;++k){if(in.vertex[k].y<in.vertex[top].y)top=k;if(in.vertex[k].y>bottom)bottom=in.vertex[k].y;}
 const int first=in.vertex[top].y<0?0:in.vertex[top].y,end=bottom>(int)s.height?(int)s.height:bottom;
 for(int y=first;y<end;++y){Edge l{},r{};if(!edge_at(in.vertex,top,-1,y,l)||!edge_at(in.vertex,top,1,y,r))return false;if(r.x<=l.x)continue;
  const unsigned width=(unsigned)(r.x-l.x);const int64_t du=gradient(r.u-l.u,width,16),dv=gradient(r.v-l.v,width,16);const int firstx=l.x<0?0:(int)l.x,lastx=r.x>(int)s.width?(int)s.width:(int)r.x;
  for(int x=firstx;x<lastx;++x){const int64_t t=x-l.x,u=floor_div(l.u+du*t,65536),v=floor_div(l.v+dv*t,65536);if(u<0||v<0||u>=in.width||v>=in.height)return false;uint8_t pixel=in.texels[(size_t)v*in.width+u];if(write&&pixel!=key)s.pixels[(size_t)y*s.pitch+x]=(fog>=0&&(y&1)==0)?(uint8_t)fog:pixel;}
 }return true;
}
}
bool draw_terrain_keyed_quad(const TerrainKeyedQuadInput&in,const TerrainTextureSurface&s,int key,int fog){
 if(!in.texels||!in.width||!in.height||in.width>256||in.height>256||key<0||key>255||fog< -1||fog>255||in.texture_bytes<(size_t)in.width*in.height||!s.pixels||!s.width||!s.height||s.width>4096||s.height>4096||s.pitch<s.width||s.pitch>65536)return false;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width;if(used>s.bytes||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,in.texels,(size_t)in.width*in.height))return false;
 for(const auto&p:in.vertex)if(p.x< -4096||p.x>4096||p.y< -4096||p.y>4096||p.u<0||p.u>255||p.v<0||p.v>255)return false;
 return pass(in,s,false,key,fog)&&pass(in,s,true,key,fog);
}
}
