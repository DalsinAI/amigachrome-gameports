/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in BASE-COMMERCE.md. */
#include "world_base_commerce.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){if(!a||!b||!an||!bn)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){
 if(overlap(p,n,&w,sizeof w))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();return overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(p,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
CommerceError effect(const OriginalWorld&w,const Rules&t,int faction,int32_t&out){TechnologyEffects e{};const auto error=world_technology_effects(w,t,faction,e);if(error==TechnologyEffectsError::overflow)return CommerceError::overflow;if(error!=TechnologyEffectsError::ok)return CommerceError::bad_state;out=e.commerce;return CommerceError::ok;}
}
CommerceError world_base_commerce(const OriginalWorld&w,const Rules&t,size_t id,const int32_t*energy,size_t count,WorldCommerceResult&out){
 if(count>512)return CommerceError::bad_state;
 const size_t bytes=count*sizeof(int32_t);
 if(world_alias(w,&out,sizeof out)||overlap(&out,sizeof out,&t,sizeof t)||overlap(&out,sizeof out,energy,bytes)||world_alias(w,energy,bytes)||overlap(energy,bytes,&t,sizeof t))return CommerceError::bad_buffer;
 if(!w.loaded()||t.count!=89||!energy||count!=w.game().base_count||!count)return CommerceError::bad_state;
 const auto*base=w.base(id);if(!base||base->faction_id<1||base->faction_id>7)return CommerceError::bad_state;
 const int faction=base->faction_id;const auto*f=w.faction((size_t)faction);
 if(w.game().data_version>=11||!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction)))return CommerceError::unsupported;
 const int32_t governor=w.game().preferences.GovernorFaction;if(governor< -1||governor>7)return CommerceError::bad_state;
 int32_t scores[512];uint16_t counts[8]{};
 for(size_t i=0;i<count;++i){const auto*b=w.base(i);if(!b||b->faction_id<0||b->faction_id>7||energy[i]<0)return CommerceError::bad_state;if(!put(512LL*energy[i]+(int64_t)i,scores[i]))return CommerceError::overflow;++counts[(unsigned)b->faction_id];}
 WorldCommerceResult next{};for(auto&partner:next.partner_base)partner=-1;
 for(unsigned i=0;i<89;++i){const auto&tech=t.technologies[i];if(tech.prerequisites[0]< -2||tech.prerequisites[0]>=89||tech.prerequisites[1]< -2||tech.prerequisites[1]>=89)return CommerceError::bad_state;if(tech.prerequisites[0]!=-2&&tech.prerequisites[1]!=-2&&(tech.flags&4u))++next.technology_count;}
 for(size_t i=0;i<count;++i)if(w.base(i)->faction_id==faction&&scores[i]>scores[id])++next.rank;
 const bool global=w.game().preferences.ProposalPassCount[4]<w.game().preferences.ProposalPassCount[3];bool our_effect=false;
 if(!f->sanction_turns)for(int other=1;other<8;++other){const auto*them=w.faction((size_t)other);if(other==faction||them->sanction_turns||!counts[other]||!(f->diplo_status[other]&2u)||next.rank>=counts[other])continue;
  if(!(them->diplo_status[faction]&2u))return CommerceError::bad_state;
  int32_t partner=-1;
  for(size_t i=0;i<count;++i)if(w.base(i)->faction_id==other){int rank=0;for(size_t j=0;j<count;++j)if(w.base(j)->faction_id==other&&scores[j]>scores[i])++rank;if(rank==next.rank){partner=(int32_t)i;break;}}
  if(partner<0)return CommerceError::bad_state;
  if(!our_effect){const auto e=effect(w,t,faction,next.fresh_technology_bonus[faction]);if(e!=CommerceError::ok)return e;our_effect=true;}
  const auto e=effect(w,t,other,next.fresh_technology_bonus[other]);if(e!=CommerceError::ok)return e;
  const CommercePartnerFacts facts{energy[id],energy[(size_t)partner],next.technology_count,next.fresh_technology_bonus[faction],next.fresh_technology_bonus[other],global,(f->diplo_status[other]&1u)!=0,governor==faction,governor==other};CommercePartnerResult result{};const auto error=commerce_partner(facts,result);if(error!=CommerceError::ok)return error;
  next.partner_base[other]=partner;next.import_value[other]=result.import_value;next.export_value[other]=result.export_value;if(!put((int64_t)next.total+result.import_value,next.total))return CommerceError::overflow;
 }
 out=next;return CommerceError::ok;
}
}
