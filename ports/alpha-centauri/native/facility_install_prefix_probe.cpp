/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_install_prefix.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t signed_word(const unsigned char* p){uint32_t n=word(p);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char record[44];size_t count=0;
 for(;;){size_t n=fread(record,1,sizeof record,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof record || ++count>200000){fclose(f);return 20;}
 alpha::FacilityInstallPrefixResult r{};r.state_flags=0x31415926;r.minerals_accumulated=r.minerals_accumulated_2=0x31415926;r.network_node_tail_required=true;memset(r.facilities,0x5a,12);alpha::LogicError error=alpha::LogicError::bad_state;int32_t profile=signed_word(record),human=signed_word(record+24);
 if((profile==0 || profile==1) && (human==0 || human==1)){alpha::FacilityInstallPrefixInput in{profile?alpha::Variant::alien_crossfire:alpha::Variant::base,signed_word(record+4),signed_word(record+8),signed_word(record+12),signed_word(record+16),signed_word(record+20),human!=0,word(record+28),{}};memcpy(in.facilities,record+32,12);error=alpha::facility_install_prefix(in,r);}
 if(error!=alpha::LogicError::ok){if(r.state_flags!=0x31415926 || r.minerals_accumulated!=0x31415926 || r.minerals_accumulated_2!=0x31415926 || !r.network_node_tail_required){fclose(f);return 21;}for(uint8_t b:r.facilities)if(b!=0x5a){fclose(f);return 21;}}
 if(!emit((uint32_t)error) || !emit(r.state_flags) || !emit((uint32_t)r.minerals_accumulated) || !emit((uint32_t)r.minerals_accumulated_2)){fclose(f);return 20;}
 for(unsigned i=0;i<3;++i)if(!emit(word(r.facilities+i*4))){fclose(f);return 20;}
 if(!emit(r.network_node_tail_required)){fclose(f);return 20;}
 }
}
