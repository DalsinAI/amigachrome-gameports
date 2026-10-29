/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_text.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
WorldCombatTextError numeric(WorldBattleEntryError e){switch(e){case WorldBattleEntryError::ok:return WorldCombatTextError::ok;case WorldBattleEntryError::bad_state:return WorldCombatTextError::bad_state;case WorldBattleEntryError::missing_runtime:return WorldCombatTextError::missing_runtime;case WorldBattleEntryError::unsupported:return WorldCombatTextError::unsupported;case WorldBattleEntryError::overflow:return WorldCombatTextError::overflow;}return WorldCombatTextError::bad_state;}
}
WorldCombatTextError inspect_world_combat_text(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t a,size_t b,const WorldBattleReportingRuntime&runtime,const AttackModifierLedger&prior,const OriginalLabelsView&labels,const CombatTextLedger&text,WorldCombatTextInspection&out){
 using E=WorldCombatTextError;using detail::effects_range_overlap;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out))return E::bad_state;
 const void*inputs[]={&d,&t,&r,&runtime,&prior,&labels,&text};const size_t sizes[]={sizeof d,sizeof t,sizeof r,sizeof runtime,sizeof prior,sizeof labels,sizeof text};
 for(unsigned i=0;i<7;++i){if(alias(w,inputs[i],sizes[i])||effects_range_overlap(inputs[i],sizes[i],&out,sizeof out))return E::bad_state;for(unsigned j=0;j<i;++j)if(effects_range_overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return E::bad_state;}
 if(!labels.bytes||!labels.size||labels.size>0xffffffffu||!labels.count||labels.count>2048||alias(w,labels.bytes,labels.size)||effects_range_overlap(labels.bytes,labels.size,&out,sizeof out))return E::bad_state;
 for(unsigned i=0;i<7;++i)if(effects_range_overlap(inputs[i],sizes[i],labels.bytes,labels.size))return E::bad_state;
 const Variant ed=w.game().data_version>=11?Variant::alien_crossfire:Variant::base;
 if(labels.edition!=ed)return E::unsupported;
 WorldCombatTextInspection next{};const auto error=inspect_world_battle_reporting(w,d,t,r,a,b,runtime,prior,next.numeric);if(error!=WorldBattleEntryError::ok)return numeric(error);
 switch(resolve_combat_reporting(labels,ed,next.numeric,text,next.text)){
 case CombatLabelError::ok:break;
 case CombatLabelError::bad_input:return E::bad_state;
 case CombatLabelError::unsupported:return E::unsupported;
 case CombatLabelError::missing_label:return E::missing_label;
 case CombatLabelError::text_too_long:return E::text_too_long;
 }
 if(next.text.resolved_lookups!=next.numeric.lookup_count)return E::bad_state;
 out=next;return E::ok;
}
}
