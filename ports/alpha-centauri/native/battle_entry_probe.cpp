/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "battle_entry.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
using namespace alpha;
static bool get(uint32_t&v){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return true;}
static bool put(uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};return fwrite(b,1,4,stdout)==4;}
static int32_t si(uint32_t v){return v&0x80000000u?-(int32_t)(~v)-1:(int32_t)v;}
int main(){
 uint32_t type;
 while(get(type)){
  uint32_t x[16];for(unsigned i=0;i<16;++i)if(!get(x[i]))return 2;
  if(type==0){
   BattleChargeFacts f{(BattleEntryObservation)x[0],(Variant)x[1],si(x[2]),si(x[3]),x[4],x[5]};
   BattleChargeResult r{12345,6789};const auto e=inspect_battle_charge(f,r);
   if(!put((uint32_t)e)||!put((uint32_t)r.charge)||!put(r.speed_used))return 3;
  }else if(type==1){
   BattleHastyFacts f{(BattleEntryObservation)x[0],(Variant)x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8],x[9],si(x[10]),si(x[11]),si(x[12]),si(x[13]),si(x[14])};
   BattleHastyResult r;memset(&r,0x5a,sizeof r);const auto e=inspect_battle_hasty(f,r);
   if(!put((uint32_t)e)||!put(r.attacker_state)||!put(r.defender_state)||!put(r.moves_spent)||!put(r.scaled)||!put((uint32_t)r.offense)||!put((uint32_t)r.penalty)||!put((uint32_t)r.report_count)||!put(r.string_lookup_required)||!put(r.report_append_required))return 3;
  }else if(type==2){
   // Known overlap is rejected before an invalid observation can be read.
   auto*c=reinterpret_cast<BattleChargeFacts*>(uintptr_t(4096));
   auto*cr=reinterpret_cast<BattleChargeResult*>(uintptr_t(4096));
   auto*h=reinterpret_cast<BattleHastyFacts*>(uintptr_t(4096));
   auto*hr=reinterpret_cast<BattleHastyResult*>(uintptr_t(4096));
   if(!put((uint32_t)inspect_battle_charge(*c,*cr))||!put((uint32_t)inspect_battle_hasty(*h,*hr)))return 3;
  }else return 2;
 }
 return ferror(stdin)||fflush(stdout)?4:0;
}
