/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_commerce.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&out){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;out=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static void word(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static __attribute__((noinline)) CommercePartnerResult*alias_pointer(void*p){return static_cast<CommercePartnerResult*>(p);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t count;if(!read_word(f,count)||count>100000)return 20;for(uint32_t i=0;i<count;++i){uint32_t w[9];for(auto&n:w)if(!read_word(f,n))return 20;CommercePartnerFacts facts{(int32_t)w[0],(int32_t)w[1],(int32_t)w[2],(int32_t)w[3],(int32_t)w[4],w[5]!=0,w[6]!=0,w[7]!=0,w[8]!=0};const auto before=facts;if(commerce_partner(facts,*alias_pointer(&facts))!=CommerceError::bad_buffer||memcmp(&facts,&before,sizeof facts))return 21;CommercePartnerResult out,unchanged;memset(&out,0x5a,sizeof out);memcpy(&unchanged,&out,sizeof out);const auto error=commerce_partner(facts,out);if(error!=CommerceError::ok&&memcmp(&out,&unchanged,sizeof out))return 21;if(memcmp(&facts,&before,sizeof facts))return 21;word((uint32_t)error);word((uint32_t)out.base_value);word((uint32_t)out.import_value);word((uint32_t)out.export_value);}const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;}
