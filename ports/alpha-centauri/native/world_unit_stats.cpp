/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in world_unit_stats.h. */
#include "world_unit_stats.h"
#include "movement.h"
namespace alpha {
static int32_t clamp(int32_t n,int32_t low,int32_t high){return n<low?low:n>high?high:n;}
static bool signed_word(int64_t n){return n>=(-2147483647LL-1) && n<=2147483647LL;}
static bool profile(const OriginalWorld& w,const UnitDefinitions& d){return w.loaded() && ((w.game().data_version<11 && d.weapon_count==21) || (w.game().data_version>=11 && d.weapon_count==26));}
LogicError world_has_project(const OriginalWorld& w,size_t project,int32_t faction,bool& output){
 if(!w.loaded() || project>=64 || faction<0 || faction>=8)return LogicError::bad_state;
 const int32_t base=w.game().preferences.SecretProjects[project];bool present=false;
 if(base>=0){const auto* b=w.base((size_t)base);if(!b)return LogicError::bad_state;present=b->faction_id==faction;}
 output=present;return LogicError::ok;
}
LogicError world_has_technology(const OriginalWorld& w,const Rules& tech,int32_t id,int32_t faction,bool& output){
 if(!w.loaded() || tech.count!=89 || faction<0 || faction>=8)return LogicError::bad_state;
 bool present=false;
 if(faction>0){
  if(id==-1)present=true;
  else if(id>=0 && id<88){const auto& t=tech.technologies[id];
   if(t.prerequisites[0]!=-2 && !(t.prerequisites[1]==-2 && t.prerequisites[0]!=-1))present=(w.game().preferences.TechOwners[id]&(1u<<faction))!=0;
  }
 }
 output=present;return LogicError::ok;
}
static LogicError facility(const OriginalWorld& w,int32_t base,uint32_t id,bool& output){
 if(base<0){output=false;return LogicError::ok;}
 const auto* b=w.base((size_t)base);if(!b || id>64)return LogicError::bad_state;
 output=(b->facilities_built[id/8]&(1u<<(id%8)))!=0;return LogicError::ok;
}
static LogicError native_morale(const OriginalWorld& w,const OriginalVehicle& v,uint32_t& output){
 int32_t morale;
 if(w.game().data_version>=11 && v.unit_id==19){
  morale=-2;const int32_t dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};
  MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};
  for(unsigned i=0;i<8;++i){int32_t x;if(movement_wrap_x(g,v.x+dx[i],x)!=LogicError::ok)return LogicError::bad_state;
   size_t index;if(movement_tile_index(g,x,v.y+dy[i],index)!=LogicError::ok)continue;
   const auto* tile=w.tile(index);if(!tile)return LogicError::bad_state;
   if((tile->items&0x20u) && (tile->climate&0xe0u)>=0x40u)++morale;
  }
 }else{
  const int32_t turn=w.game().preferences.CurrentTurn;morale=turn<45?0:turn<90?1:turn<170?2:turn<250?3:turn<330?4:6;
  if(v.state&0x100u)++morale;
  if(v.unit_id==10)++morale;
  morale+=(v.flags>>3)&3u;
 }
 output=(uint32_t)clamp(morale,0,6);return LogicError::ok;
}
LogicError world_effective_morale(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t index,bool riots,uint32_t& output){
 if(!profile(w,d) || tech.count!=89)return LogicError::bad_state;
 const auto* v=w.vehicle(index);if(!v || v->unit_id<0)return LogicError::bad_state;
 const auto* p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
 uint32_t ignored;auto error=saved_prototype_speed(d,*p,(size_t)v->unit_id,1,false,ignored);if(error!=LogicError::ok)return error;
 if(!v->faction_id)return native_morale(w,*v,output);
 const auto* state=w.faction(v->faction_id);const auto* rules=w.faction_definition(v->faction_id);if(!state || !rules)return LogicError::bad_state;
 if(p->plan==11){
  int32_t morale=clamp(state->SE_probe,0,3);bool project;
  error=world_has_project(w,30,v->faction_id,project);if(error!=LogicError::ok)return error;if(project)morale+=2;
  for(int32_t id=0;id<89;++id)if(tech.technologies[id].flags&2u){bool achieved;error=world_has_technology(w,tech,id,v->faction_id,achieved);if(error!=LogicError::ok)return error;if(achieved)++morale;}
  output=(uint32_t)clamp(morale+v->morale,2,6);return LogicError::ok;
 }
 const bool basic_psi=v->unit_id<64 && d.weapons[p->weapon_id].offense<0;
 if(basic_psi){output=(uint32_t)clamp(v->morale,0,6);return LogicError::ok;}
 int64_t modifier=clamp(state->SE_morale,-4,4);if(modifier<=-2)++modifier;else if(modifier>=2)--modifier;
 if(rules->rule_morale<0)modifier+=rules->rule_morale;
 if(!signed_word(modifier))return LogicError::overflow;
 if(v->home_base_id>=0){
  bool creche;error=facility(w,v->home_base_id,2,creche);if(error!=LogicError::ok)return error;if(creche && modifier<0)modifier/=2;
  bool brood;error=facility(w,v->home_base_id,35,brood);if(error!=LogicError::ok)return error;
  if(brood && v->unit_id<64 && (d.weapons[p->weapon_id].offense<0 || (w.game().data_version>=11 && v->unit_id==15)) && modifier<0)modifier/=2;
 }
 if(rules->rule_morale>0)modifier+=rules->rule_morale;
 if(!signed_word(modifier))return LogicError::overflow;
 const bool immune=(rules->rule_flags&0x8000u)!=0;if(immune && modifier<0)modifier=0;
 if(riots && v->home_base_id>=0){const auto* base=w.base((size_t)v->home_base_id);if(!base)return LogicError::bad_state;if((base->state_flags&2u) && !immune)--modifier;}
 const int64_t total=modifier+v->morale;if(!signed_word(total))return LogicError::overflow;
 output=(uint32_t)clamp((int32_t)total,0,6);return LogicError::ok;
}
LogicError world_speed_ability_flags(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t id,uint32_t& output){
 if(!profile(w,d))return LogicError::bad_state;
 const auto* p=w.prototype(id);if(!p)return LogicError::bad_state;
 uint32_t ignored;auto error=saved_prototype_speed(d,*p,id,1,false,ignored);if(error!=LogicError::ok)return error;
 uint32_t flags=p->ability_flags;
 if(d.weapon_count==21 || id<64){output=flags;return LogicError::ok;}
 const uint32_t requested=d.chassis[p->chassis_id].triad==2?
   (0x800420u | (d.ability_count>27?0x8000000u:0u)):0x800400u;
 if((flags&requested)==requested){output=flags;return LogicError::ok;}
 const auto* faction=w.faction_definition(id/64);
 if(!faction || faction->faction_bonus_count<0 || faction->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<faction->faction_bonus_count;++i)if(faction->faction_bonus_id[i]==15){
  const int ability=faction->faction_bonus_val1[i];if(ability<0 || (uint32_t)ability>=d.ability_count)return LogicError::bad_state;
  const uint32_t bit=1u<<ability;if(!(requested&bit) || (flags&bit))continue;
  bool achieved;error=world_has_technology(w,tech,d.abilities[ability].prerequisite,(int32_t)(id/64),achieved);
  if(error!=LogicError::ok)return error;
  if(achieved)flags|=bit;
 }
 output=flags;return LogicError::ok;
}
LogicError world_vehicle_speed(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t index,int32_t road,bool skip_morale,uint32_t& output){
 if(!profile(w,d))return LogicError::bad_state;
 const auto* v=w.vehicle(index);if(!v || v->unit_id<0)return LogicError::bad_state;const auto* p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
 uint32_t ignored;auto error=saved_prototype_speed(d,*p,(size_t)v->unit_id,road,false,ignored);if(error!=LogicError::ok)return error;
 SavedSpeedContext context{false,false,skip_morale,0};
 if(w.game().data_version>=11 && v->unit_id==19)return saved_vehicle_speed(d,*p,*v,road,context,output);
 const int32_t triad=d.chassis[p->chassis_id].triad;
 if(triad==2){error=world_has_project(w,35,v->unit_id/64,context.cloudbase_academy);if(error!=LogicError::ok)return error;}
 if(triad==1){error=world_has_project(w,10,v->faction_id,context.maritime_control_center);if(error!=LogicError::ok)return error;}
 if(!skip_morale){uint32_t morale;error=world_effective_morale(w,d,tech,index,true,morale);if(error!=LogicError::ok)return error;context.effective_morale=(int32_t)morale;}
 OriginalPrototype effective=*p;
 error=world_speed_ability_flags(w,d,tech,(size_t)v->unit_id,effective.ability_flags);if(error!=LogicError::ok)return error;
 return saved_vehicle_speed(d,effective,*v,road,context,output);
}
}
