/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_notice.h"
#include <stdio.h>
#include <string.h>
#include <initializer_list>
#define REQUIRE(x) do {if(!(x))return 21;} while(0)
int main(){
 unsigned cases=0;
 for(unsigned v=0;v<2;++v)for(int fac=2;fac<=(v?41:33);++fac)for(int f=0;f<8;++f){
  const auto variant=v?alpha::Variant::alien_crossfire:alpha::Variant::base;
  for(unsigned flags=0;flags<16;++flags){
   alpha::FacilityNoticeEntryInput entry{variant,fac,f,f,flags&1u,(flags>>1)&1u,{}};
   if(flags&4u)entry.announced[fac/8]=(unsigned char)(1u<<(fac%8));
   if(flags&8u)entry.local_faction=(f+1)%8;
   alpha::FacilityNoticeRoute route=alpha::FacilityNoticeRoute::none;
   REQUIRE(alpha::facility_notice_entry(entry,route)==alpha::LogicError::ok);
   const auto expected=v && fac==40?alpha::FacilityNoticeRoute::subspace_special:
    (flags&8u)?(fac>=(v?40:33)?alpha::FacilityNoticeRoute::foreign:alpha::FacilityNoticeRoute::none):
    (!(flags&1u) && (flags&2u) && !(flags&4u))?alpha::FacilityNoticeRoute::local_first:alpha::FacilityNoticeRoute::local_regular;
   REQUIRE(route==expected);++cases;
  }
  if(v && fac==40)continue;
  for(unsigned warning=0;warning<4;++warning){
   alpha::FacilityNoticeConstructionInput in{variant,fac,warning,{}};
   // Current construction-stage bytes include changes absent from entry.
   for(unsigned i=0;i<12;++i)in.announced[i]=(unsigned char)(i*17+f);
   unsigned char expected[12];memcpy(expected,in.announced,12);
   if(warning&1u)expected[fac/8]|=(unsigned char)(1u<<(fac%8));
   alpha::FacilityNoticeConstructionResult result{};
   REQUIRE(alpha::facility_notice_construction(in,result)==alpha::LogicError::ok);
   REQUIRE(result.blurb_requested==bool(warning&1u));REQUIRE(!memcmp(expected,result.announced,12));++cases;
  }
 }
 const unsigned prefs[]={0,0x4000,0x20000,0x24000,0xffffffff};
 for(int f=0;f<8;++f)for(unsigned mask=0;mask<256;++mask)for(auto before:prefs)for(auto after:prefs){
  bool requested=false;
  REQUIRE(alpha::facility_notice_voice({f,before,mask},requested)==alpha::LogicError::ok);
  REQUIRE(requested==bool((before&0x4000) && (mask&(1u<<f))));
  REQUIRE(alpha::facility_notice_fade(after)==((after&0x24000)==0x24000));++cases;
 }
 for(int f:{-1,8}){bool result=true;REQUIRE(alpha::facility_notice_voice({f,0,0},result)==alpha::LogicError::bad_state);REQUIRE(result);++cases;}
 for(unsigned v=0;v<3;++v)for(int fac:{-1,1,2,33,34,40,41,42}){
  const auto variant=v==0?alpha::Variant::base:v==1?alpha::Variant::alien_crossfire:static_cast<alpha::Variant>(999);
  alpha::FacilityNoticeConstructionInput in{variant,fac,1,{}};
  alpha::FacilityNoticeConstructionResult r{};memset(r.announced,90,12);r.blurb_requested=true;
  bool valid=v<2 && fac>=2 && fac<=(v?41:33) && !(v==1 && fac==40);
  REQUIRE((alpha::facility_notice_construction(in,r)==alpha::LogicError::ok)==valid);
  if(!valid){for(auto b:r.announced)REQUIRE(b==90);REQUIRE(r.blurb_requested);}++cases;
 }
 printf("%u stage controls passed\n",cases);return 0;
}
