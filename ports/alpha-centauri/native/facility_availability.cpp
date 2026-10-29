/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in facility_availability.h. */
#include "facility_availability.h"
namespace alpha {
namespace {
bool project(const FacilityAvailabilityContext& c,int id){return id<(c.profile==Variant::base?33:64) && c.project_owner[id]==c.faction;}
bool has(const FacilityAvailabilityContext& c,int id){
 const int repeat=c.profile==Variant::base?34:65;if(id>=repeat)return false;
 if(c.facilities[(unsigned)id/8]&(1u<<((unsigned)id%8)))return true;
 for(int i=0;i<c.queue_count;++i)if(c.queue[i]==-id)return true;
 return false;
}
bool calculate(const FacilityDefinitions& d,const FacilityAvailabilityContext& c,int id){
 const int first=(int)d.first_project;
 if(id==0 || (id==10 && c.difficulty<2) || (id>=first && (c.game_rules&0x80000000u)))return false;
 // Original special Ascent branch precedes technology, ownership and Caretaker
 // tests, including the original destroyed-Voice behavior. Do not import fixes.
 if(id==first+32)return c.project_owner[31]!=-1 && (c.game_rules&0x800u);
 const int prerequisite=d.slots[id].prerequisite;
 if(prerequisite!=-1 && (prerequisite<0 || prerequisite>=88 || !c.known_technology[prerequisite]))return false;
 if(c.profile==Variant::alien_crossfire){
  if(!c.expansion_enabled && id>=103 && id<=106)return false;
  if(c.caretaker && id==101)return false;
 }
 if(id>=first)return c.project_owner[id-first]==-1;
 if(!c.has_base)return true;
 if(has(c,id))return false;
 if(id==28){if(project(c,10) || !c.coastal)return false;}
 else {
  int redundant=-1;
  if(id==4)redundant=5;else if(id==27)redundant=1;else if(id==30)redundant=17;
  else if(id==21)redundant=28;
  else if(c.profile==Variant::alien_crossfire && id==29)redundant=35;
  else if(c.profile==Variant::alien_crossfire && id==7)redundant=36;
  if(redundant>=0 && project(c,redundant))return false;
 }
 if(c.profile==Variant::alien_crossfire){
  if(!c.expansion_enabled && id>=34 && id<=41)return false;
  if(id>=36 && id<=38 && !c.coastal)return false;
  if(id==40 && !(c.faction_rule_flags&0x80u))return false;
 }
 switch(id){
  case 3:return !has(c,26);
  case 5:return has(c,4) || project(c,5);
  case 10:return !(c.faction_rule_flags&0x200u);
  case 11:return has(c,6) && !project(c,6);
  case 14:return has(c,13);
  case 16:return has(c,15);
  case 18:return has(c,17);
  case 20:return has(c,19) || has(c,22);
  case 21:return has(c,19);
  case 23:return !has(c,12); // Original Garden12 does not reciprocally test23.
  case 25:return has(c,24);
  case 32:return has(c,31);
  default:break;
 }
 const bool orbital=c.profile==Variant::base?(id>=34 && id<=37):((id>=65 && id<=68) || id==41);
 if(orbital)return has(c,29) || project(c,27) || project(c,35);
 return true;
}
}
LogicError facility_available(const FacilityDefinitions& d,const FacilityAvailabilityContext& c,size_t id,bool& output){
 if((c.profile!=Variant::base && c.profile!=Variant::alien_crossfire) || c.faction<1 || c.faction>7 ||
    c.difficulty<0 || c.difficulty>5 || c.queue_count<0 || c.queue_count>10 ||
    (c.profile==Variant::base && c.expansion_enabled))return LogicError::bad_state;
 const size_t slots=c.profile==Variant::base?72:134,repeat=c.profile==Variant::base?34:65,first=c.profile==Variant::base?39:70;
 if(d.slot_count!=slots || d.first_repeatable!=repeat || d.first_project!=first || d.project_count!=slots-first || id>=slots)return LogicError::bad_state;
 for(size_t i=0;i<89;++i)if(c.known_technology[i]>1)return LogicError::bad_state;
 for(size_t i=0;i<slots-first;++i)if(c.project_owner[i]<-2 || c.project_owner[i]>7)return LogicError::bad_state;
 if(d.slots[id].prerequisite<-2 || d.slots[id].prerequisite>88)return LogicError::bad_state;
 const bool next=calculate(d,c,(int)id);output=next;return LogicError::ok;
}
}
