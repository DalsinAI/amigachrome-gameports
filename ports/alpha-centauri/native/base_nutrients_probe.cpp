/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_nutrients.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t signed_word(uint32_t n){return n<=2147483647u?(int32_t)n:-1-(int32_t)(UINT32_MAX-n);}
int main(){unsigned char bytes[40];while(true){const size_t n=fread(bytes,1,sizeof bytes,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof bytes)return 20;uint32_t words[10];for(unsigned i=0;i<10;++i){words[i]=0;for(unsigned j=0;j<4;++j)words[i]|=(uint32_t)bytes[4*i+j]<<(8*j);}
 NutrientFacts f{signed_word(words[0]),signed_word(words[1]),signed_word(words[2]),signed_word(words[3]),signed_word(words[4]),signed_word(words[5]),signed_word(words[6]),signed_word(words[7]),words[8],words[9]};const auto original=f;
 NutrientResult result;memset(&result,0x5a,sizeof result);const auto old=result;const auto error=base_nutrients(f,result);if(memcmp(&f,&original,sizeof f)||(error!=NutrientError::ok&&memcmp(&result,&old,sizeof result)))return 21;
 alignas(NutrientFacts) unsigned char aliases[128];memset(aliases,0x77,sizeof aliases);memcpy(aliases+32,&f,sizeof f);unsigned char before[sizeof aliases];memcpy(before,aliases,sizeof aliases);
 const unsigned offsets[]={12u,32u,36u,68u};for(unsigned offset:offsets){auto&input=*reinterpret_cast<NutrientFacts*>(aliases+32);auto&output=*reinterpret_cast<NutrientResult*>(aliases+offset);if(base_nutrients(input,output)!=NutrientError::bad_buffer||memcmp(before,aliases,sizeof aliases))return 21;}
 struct Adjacent{NutrientFacts input;NutrientResult output;} adjacent{f,{}};memset(&adjacent.output,0x5a,sizeof adjacent.output);if(base_nutrients(adjacent.input,adjacent.output)!=error||memcmp(&adjacent.input,&f,sizeof f)||memcmp(&adjacent.output,&result,sizeof result))return 21;
 const uint32_t out[]={(uint32_t)error,(uint32_t)result.growth,(uint32_t)result.intake,(uint32_t)result.consumption,(uint32_t)result.surplus,(uint32_t)result.stored,(uint32_t)result.faction_surplus};for(uint32_t word:out){unsigned char b[4];for(unsigned j=0;j<4;++j)b[j]=(unsigned char)(word>>(8*j));if(fwrite(b,1,4,stdout)!=4)return 20;}
 }}
