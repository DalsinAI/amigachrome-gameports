/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_lighting.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static uint8_t lattice[alpha::cvr_shade_lattice_size];
int main(){
 uint8_t out[2048];unsigned theta=99,phi=99,cases=0;
 if(!alpha::build_cvr_shade_lattice(lattice,sizeof(lattice)))return 20;
 for(unsigned bad=0;bad<13;++bad){
  memset(out,77,sizeof(out));theta=99;phi=99;float x=0,y=0,z=1;
  const uint8_t* input=lattice;uint8_t* dest=out;size_t count=sizeof(lattice),size=sizeof(out);
  switch(bad){case 0:input=nullptr;break;case 1:count=0;break;case 2:dest=nullptr;break;
   case 3:size=2047;break;case 4:x=NAN;break;case 5:y=INFINITY;break;
   case 6:z=-INFINITY;break;case 7:count=51287;break;case 8:count=51289;break;
   case 9:y=NAN;break;case 10:lattice[89]=24;break;case 11:z=NAN;break;case 12:size=2049;break;}
  if(alpha::cvr_shades_for_light(input,count,x,y,z,dest,size,&theta,&phi))return 20;
  for(auto c:out)if(c!=77)return 20;
  if(theta!=99||phi!=99)return 20;
  lattice[89]=22;++cases;
 }
 memset(out,77,sizeof(out));
 if(alpha::build_cvr_shade_lattice(nullptr,sizeof(lattice))||alpha::build_cvr_shade_lattice(out,sizeof(out)))return 20;
 for(auto c:out)if(c!=77)return 20;
 cases+=2;
 printf("CVR LIGHTING CONTROL cases=%u RC 0\n",cases);return 0;
}
