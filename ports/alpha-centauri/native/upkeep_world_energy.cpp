/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in upkeep_world_energy.h. */
#include "upkeep_world_energy.h"
#include "movement.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool aliases(const OriginalWorld&w,const void*stage,size_t sn,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,stage,sn))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool geometry(const OriginalWorld&w,MapGeometry&g){if(!w.loaded())return false;const auto&m=w.game().map.info;
 if(m.width<2||m.width>4096||(m.width&1u)||!m.height||m.height>4096||m.tile_count!=(m.width/2)*m.height||w.game().vehicle_count>2048||w.game().base_count>512)return false;
 g={(int32_t)m.width,(int32_t)m.height,m.flat};return true;
}
}
EnergyError world_energy_accounting(const OriginalWorld&w,size_t id,const UpkeepWorldEnergyStage&s,UpkeepWorldEnergyResult&output){
 if(aliases(w,&s,sizeof s,&output,sizeof output))return EnergyError::bad_buffer;
 MapGeometry g;if(!geometry(w,g))return EnergyError::bad_state;const auto*b=w.base(id);if(!b||b->faction_id<0||b->faction_id>=8)return EnergyError::bad_state;
 const auto*f=w.faction((size_t)b->faction_id);if(!f)return EnergyError::bad_state;int32_t nearest=999;
 for(size_t i=0;i<w.game().base_count;++i){const auto*h=w.base(i);if(h->faction_id!=b->faction_id||!(h->facilities_built[0]&2u))continue;
  int32_t x;if(movement_distance_x(g,h->x,b->x,x)!=LogicError::ok)return EnergyError::bad_state;
  const int32_t y=h->y>b->y?h->y-b->y:b->y-h->y,hi=x>y?x:y,lo=x<y?x:y;
  const int32_t d=hi-(((x+y)/2-lo+1)/2);if(d<nearest)nearest=d;
 }
 UpkeepWorldEnergyResult r{};r.headquarters_distance=nearest==999?-1:nearest;r.pending_efficiency=f->SE_effic_pending;r.physical_creche=(b->facilities_built[0]&4u)!=0;
 EnergyFacts facts{s.fresh_intake,s.convoy_to,s.commerce,s.energy_grid,s.convoy_from,r.headquarters_distance,r.pending_efficiency,s.upkeep_stage,r.physical_creche,{}};
 for(unsigned j=0;j<9;++j)facts.social_efficiency_before[j]=f->social_effic[j];
 const auto e=base_energy_accounting(facts,r.accounting);if(e!=EnergyError::ok)return e;output=r;return EnergyError::ok;
}
EnergyError world_energy_shortfall_orders(const OriginalWorld&w,size_t id,int32_t surplus,EnergyShortfallOrders&output){
 if(aliases(w,nullptr,0,&output,sizeof output))return EnergyError::bad_buffer;
 MapGeometry g;if(!geometry(w,g))return EnergyError::bad_state;const auto*b=w.base(id);if(!b||b->faction_id<0||b->faction_id>=8)return EnergyError::bad_state;
 EnergyShortfallOrders r{};
 if(surplus<0)for(size_t i=0;i<w.game().vehicle_count;++i){const auto*v=w.vehicle(i);if(v->faction_id!=b->faction_id||v->order!=3||v->order_auto_type!=2)continue;
  if(v->unit_id<0||v->unit_id>=512)return EnergyError::bad_state;
  const auto*p=w.prototype((size_t)v->unit_id);if(!p||p->plan!=10)continue;
  size_t tile;if(movement_tile_index(g,v->x,v->y,tile)!=LogicError::ok)return EnergyError::bad_state;const auto*t=w.tile(tile);if(!t)return EnergyError::bad_state;
  if((t->items&1u)&&(t->val2&15u)<8){r.cancel[i/32]|=1u<<(i%32);++r.count;}
 }
 output=r;return EnergyError::ok;
}
}
