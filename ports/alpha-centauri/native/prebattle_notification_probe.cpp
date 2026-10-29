/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_notification.h"
#include <cstdio>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
template<class T>static T&at(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){uint32_t w[75];while(get(w[0])){
 for(unsigned i=1;i<75;++i)if(!get(w[i]))return 2;
 PrebattleNotificationFacts f{(BattleEntryObservation)w[0],(Variant)w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8],w[9],w[10],{}};
 for(unsigned i=0;i<64;++i)f.status[i]=w[11+i];
 const auto before=f;PrebattleNotificationProposal out;memset(&out,0xa5,sizeof out);const auto prior=out;
 const auto e=inspect_prebattle_notification(f,out);
 if(e!=PrebattleNotificationError::ok&&memcmp(&out,&prior,sizeof out))return 3;
 if(memcmp(&f,&before,sizeof f))return 4;
 put((uint32_t)e);put((uint32_t)out.route);put(out.warning);put(out.spoke);
 for(unsigned offset=0;offset+sizeof out<=sizeof f;offset+=4){
  if(inspect_prebattle_notification(f,at<PrebattleNotificationProposal>((uintptr_t)&f+offset))!=PrebattleNotificationError::bad_state||memcmp(&f,&before,sizeof f))return 5;
 }
 if(inspect_prebattle_notification(at<PrebattleNotificationFacts>(4096),at<PrebattleNotificationProposal>(4096))!=PrebattleNotificationError::bad_state)return 6;
 }return ferror(stdin)||ferror(stdout)?7:0;}
