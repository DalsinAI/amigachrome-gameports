/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_flat_context.h"
namespace alpha {namespace {
const OriginalMapTile* at(const OriginalWorld&w,int x,int y){const auto&m=w.game().map.info;if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;return w.tile(((size_t)y*m.width+(unsigned)x)/2);}
}
CvrFlatContext cvr_source_flat_context(const OriginalWorld&w,int x,int y,unsigned zoom){
 if(!w.loaded()||(zoom!=1&&zoom!=2)||x<0||y<0||x>=(int)w.game().map.info.width||y>=(int)w.game().map.info.height||((x+y)&1))return {false,0,0};
 const auto*centre=at(w,x,y);if(!centre)return {false,0,0};const unsigned elevation=centre->climate>>5;if(elevation<3)return {true,4,0};if(elevation>6||(centre->landmarks&0xb03u))return {false,0,0};
 // Four corner neighbourhoods in the original parity-grid coordinate convention.
 const int dx[4][4]={{-2,-1,0,-1},{-1,0,1,0},{0,1,2,1},{-1,0,1,0}};
 const int dy[4][4]={{0,-1,0,1},{-1,-2,-1,0},{0,-1,0,1},{1,0,1,2}};
 unsigned sum=0,relative=0;
 for(unsigned corner=0;corner<4;++corner){unsigned total=0;for(unsigned i=0;i<4;++i){const auto*t=at(w,x+dx[corner][i],y+dy[corner][i]);if(!t||(t->climate>>5)<3||(t->climate>>5)>6||(t->landmarks&0xb03u))return {false,0,0};total+=t->climate>>5;}
  const int code=(int)total-(int)elevation*4+4;if(code<1||code>7||(corner&&relative!=(unsigned)code))return {false,0,0};relative=(unsigned)code;sum=total;
 }
 const int original_zoom=zoom==1?-5:5;const int render_height=(original_zoom+16)*25/16;const int height=((int)sum-12)*render_height/4;
 return {true,(uint8_t)relative,height};
}
}
