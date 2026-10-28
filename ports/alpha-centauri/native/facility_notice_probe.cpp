/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_notice.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t r;memcpy(&r,&n,4);return r;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t b[52];unsigned count=0;
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>200000)return 20;
 alpha::FacilityNoticeInput in{};in.variant=get(b)?alpha::Variant::alien_crossfire:alpha::Variant::base;in.facility=s(get(b+4));in.faction=s(get(b+8));in.local_faction=s(get(b+12));in.game_state=get(b+16);in.warnings_entry=get(b+20);in.warnings_after_construction=get(b+24);in.preferences_before_voice=get(b+28);in.preferences_after_popup=get(b+32);in.human_mask=get(b+36);memcpy(in.announced,b+40,12);
 alpha::FacilityNoticeResult r{};r.route=alpha::FacilityNoticeRoute::subspace_special;memset(r.announced,90,12);r.blurb_requested=r.voice_load_requested=r.voice_fade_requested=true;
 auto err=get(b)>1?alpha::LogicError::bad_state:alpha::facility_notice(in,r);
 if(err!=alpha::LogicError::ok){for(auto c:r.announced)if(c!=90)return 21;if(r.route!=alpha::FacilityNoticeRoute::subspace_special || !r.blurb_requested || !r.voice_load_requested || !r.voice_fade_requested)return 21;}
 if(!emit((uint32_t)err) || !emit((uint32_t)r.route) || !emit(r.blurb_requested) || !emit(r.voice_load_requested) || !emit(r.voice_fade_requested) || fwrite(r.announced,1,12,stdout)!=12)return 20;
 }
}
