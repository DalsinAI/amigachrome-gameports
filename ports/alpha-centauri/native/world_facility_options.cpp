/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_facility_options.h"
#include "world_unit_stats.h"
#include "movement.h"
#include <string.h>
namespace alpha {
namespace {
LogicError coast(const OriginalWorld& w,const OriginalBase& b,bool& output){
 const auto& m=w.game().map.info;MapGeometry g{(int32_t)m.width,(int32_t)m.height,m.flat};
 const int32_t dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};bool value=false;
 for(unsigned j=0;j<8;++j){
  int32_t x;if(movement_wrap_x(g,b.x+dx[j],x)!=LogicError::ok)return LogicError::bad_state;
  const int32_t y=b.y+dy[j];if(x<0 || x>=g.width || y<0 || y>=g.height)continue;
  size_t index;if(movement_tile_index(g,x,y,index)!=LogicError::ok)return LogicError::bad_state;
  const auto* tile=w.tile(index);if(!tile)return LogicError::bad_state;
  if((tile->climate&0xe0u)<0x60u)value=true;
 }
 output=value;return LogicError::ok;
}
LogicError caretaker(const OriginalFactionDefinition& f,bool& output){
 for(size_t i=0;i<sizeof f.filename && f.filename[i];++i)if((uint8_t)f.filename[i]>=128)return LogicError::bad_state;
 const char match[]="CARETAKE";bool value=true;
 for(size_t i=0;i<sizeof match;++i){unsigned char ch=(unsigned char)f.filename[i];if(ch>='a' && ch<='z')ch=(unsigned char)(ch-'a'+'A');if(ch!=(unsigned char)match[i]){value=false;break;}}
 output=value;return LogicError::ok;
}
}
LogicError world_facility_options(const OriginalWorld& w,const Rules& tech,const FacilityDefinitions& d,
 Variant profile,bool expansion_enabled,int32_t faction,int32_t base,int32_t queue_count,WorldFacilityOptions& output){
 if(!w.loaded() || tech.count!=89 || faction<1 || faction>7 || base< -1 || queue_count<0 || queue_count>10 ||
    (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    ((w.game().data_version<11)!=(profile==Variant::base)) || (profile==Variant::base && expansion_enabled))return LogicError::bad_state;
 const size_t count=profile==Variant::base?72:134,first=profile==Variant::base?39:70;
 if(d.slot_count!=count || d.first_repeatable!=(profile==Variant::base?34u:65u) || d.first_project!=first || d.project_count!=count-first)return LogicError::bad_state;
 const auto* f=w.faction_definition((size_t)faction);if(!f)return LogicError::bad_state;
 FacilityAvailabilityContext c{};c.profile=profile;c.expansion_enabled=expansion_enabled;c.faction=faction;
 c.difficulty=w.game().preferences.DiffLevel;c.queue_count=queue_count;c.game_rules=(uint32_t)w.game().preferences.GameRules;c.faction_rule_flags=(uint32_t)f->rule_flags;
 if(profile==Variant::alien_crossfire && caretaker(*f,c.caretaker)!=LogicError::ok)return LogicError::bad_state;
 if(base>=0){const auto* b=w.base((size_t)base);if(!b)return LogicError::bad_state;c.has_base=true;
  memcpy(c.facilities,b->facilities_built,sizeof c.facilities);memcpy(c.queue,b->queue_items,sizeof c.queue);
  if(coast(w,*b,c.coastal)!=LogicError::ok)return LogicError::bad_state;
 }
 for(size_t i=0;i<89;++i){bool known;if(world_has_technology(w,tech,(int32_t)i,faction,known)!=LogicError::ok)return LogicError::bad_state;c.known_technology[i]=known?1:0;}
 for(size_t i=0;i<count-first;++i){const int32_t location=w.game().preferences.SecretProjects[i];
  if(location< -2)return LogicError::bad_state;
  if(location<0)c.project_owner[i]=(int8_t)location;
  else {const auto* b=w.base((size_t)location);if(!b)return LogicError::bad_state;c.project_owner[i]=b->faction_id;}
 }
 WorldFacilityOptions next{};next.count=(uint32_t)count;next.adjacent_ocean=c.coastal;
 for(size_t i=0;i<count;++i){bool available;if(facility_available(d,c,i,available)!=LogicError::ok)return LogicError::bad_state;next.available[i]=available?1:0;}
 output=next;return LogicError::ok;
}
}
