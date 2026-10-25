/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_fixed_transform.h"
#include <stdio.h>
#include <string.h>
static void word(uint32_t n,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)putchar((int)((n>>(8*i))&255));}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;unsigned cases=0;while(true){unsigned char bytes[36];const size_t size=fread(bytes,1,36,f);if(!size)break;if(size!=36||++cases>4096){fclose(f);return 20;}float matrix[9];for(unsigned i=0;i<9;++i){const uint32_t n=(uint32_t)bytes[i*4]|(uint32_t)bytes[i*4+1]<<8|(uint32_t)bytes[i*4+2]<<16|(uint32_t)bytes[i*4+3]<<24;memcpy(matrix+i,&n,4);}alpha::CvrFixedBasis b{};alpha::CvrFixedSeed s{};if(!alpha::prepare_cvr_fixed_basis(matrix,b)){fclose(f);return 20;}for(auto n:b.xy)word((uint16_t)n,2);for(auto n:b.z)word((uint32_t)n);
 for(unsigned d=0;d<26;++d){const unsigned q=d+(d>=13);const int16_t p[]={(int16_t)((int)(q/9)-1),(int16_t)((int)((q/3)%3)-1),(int16_t)((int)(q%3)-1)};int32_t x,y;uint32_t z;alpha::cvr_fixed_position(b,s,p,x,y,z);word((uint32_t)x);word((uint32_t)y);word(z);word(0);}
 }const bool ok=cases&&!ferror(f)&&!ferror(stdout);fclose(f);return ok?0:20;}
