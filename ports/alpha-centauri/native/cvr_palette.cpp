/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent implementation from verified arithmetic/data-flow observations.
 * No original executable instructions, palettes or shade tables are included.
 */
#include "cvr_palette.h"
#include <string.h>
namespace alpha { namespace {
bool overlap(const void* a,size_t n,const void* b,size_t m){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(n>UINTPTR_MAX-x || m>UINTPTR_MAX-y)return true;
 return x<y+m && y<x+n;
}
}
bool build_cvr_default_global_palette(const uint8_t* input,size_t input_size,uint8_t* output,size_t capacity){
 if(!input || input_size!=768 || !output || capacity<768 || overlap(input,768,output,768))return false;
 uint8_t palette[768]{};
 // Common indexed Windows colours, computed from channel bits rather than
 // embedding executable palette tables. Four reserved red slots are explicit.
 for(unsigned i=0;i<8;++i)for(unsigned c=0;c<3;++c){
  palette[i*3+c]=i==7?192:((i&(1u<<c))?128:0);
  palette[(248+i)*3+c]=i==0?128:((i&(1u<<c))?255:0);
 }
 palette[8*3]=8;palette[9*3]=9;palette[246*3]=245;palette[247*3]=246;
 for(unsigned i=0;i<236*3;++i)palette[30+i]=input[i]>235?235:input[i];
 memcpy(output,palette,sizeof(palette));return true;
}
bool build_cvr_faction_palette(const uint8_t* palette,size_t palette_size,
 const uint8_t* shades,size_t shade_size,const uint8_t* tint,size_t tint_size,
 uint8_t* output,size_t capacity){
 if(!palette || palette_size!=768 || !shades || shade_size!=6144 || !tint || tint_size!=3 || !output || capacity<6144 || overlap(output,6144,palette,768) || overlap(output,6144,shades,6144) || overlap(output,6144,tint,3))return false;
 uint8_t remap[256];
 for(unsigned source=0;source<256;++source){
  unsigned target[3];for(unsigned c=0;c<3;++c)target[c]=(unsigned)palette[source*3+c]*tint[c]/255;
  unsigned best=0,best_distance=200000;
  for(unsigned candidate=0;candidate<256;++candidate){
   unsigned distance=0;
   for(unsigned c=0;c<3;++c){const int d=(int)palette[candidate*3+c]-(int)target[c];distance+=(unsigned)(d*d);}
   if(distance<best_distance){best=candidate;best_distance=distance;}
  }
  remap[source]=(uint8_t)best;
 }
 for(size_t i=0;i<6144;++i)output[i]=remap[shades[i]];
 return true;
}
}
