/* SPDX-License-Identifier: GPL-3.0-or-later; source attribution in header. */
#include "world_battle_monument.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool alias(const OriginalWorld&w,const BasicRules&r,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&r,sizeof r))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
}
CombatError world_battle_monument(const OriginalWorld&w,const BasicRules&r,int32_t winner,int32_t enemy,BattleMonumentUpdate&out){
 if(alias(w,r,&out,sizeof out))return CombatError::bad_buffer;
 if(!w.loaded()||winner<1||winner>7||enemy<1||enemy>7||winner==enemy||r.normal_starting_year<0||r.normal_starting_year>999999||w.game().monuments.size!=0x27a0u)return CombatError::bad_state;
 const auto*f=w.faction_definition((size_t)winner);if(!f)return CombatError::bad_state;
 const int64_t year=(int64_t)r.normal_starting_year+w.game().preferences.CurrentTurn;
 if(year<-2147483647LL-1||year>2147483647LL)return CombatError::overflow;
 BattleMonumentUpdate next{};next.year=(int32_t)year;next.enemy_faction=enemy;
 const auto*raw=w.game().monuments.data;const size_t event=6u*0x38u;
 if(!(f->rule_flags&0x80u)&&!u32(raw+(size_t)winner*0x4f4u+event+0x10u)){
  next.changed=true;bool previous=false;
  for(int32_t i=0;i<8;++i)if(i!=winner&&u32(raw+(size_t)i*0x4f4u+event+0x10u))previous=true;
  next.first_achiever=previous?0:1;next.first_faction=previous?enemy:winner;
  if(previous)for(int32_t i=0;i<8;++i)if(i!=winner&&u32(raw+(size_t)i*0x4f4u+event+0x10u)&&u32(raw+(size_t)i*0x4f4u+event+0x14u))next.first_faction=i;
 }
 out=next;return CombatError::ok;
}
}
