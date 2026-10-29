/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_availability.h"
#include <stdio.h>
#include <string.h>
static int32_t load(const unsigned char* b){uint32_t n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
static alpha::FacilityDefinitions definitions;
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char b[256];size_t count=0;
 for(;;){const size_t n=fread(b,1,sizeof b,f);if(!n){const bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>500000){fclose(f);return 20;}
  alpha::FacilityAvailabilityContext c{};bool available=true;alpha::LogicError error=alpha::LogicError::bad_state;
  const int32_t profile=load(b),id=load(b+40);
  if((profile==0 || profile==1) && load(b+4)>=0 && load(b+4)<=1 && load(b+8)>=0 && load(b+8)<=1 && load(b+12)>=0 && load(b+12)<=1 && load(b+16)>=0 && load(b+16)<=1){
   c.profile=profile?alpha::Variant::alien_crossfire:alpha::Variant::base;c.expansion_enabled=load(b+4)!=0;c.caretaker=load(b+8)!=0;c.has_base=load(b+12)!=0;c.coastal=load(b+16)!=0;
   c.faction=load(b+20);c.difficulty=load(b+24);c.queue_count=load(b+28);c.game_rules=(uint32_t)load(b+32);c.faction_rule_flags=(uint32_t)load(b+36);
   memcpy(c.facilities,b+48,12);memcpy(c.known_technology,b+60,89);memcpy(c.project_owner,b+149,64);for(unsigned i=0;i<10;++i)c.queue[i]=load(b+213+i*4);
   definitions.slot_count=profile?134:72;definitions.first_repeatable=profile?65:34;definitions.first_project=profile?70:39;definitions.project_count=profile?64:33;
   if(id>=0 && id<134)definitions.slots[id].prerequisite=load(b+44);
   if(b[253]==1)++definitions.slot_count;else if(b[253]==2)++definitions.first_repeatable;else if(b[253]==3)++definitions.first_project;else if(b[253]==4)++definitions.project_count;
   error=alpha::facility_available(definitions,c,(size_t)id,available);
  }
  if(error!=alpha::LogicError::ok && !available){fclose(f);return 21;}
  if(!emit((int32_t)error) || !emit(available?1:0)){fclose(f);return 20;}
 }
}
