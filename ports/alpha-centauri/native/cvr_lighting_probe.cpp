/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_lighting.h"
#include <stdio.h>
#include <string.h>
static uint32_t u32(const uint8_t*b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
static uint8_t lattice[alpha::cvr_shade_lattice_size];
int main(int argc,char**argv){
 if(argc!=2&&argc!=3)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint8_t header[8];
 if(fread(header,1,8,f)!=8||memcmp(header,"CVRL",4)||u32(header+4)>4096){fclose(f);return 20;}
 FILE*out=argc==3?fopen(argv[2],"wb"):stdout;if(!out){fclose(f);return 20;}
 bool ok=alpha::build_cvr_shade_lattice(lattice,sizeof(lattice));
 if(ok)ok=fwrite(lattice,1,sizeof(lattice),out)==sizeof(lattice);
 for(uint32_t i=0;ok&&i<u32(header+4);++i){
  uint8_t bytes[12],shades[2048],indices[8];float vector[3];unsigned theta,phi;
  if(fread(bytes,1,12,f)!=12){ok=false;break;}
  for(unsigned j=0;j<3;++j){uint32_t word=u32(bytes+4*j);memcpy(vector+j,&word,4);}
  ok=alpha::cvr_shades_for_light(lattice,sizeof(lattice),vector[0],vector[1],vector[2],shades,sizeof(shades),&theta,&phi);
  if(!ok)fprintf(stderr,"LIGHTING DIRECTION FAIL index=%lu vector=%.9g,%.9g,%.9g\n",(unsigned long)i,double(vector[0]),double(vector[1]),double(vector[2]));
  if(ok){for(unsigned j=0;j<4;++j){indices[j]=uint8_t(theta>>(j*8));indices[j+4]=uint8_t(phi>>(j*8));}
   ok=fwrite(indices,1,8,out)==8&&fwrite(shades,1,2048,out)==2048;}
 }
 if(fgetc(f)!=EOF||ferror(f))ok=false;
 fclose(f);if(out!=stdout&&fclose(out)!=0)ok=false;return ok?0:20;
}
