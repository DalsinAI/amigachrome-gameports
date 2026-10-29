/* SPDX-License-Identifier: GPL-3.0-or-later. See social_upkeep.h. */
#include "world_social_upkeep.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool alias(const OriginalWorld&w,const SocialRules&r,const SocialUpkeepResult&out){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 const void*p=&out;const size_t n=sizeof out;
 if(effects_range_overlap(p,n,&w,sizeof w)||effects_range_overlap(p,n,&r,sizeof r)||w.overlaps_byte_storage(p,n))return true;
 const auto&g=w.game();
 return effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||
 effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||
 effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||
 effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||
 effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||
 effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError world_social_upkeep(const OriginalWorld&w,const SocialRules&r,Variant profile,int32_t id,SocialUpkeepResult&out){
 if(alias(w,r,out))return LogicError::bad_state;
 if(!w.loaded()||id<1||id>7||(profile!=Variant::base&&profile!=Variant::alien_crossfire)||
    ((w.game().data_version<11)!=(profile==Variant::base))||r.bonus_count!=(profile==Variant::base?29u:41u))return LogicError::bad_state;
 const auto*f=w.faction((size_t)id);const auto*d=w.faction_definition((size_t)id);if(!f||!d)return LogicError::bad_state;
 SocialUpkeepFacts facts{};facts.variant=profile;
 facts.selected[0]=(uint32_t)f->SE_Politics_pending;facts.selected[1]=(uint32_t)f->SE_Economics_pending;
 facts.selected[2]=(uint32_t)f->SE_Values_pending;facts.selected[3]=(uint32_t)f->SE_Future_pending;
 const int32_t initial[]={f->SE_economy_base,f->SE_effic_base,f->SE_support_base,f->SE_talent_base,f->SE_morale_base,f->SE_police_base,f->SE_growth_base,f->SE_planet_base,f->SE_probe_base,f->SE_industry_base,f->SE_research_base};
 for(unsigned i=0;i<11;++i)facts.base_effects[i]=initial[i];
 facts.bonus_count=d->faction_bonus_count;facts.rule_flags=(uint32_t)d->rule_flags;
 for(unsigned i=0;i<8;++i){facts.bonus_kind[i]=d->faction_bonus_id[i];facts.bonus_first[i]=d->faction_bonus_val1[i];facts.bonus_second[i]=d->faction_bonus_val2[i];}
 // These are project INDICES, not facility IDs; same four indices in both.
 const unsigned indices[]={21,24,13,23};bool* owned[]={&facts.network_backbone,&facts.cloning_vats,&facts.ascetic_virtues,&facts.living_refinery};
 for(unsigned i=0;i<4;++i){const int32_t at=w.game().preferences.SecretProjects[indices[i]];
  if(at<0)continue;
  const auto*base=w.base((size_t)at);if(!base)return LogicError::bad_state;*owned[i]=base->faction_id==id;
 }
 // For has_temple's whose_territory(faction,x,y,null,0), returning this
 // faction requires owner==faction>0. Foreign treaties cannot change that.
 for(size_t i=0;i<w.game().map.info.tile_count;++i){const auto*t=w.tile(i);if(!t)return LogicError::bad_state;
  if((t->landmarks&0x80002000u)==0x2000u && !(t->landmarks&0xff000000u) &&
     t->owner==id && (t->visibility&(1u<<(unsigned)id))){facts.temple=true;break;}
 }
 return social_upkeep(r,facts,out);
}
}
