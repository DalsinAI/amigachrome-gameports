/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "monument_presentation.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t r;memcpy(&r,&n,4);return r;}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static bool guards(){
 struct Storage {alpha::MonumentPresentationResult result;uint8_t tail[alpha::monument_record_bytes];} b{};
 alpha::MonumentPresentationInput in{};in.variant=alpha::Variant::base;in.size=alpha::monument_record_bytes;
 for(unsigned i=0;i<3;++i){b.result={0x12345678,true,true};in.local_record=i==0?nullptr:i==1?(uint8_t*)&b:(uint8_t*)&b+sizeof b.result-1;
 if(alpha::monument_presentation(in,b.result)!=alpha::LogicError::bad_state || b.result.achieved_count!=0x12345678 || !b.result.count_written || !b.result.initialization_required)return false;}
 return true;
}
int main(int argc,char** argv){if(argc!=2)return 20;if(!guards())return 21;FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t b[28+alpha::monument_record_bytes];unsigned count=0;
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>100000)return 20;
 alpha::MonumentPresentationInput in{get(b)?alpha::Variant::alien_crossfire:alpha::Variant::base,b+28,get(b+24),get(b+4),get(b+8),get(b+12),s(get(b+16)),get(b+20)!=0};
 alpha::MonumentPresentationResult r{0x12345678,true,true};auto err=get(b)>1 || get(b+20)>1?alpha::LogicError::bad_state:alpha::monument_presentation(in,r);
 if(err!=alpha::LogicError::ok && (r.achieved_count!=0x12345678 || !r.count_written || !r.initialization_required))return 21;
 if(!emit((uint32_t)err) || !emit(r.achieved_count) || !emit(r.count_written) || !emit(r.initialization_required))return 20;
 }
}
