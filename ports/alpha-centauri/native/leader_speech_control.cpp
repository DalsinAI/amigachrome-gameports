/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "leader_speech.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static bool check(bool good){++checks;if(!good)printf("FAIL %u\n",checks);return good;}
int main(){bool good=true;
 good&=check(alpha::leader_speech_count(false)==7&&alpha::leader_speech_count(true)==14);
 for(size_t i=0;i<14;++i){auto e=alpha::leader_speech(i);good&=check(e&&alpha::find_leader_speech(e->stem,true)==(int)i);good&=check(alpha::find_leader_speech(e->stem,false)==(i<7?(int)i:-1));}
 good&=check(!alpha::leader_speech(14));good&=check(alpha::find_leader_speech("GAIANS",false)==0);
 const char* bad[]={"","gaiant","GAIANX","gaians.mp3","../gaians","voices/gaians","gaians/","gaians\\",nullptr};
 for(auto b:bad)good&=check(alpha::find_leader_speech(b,true)==-1);
 uint8_t wav[]={82,73,70,70,44,0,0,0,87,65,86,69,102,109,116,32,16,0,0,0,1,0,1,0,34,86,0,0,68,172,0,0,2,0,16,0,100,97,116,97,8,0,0,0,0,0,255,127,0,128,255,255};
 int16_t out[4]={};size_t n=999;good&=check(alpha::decode_speech_wav(wav,sizeof(wav),out,4,n)&&n==4&&out[0]==0&&out[1]==32767&&out[2]==-32768&&out[3]==-1);
 for(size_t size=0;size<sizeof(wav);++size){n=999;out[0]=123;good&=check(!alpha::decode_speech_wav(wav,size,out,4,n)&&n==999&&out[0]==123);}
 unsigned positions[]={0,4,8,16,20,22,24,28,32,34,40};
 for(auto pos:positions){uint8_t copy[sizeof(wav)];memcpy(copy,wav,sizeof(wav));copy[pos]^=1;n=999;out[0]=123;good&=check(!alpha::decode_speech_wav(copy,sizeof(copy),out,4,n)&&n==999&&out[0]==123);}
 n=999;good&=check(!alpha::decode_speech_wav(wav,sizeof(wav),out,3,n)&&n==999);
 printf("LEADER SPEECH %u checks %s\n",checks,good?"PASS":"FAIL");return good?0:20;
}
