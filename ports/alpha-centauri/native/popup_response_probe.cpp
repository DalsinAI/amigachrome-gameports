/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "popup_response.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t r;memcpy(&r,&n,4);return r;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t b[36];unsigned count=0;
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>200000)return 20;
 alpha::PopupResponseInput in{get(b)?alpha::Variant::alien_crossfire:alpha::Variant::base,static_cast<alpha::PopupResponseRuntime>(get(b+4)),s(get(b+8)),s(get(b+12)),get(b+16),get(b+20),get(b+24),get(b+28),get(b+32)!=0};
 alpha::PopupResponseResult r{0x12345678,0x12345678,0x12345678,0x12345678,true,true};auto err=get(b)>1 || get(b+32)>1?alpha::LogicError::bad_state:alpha::popup_response(in,r);
 if(err!=alpha::LogicError::ok && (r.base_upkeep_flag!=0x12345678 || r.warnings!=0x12345678 || r.warnings_mirror!=0x12345678 || r.governor_flags!=0x12345678 || !r.staple_required || !r.preferences_save_required))return 21;
 if(!emit((uint32_t)err) || !emit((uint32_t)r.base_upkeep_flag) || !emit(r.warnings) || !emit(r.warnings_mirror) || !emit(r.governor_flags) || !emit(r.staple_required) || !emit(r.preferences_save_required))return 20;
 }
}
