/* Compare worker output to a scalar byte model, with padded pitches and sentinels. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amigaopenup.h"
int main(void){unsigned w,h,y,x,c;unsigned seed=7;UBYTE table[1024];
 for(c=0;c<1024;c++)table[c]=(c*73)^0xa5;
 for(w=1;w<1030;w+=17)for(h=1;h<=33;h+=8){
  unsigned ip=w+7,op=(w*4+31)&~15u;UBYTE *in=malloc(ip*h),*out=malloc(op*h+16);
  for(x=0;x<ip*h;x++){seed=seed*1664525u+1013904223u;in[x]=seed>>24;}
  memset(out,0xcd,op*h+16);
  for(y=0;y<h;y++)assert(!GFX2_ExpandRow(y,in,out,table,w,ip,op));
  for(y=0;y<h;y++){for(x=0;x<w;x++)assert(!memcmp(out+y*op+x*4,table+in[y*ip+x]*4,4));
   for(x=w*4;x<op;x++)assert(out[y*op+x]==0xcd);}
  for(x=0;x<16;x++)assert(out[op*h+x]==0xcd);free(in);free(out);
 }
 puts("palette worker: padded pitches, transparent entries, row isolation and bounds PASS");return 0;}
