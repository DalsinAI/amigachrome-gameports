/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "completion_tail.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char b[36];size_t count=0;
 for(;;){size_t n=fread(b,1,36,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=36 || ++count>200000)return 20;uint32_t w[9];for(unsigned i=0;i<9;++i)w[i]=word(b+4*i);alpha::CompletionTailResult r{0x31415926,0x31415926,0x31415926,true,true};auto e=alpha::LogicError::bad_state;
 if(w[0]<=1 && w[7]<=1 && w[8]<=1){alpha::CompletionTailInput in{w[0]?alpha::Variant::alien_crossfire:alpha::Variant::base,s(w[1]),s(w[2]),s(w[3]),s(w[4]),w[5],w[6],w[7]!=0,w[8]!=0};e=alpha::completion_tail(in,r);}
 if(e!=alpha::LogicError::ok && (r.clean_minerals!=0x31415926 || r.current_item!=0x31415926 || r.player_flags!=0x31415926 || !r.base_reset_required || !r.stockpile_fallback))return 21;
 if(!emit((uint32_t)e) || !emit((uint32_t)r.clean_minerals) || !emit((uint32_t)r.current_item) || !emit(r.player_flags) || !emit(r.base_reset_required) || !emit(r.stockpile_fallback))return 20;
 }
}
