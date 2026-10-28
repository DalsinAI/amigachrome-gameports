/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_parse_context.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t b[2928];
 while(true){size_t n=fread(b,1,sizeof b,f);if(!n)break;if(n!=sizeof b)return 20;alpha::OriginalParseContext in{};memcpy(in.strings,b+24,2560);size_t at=24+2560;for(unsigned i=0;i<10;++i){in.genders[i]=(int32_t)get(b+at);at+=4;}for(unsigned i=0;i<10;++i){in.plurals[i]=(int32_t)get(b+at);at+=4;}in.default_gender=(int32_t)get(b+at);in.default_plural=(int32_t)get(b+at+4);
 alpha::OriginalParseTextInput q{(alpha::DiscoveryRuntimeObservation)get(b),(int32_t)get(b+4),(int32_t)get(b+8),(int32_t)get(b+12),get(b+20)?b+2672:nullptr,get(b+16)};const auto original=in;const auto oldq=q;alpha::OriginalParseContext out{};memset(&out,0x5a,sizeof out);const auto old=out;
 const auto e=alpha::original_parse_text(in,q,out);if(memcmp(&in,&original,sizeof in) || memcmp(&q,&oldq,sizeof q) || (e!=alpha::OriginalParseError::ok && memcmp(&out,&old,sizeof out)))return 21;
 if(!emit((uint32_t)e))return 20;
 const auto& result=e==alpha::OriginalParseError::ok?out:in;
 if(fwrite(result.strings,1,2560,stdout)!=2560)return 20;
 for(unsigned i=0;i<10;++i)if(!emit((uint32_t)result.genders[i]))return 20;
 for(unsigned i=0;i<10;++i)if(!emit((uint32_t)result.plurals[i]))return 20;
 if(!emit((uint32_t)result.default_gender) || !emit((uint32_t)result.default_plural))return 20;
 const auto held=out;auto alias=[&](alpha::OriginalParseContext& target){return alpha::original_parse_text(in,q,target)==alpha::OriginalParseError::bad_state;};if(!alias(in) || memcmp(&in,&original,sizeof in))return 21;
 auto& requestAlias=*(alpha::OriginalParseContext*)&q;if(!alias(requestAlias) || memcmp(&q,&oldq,sizeof q))return 21;
 if(q.bytes && q.size){auto& textAlias=*(alpha::OriginalParseContext*)(const_cast<uint8_t*>(q.bytes));if(!alias(textAlias))return 21;}
 auto wrap=q;wrap.bytes=(const uint8_t*)(UINTPTR_MAX-3);wrap.size=16;if(alpha::original_parse_text(in,wrap,out)!=alpha::OriginalParseError::bad_state || memcmp(&out,&held,sizeof out))return 21;
 }
 fclose(f);return 0;
}
