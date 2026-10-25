/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "leader_speech.h"
#include <string.h>
namespace alpha {
static const LeaderSpeech entries[]={
 {"gaians","Lady Deirdre Skye","Gaia's Stepdaughters",false},
 {"hive","Chairman Sheng-Ji Yang","Human Hive",false},
 {"univ","Academician Prokhor Zakharov","University of Planet",false},
 {"morgan","CEO Nwabudike Morgan","Morgan Industries",false},
 {"spartans","Colonel Corazon Santiago","Spartan Federation",false},
 {"believe","Sister Miriam Godwinson","The Lord's Believers",false},
 {"peace","Commissioner Pravin Lal","Peacekeeping Forces",false},
 {"cyborg","Prime Function Aki Zeta-5","Cybernetic Consciousness",true},
 {"pirates","Captain Ulrik Svensgaard","Nautilus Pirates",true},
 {"drone","Foreman Domai","Free Drones",true},
 {"angels","Datajack Sinder Roze","Data Angels",true},
 {"fungboy","Prophet Cha Dawn","Cult of Planet",true},
 {"caretake","Caretaker Lular H'minee","Manifold Caretakers",true},
 {"usurper","Usurper Judaa Marr","Manifold Usurpers",true},
};
const LeaderSpeech* leader_speech(size_t i){return i<14?entries+i:nullptr;}
size_t leader_speech_count(bool expansion){return expansion?14:7;}
int find_leader_speech(const char* s,bool expansion){
 if(!s || !*s)return -1;
 for(size_t i=0;i<leader_speech_count(expansion);++i){
  size_t j=0;for(;s[j]&&entries[i].stem[j];++j){unsigned c=(unsigned char)s[j];if(c>='A'&&c<='Z')c+=32;if(c!=(unsigned char)entries[i].stem[j])break;}
  if(!s[j]&&!entries[i].stem[j])return (int)i;
 }return -1;
}
static uint16_t u16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
static uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
bool decode_speech_wav(const uint8_t* b,size_t n,int16_t* out,size_t cap,size_t& count){
 if(!b||n<12||n>16*1024*1024||memcmp(b,"RIFF",4)||memcmp(b+8,"WAVE",4)||u32(b+4)!=n-8)return false;
 const uint8_t* data=nullptr;size_t bytes=0;bool fmt=false;
 for(size_t pos=12;pos<n;){if(n-pos<8)return false;const uint8_t* h=b+pos;size_t len=u32(h+4);pos+=8;if(len>n-pos)return false;
  if(!memcmp(h,"fmt ",4)){if(fmt||len!=16||u16(b+pos)!=1||u16(b+pos+2)!=1||u32(b+pos+4)!=22050||u32(b+pos+8)!=44100||u16(b+pos+12)!=2||u16(b+pos+14)!=16)return false;fmt=true;}
  else if(!memcmp(h,"data",4)){if(data||!len||(len&1))return false;data=b+pos;bytes=len;}
  pos+=len;if(len&1){if(pos==n)return false;++pos;}
 }
 if(!fmt||!data||!out||bytes/2>cap)return false;
 for(size_t i=0;i<bytes/2;++i){uint16_t v=u16(data+2*i);out[i]=v<32768?(int16_t)v:(int16_t)((int32_t)v-65536);}
 count=bytes/2;return true;
}
}
