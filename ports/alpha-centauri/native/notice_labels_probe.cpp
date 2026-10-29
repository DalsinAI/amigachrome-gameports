/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "notice_labels.h"
#include <stdio.h>
#include <string.h>
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t r;memcpy(&r,&n,4);return r;}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t b[36];unsigned count=0;
 for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>200000)return 20;
 alpha::NoticeLabelsInput in{get(b)?alpha::Variant::alien_crossfire:alpha::Variant::base,s(get(b+4)),s(get(b+16)),s(get(b+32)),get(b+20),get(b+24),get(b+28),get(b+8)!=0,get(b+12)!=0};
 alpha::NoticeLabelsResult r{};memset(r.label,90,10);r.warning_mask=0x12345678;r.queue_count_slot=0x12345678;r.icon=alpha::FacilityNoticeIcon::alien;r.queue_count_written=true;
 auto err=get(b)>1 || get(b+8)>1 || get(b+12)>1?alpha::LogicError::bad_state:alpha::notice_labels(in,r);
 if(err!=alpha::LogicError::ok){for(char c:r.label)if(c!=90)return 21;if(r.warning_mask!=0x12345678 || r.queue_count_slot!=0x12345678 || r.icon!=alpha::FacilityNoticeIcon::alien || !r.queue_count_written)return 21;}
 if(!emit((uint32_t)err) || !emit(r.warning_mask) || !emit((uint32_t)r.queue_count_slot) || !emit((uint32_t)r.icon) || !emit(r.queue_count_written) || fwrite(r.label,1,10,stdout)!=10)return 20;
 }
}
