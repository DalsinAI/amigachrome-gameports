/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_palette.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc!=3)return 20;
 struct Memory{uint8_t* data;uint8_t* output;Memory():data((uint8_t*)malloc(6923)),output((uint8_t*)malloc(6144)){}~Memory(){free(data);free(output);}}memory;
 if(!memory.data || !memory.output)return 20;
 auto*data=memory.data;auto*output=memory.output;FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 const bool read=fread(data,1,6923,f)==6923 && fgetc(f)==EOF;fclose(f);
 if(!read || memcmp(data,"ACPA\1\0\0\0",8))return 20;
 if(!alpha::build_cvr_faction_palette(data+8,768,data+776,6144,data+6920,3,output,6144))return 20;
 f=fopen(argv[2],"wb");if(!f)return 20;const bool written=fwrite(output,1,6144,f)==6144;fclose(f);if(!written)return 20;
 // Invalid inputs must leave the destination unchanged.
 memset(output,47,6144);
 struct Invalid{const uint8_t*p;size_t pn;const uint8_t*s;size_t sn;const uint8_t*t;size_t tn;uint8_t*out;size_t cap;};
 const Invalid cases[]={
 {nullptr,768,data+776,6144,data+6920,3,output,6144},
 {data+8,767,data+776,6144,data+6920,3,output,6144},
 {data+8,769,data+776,6144,data+6920,3,output,6144},
 {data+8,768,nullptr,6144,data+6920,3,output,6144},
 {data+8,768,data+776,6143,data+6920,3,output,6144},
 {data+8,768,data+776,6145,data+6920,3,output,6144},
 {data+8,768,data+776,6144,nullptr,3,output,6144},
 {data+8,768,data+776,6144,data+6920,2,output,6144},
 {data+8,768,data+776,6144,data+6920,4,output,6144},
 {data+8,768,data+776,6144,data+6920,3,nullptr,6144},
 {data+8,768,data+776,6144,data+6920,3,output,6143},
 {output,768,data+776,6144,data+6920,3,output,6144},
 {data+8,768,output,6144,data+6920,3,output,6144},
 {data+8,768,data+776,6144,output,3,output,6144}};
 for(const auto&v:cases){if(alpha::build_cvr_faction_palette(v.p,v.pn,v.s,v.sn,v.t,v.tn,v.out,v.cap))return 20;for(size_t j=0;j<6144;++j)if(output[j]!=47)return 20;}
 puts("FACTION PALETTE table6144 invalid14 RC0");return 0;
}
