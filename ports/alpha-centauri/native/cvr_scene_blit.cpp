/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_scene_blit.h"
#include "pcx_image.h"
#include <limits.h>
#include <string.h>
namespace alpha {namespace {
bool overlaps(const void*a,size_t as,const void*b,size_t bs){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x>UINTPTR_MAX-as||y>UINTPTR_MAX-bs||(x<y+bs&&y<x+as);}
}
bool composite_cvr_scene_1x(const uint8_t* source,size_t source_size,const uint8_t* rgb,size_t rgb_size,int32_t cx,int32_t cy,const CvrSceneRect&clip,CvrSceneRgbTarget target){
 if(!source||source_size!=65536||!rgb||rgb_size!=768||!target.pixels||!target.width||!target.height||target.width>4096||target.height>4096||target.row_bytes<target.width*3||target.row_bytes>4096*3||target.capacity<(size_t)target.row_bytes*target.height||clip.left<0||clip.top<0||clip.right<clip.left||clip.bottom<clip.top||clip.right>(int32_t)target.width||clip.bottom>(int32_t)target.height)return false;
 const size_t bytes=(size_t)target.row_bytes*target.height;if(overlaps(source,65536,target.pixels,bytes)||overlaps(rgb,768,target.pixels,bytes))return false;
 const int64_t left=(int64_t)cx-128,top=(int64_t)cy-128;for(unsigned y=0;y<256;++y){const int64_t dy=top+y;if(dy<clip.top||dy>=clip.bottom)continue;for(unsigned x=0;x<256;++x){const int64_t dx=left+x;if(dx<clip.left||dx>=clip.right)continue;const uint8_t index=source[y*256+x];if(index!=9)memcpy(target.pixels+(size_t)dy*target.row_bytes+(size_t)dx*3,rgb+(size_t)index*3,3);}}
 return true;
}
bool cvr_basic_unit_marker(const PcxImage&image,unsigned chassis,int32_t&x,int32_t&y){if(chassis>1||image.width()!=1024||image.height()!=768||!image.pixels())return false;const auto*p=image.pixels();const unsigned row=2+77*chassis;int xx=-1,yy=-1;for(unsigned i=0;i<100;++i)for(unsigned j=row-1;j<=row;++j)if(((p[j*1024+918+i]+10u)&255)==5)xx=(int)i;for(unsigned i=0;i<75;++i)for(unsigned j=916;j<918;++j)if(((p[(row+1+i)*1024+j]+10u)&255)==5)yy=(int)i;if(xx<0||yy<0)return false;x=xx;y=yy;return true;}
bool cvr_unit_anchor_zero_zoom(int32_t x,int32_t y,int32_t mx,int32_t my,int32_t&cx,int32_t&cy){if(mx<0||mx>=100||my<0||my>=75)return false;const int64_t xx=(int64_t)x+mx+50,yy=(int64_t)y+my+50;if(xx<INT32_MIN||xx>INT32_MAX||yy<INT32_MIN||yy>INT32_MAX)return false;cx=(int32_t)xx;cy=(int32_t)yy;return true;}
}
