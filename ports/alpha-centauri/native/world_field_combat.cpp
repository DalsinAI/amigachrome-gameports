/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker default field strength/round policy, MIT pin in combat_rounds.h.
 * Effective morale delegates to recovered OpenSMACX world_unit_stats.
 */
#include "world_field_combat.h"
#include "movement.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool alias(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&d,sizeof d)||overlap(out,n,&t,sizeof t)||overlap(out,n,&r,sizeof r))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool scale(int64_t&value,int64_t numerator,int64_t denominator=100){
 if(numerator<0||denominator<1||value<0||value>2147483647LL||numerator>2147483647LL)return false;
 value=value*numerator/denominator;return value<=2147483647LL;
}
bool combat_proto(const OriginalPrototype&p,const UnitDefinitions&d){
 return p.chassis_id<9&&p.weapon_id<d.weapon_count&&p.armor_id<d.armor_count
 &&p.reactor_id>=1&&p.reactor_id<=4&&p.plan>=0&&p.plan<=3&&!p.ability_flags
 &&d.chassis[p.chassis_id].triad==0&&d.chassis[p.chassis_id].speed==1
 &&!d.chassis[p.chassis_id].missile&&d.weapons[p.weapon_id].offense>0
 &&d.weapons[p.weapon_id].mode==0&&d.armor[p.armor_id].defense>=0;
}
}
CombatError world_field_combat(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,
 const BasicRules&r,size_t aid,size_t did,FieldCombatFacts&out){
 if(alias(w,d,t,r,&out,sizeof out))return CombatError::bad_buffer;
 if(!w.loaded()||t.count!=89||aid==did||r.move_rate_roads<1||r.move_rate_roads>100)return CombatError::bad_state;
 if(w.game().data_version>=11||d.weapon_count!=21||d.armor_count!=10)return CombatError::unsupported;
 const auto*a=w.vehicle(aid);const auto*b=w.vehicle(did);
 if(!a||!b||a->unit_id<0||b->unit_id<0||a->faction_id<1||a->faction_id>7||b->faction_id<1||b->faction_id>7)return CombatError::bad_state;
 if(a->faction_id!=w.game().player||a->faction_id==b->faction_id||!(w.game().preferences.FactionStatus[0]&(1u<<a->faction_id)))return CombatError::unsupported;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);
 if(!ap||!bp)return CombatError::bad_state;
 if(!combat_proto(*ap,d)||!combat_proto(*bp,d))return CombatError::unsupported;
 if(a->order||a->waypoint_count||a->state&(0x0b084221u|0x1000u)||a->next_veh_id_stack!=-1||a->prev_veh_id_stack!=-1||b->next_veh_id_stack!=-1||b->prev_veh_id_stack!=-1)return CombatError::unsupported;
 const auto*af=w.faction(a->faction_id);const auto*bf=w.faction(b->faction_id);
 const auto*ar=w.faction_definition(a->faction_id);const auto*br=w.faction_definition(b->faction_id);
 if(!af||!bf||!ar||!br)return CombatError::bad_state;
 if((af->diplo_status[b->faction_id]&0x17u)!=0x10u||(bf->diplo_status[a->faction_id]&0x17u)!=0x10u)return CombatError::unsupported;
 if(ar->faction_bonus_count<0||ar->faction_bonus_count>8||br->faction_bonus_count<0||br->faction_bonus_count>8||af->diff_level<0||af->diff_level>5||bf->diff_level<0||bf->diff_level>5)return CombatError::bad_state;
 const auto&m=w.game().map.info;const MapGeometry geo{(int32_t)m.width,(int32_t)m.height,m.flat};
 size_t at,bt;if(movement_tile_index(geo,a->x,a->y,at)!=LogicError::ok||movement_tile_index(geo,b->x,b->y,bt)!=LogicError::ok)return CombatError::bad_state;
 int32_t xdist;if(movement_distance_x(geo,a->x,b->x,xdist)!=LogicError::ok)return CombatError::bad_state;
 const int ydist=a->y>b->y?a->y-b->y:b->y-a->y;
 if(!((xdist==1&&ydist==1)||(xdist==2&&!ydist)||(!xdist&&ydist==2)))return CombatError::unsupported;
 const auto*as=w.tile(at);const auto*bs=w.tile(bt);if(!as||!bs)return CombatError::bad_state;
 if(!(as->items&2u)||!(bs->items&2u)||(as->val2&15u)!=a->faction_id||(bs->val2&15u)!=b->faction_id)return CombatError::unsupported;
 if((as->climate&0xe0u)<0x60u||(bs->climate&0xe0u)<0x60u||(as->items&(1u|0x20u|0x800u))||(bs->items&(1u|0x20u|0x800u))||!(bs->visibility&(1u<<a->faction_id))||!(b->visibility&(1u<<a->faction_id)))return CombatError::unsupported;
 // Conservatively reject all sensor/GSP worlds until exact nearby sensor and
 // nearest-base territory resolution is connected. Never silently omit bonus.
 for(size_t i=0;i<m.tile_count;++i)if(w.tile(i)->items&0x80000000u)return CombatError::unsupported;
 for(size_t i=0;i<w.game().base_count;++i){const auto*base=w.base(i);
  if((base->x==a->x&&base->y==a->y)||(base->x==b->x&&base->y==b->y)||(base->facilities_built[5]&2u))return CombatError::unsupported;
 }
 size_t ac=0,bc=0;for(size_t i=0;i<w.game().vehicle_count;++i){const auto*v=w.vehicle(i);ac+=v->x==a->x&&v->y==a->y;bc+=v->x==b->x&&v->y==b->y;}
 if(ac!=1||bc!=1)return CombatError::unsupported;
 FieldCombatFacts next{};
 if(world_effective_morale(w,d,t,aid,true,next.attacker_morale)!=LogicError::ok||world_effective_morale(w,d,t,did,true,next.defender_morale)!=LogicError::ok)return CombatError::bad_state;
 uint32_t speed;if(world_vehicle_speed(w,d,t,aid,r.move_rate_roads,false,speed)!=LogicError::ok)return CombatError::bad_state;
 if(a->moves_spent>=speed)return CombatError::unsupported;
 next.attacker_moves_left=(int32_t)(speed-a->moves_spent);
 // Both are infantry. Even elite defender cannot trigger source retreat,
 // because retreat speed comparisons explicitly skip the elite morale bonus.
 const int32_t defense_rating=d.armor[bp->armor_id].defense<1?1:d.armor[bp->armor_id].defense>9999?9999:d.armor[bp->armor_id].defense;
 int64_t offense=(int64_t)d.weapons[ap->weapon_id].offense*8*(next.attacker_morale+6)*4;
 int64_t defense=(int64_t)defense_rating*8*((next.defender_morale<1?1:next.defender_morale)+6);
 if(offense>2147483647LL||defense>2147483647LL)return CombatError::overflow;
 if((ar->rule_flags&0x400u)&&!scale(offense,100LL+r.combat_pct_fanatic_atk_bonus))return CombatError::overflow;
 for(int i=0;i<ar->faction_bonus_count;++i)if(ar->faction_bonus_id[i]==18&&!scale(offense,ar->faction_bonus_val1[i]))return CombatError::overflow;
 for(int i=0;i<br->faction_bonus_count;++i)if(br->faction_bonus_id[i]==17&&!scale(defense,br->faction_bonus_val1[i]))return CombatError::overflow;
 if((as->items&12u)&&(bs->items&12u)&&!scale(offense,100LL+r.combat_pct_atk_road))return CombatError::overflow;
 const unsigned aa=as->climate>>5,ba=bs->climate>>5;
 if(ba>aa&&!scale(defense,100LL+r.combat_pen_pct_atk_lwr_elev))return CombatError::overflow;
 if(aa>ba&&!scale(offense,100LL+r.combat_pct_atk_higher_elev))return CombatError::overflow;
 next.terrain_defense=((bs->val3&0xc0u)>0x40u||(bs->items&0x200000u))?3:2;
 if(!scale(defense,2*next.terrain_defense,1))return CombatError::overflow;
 const int left=next.attacker_moves_left<r.move_rate_roads?next.attacker_moves_left:r.move_rate_roads;
 if(!scale(offense,left,r.move_rate_roads))return CombatError::overflow;
 const bool bhuman=(w.game().preferences.FactionStatus[0]&(1u<<b->faction_id))!=0;
 if(!bhuman&&af->diff_level<2&&!scale(offense,4-af->diff_level,2))return CombatError::overflow;
 next.duel={(int32_t)offense,(int32_t)defense,ap->reactor_id,bp->reactor_id,a->damage_taken,b->damage_taken};
 if(a->damage_taken>=ap->reactor_id*10||b->damage_taken>=bp->reactor_id*10)return CombatError::bad_state;
 out=next;return CombatError::ok;
}
CombatError world_resolve_field_combat(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,
 const BasicRules&r,size_t aid,size_t did,CombatRollStream&stream,FieldCombatFacts&facts,CombatResult&out){
 if(stream.count>SIZE_MAX/sizeof(uint16_t))return CombatError::bad_buffer;
 const size_t bytes=stream.count*sizeof(uint16_t);
 if(alias(w,d,t,r,&stream,sizeof stream)||alias(w,d,t,r,&facts,sizeof facts)||alias(w,d,t,r,&out,sizeof out)
 ||overlap(&stream,sizeof stream,&facts,sizeof facts)||overlap(&stream,sizeof stream,&out,sizeof out)||overlap(&facts,sizeof facts,&out,sizeof out)
 ||overlap(&stream,sizeof stream,stream.values,bytes)||overlap(&facts,sizeof facts,stream.values,bytes)||overlap(&out,sizeof out,stream.values,bytes))return CombatError::bad_buffer;
 FieldCombatFacts next{};auto e=world_field_combat(w,d,t,r,aid,did,next);if(e!=CombatError::ok)return e;
 CombatRollStream rng=stream;CombatResult result{};e=resolve_combat_rounds(next.duel,rng,result);if(e!=CombatError::ok)return e;
 stream.cursor=rng.cursor;facts=next;out=result;return CombatError::ok;
}
}
