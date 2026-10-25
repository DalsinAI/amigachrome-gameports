/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pcx_image.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {
PcxImage::~PcxImage(){free(pixels_);}
static uint32_t le16(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8);}
bool PcxImage::load(const uint8_t* d,size_t n){
 if(!d || n<128+769 || d[0]!=10 || d[1]!=5 || d[2]!=1 || d[3]!=8 || d[65]!=1 || d[n-769]!=12)return false;
 const uint32_t x0=le16(d+4),y0=le16(d+6),x1=le16(d+8),y1=le16(d+10),stride=le16(d+66);
 if(x1<x0 || y1<y0)return false;
 const uint32_t w=x1-x0+1,h=y1-y0+1;
 if(w>2048 || h>2048 || stride<w || stride>4096 || (stride&1))return false;
 auto* p=(uint8_t*)malloc((size_t)w*h);if(!p)return false;
 size_t pos=128,total=(size_t)stride*h,decoded=0,end=n-769;
 while(decoded<total){
  if(pos>=end){free(p);return false;}uint8_t value=d[pos++];size_t run=1;
  if((value&0xc0)==0xc0){run=value&63u;if(!run || pos>=end){free(p);return false;}value=d[pos++];}
  if(run>total-decoded){free(p);return false;}
  while(run--){size_t col=decoded%stride;if(col<w)p[(decoded/stride)*w+col]=value;++decoded;}
 }
 // Optional producer padding may precede the trailing palette; never read it
 // as pixels, and never use a serialized pointer or host struct layout.
 memcpy(palette_,d+n-768,768);free(pixels_);pixels_=p;width_=w;height_=h;return true;
}
const uint8_t* PcxImage::rgb(uint32_t x,uint32_t y)const{
 if(!pixels_ || x>=width_ || y>=height_)return nullptr;
 return palette_+3u*pixels_[(size_t)y*width_+x];
}
}
