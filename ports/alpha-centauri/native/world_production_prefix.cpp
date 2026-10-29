/* SPDX-License-Identifier: GPL-3.0-or-later. See world_production_prefix.h. */
#include "world_production_prefix.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool alias(const OriginalWorld&w,const Rules&r,const ProductionPrefixResult&out){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 const void*p=&out;const size_t n=sizeof out;const auto&g=w.game();
 return effects_range_overlap(p,n,&w,sizeof w)||effects_range_overlap(p,n,&r,sizeof r)||w.overlaps_byte_storage(p,n)||
 effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||
 effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||
 effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError world_production_prefix(const OriginalWorld&w,const Rules&r,Variant profile,int32_t phase,ProductionPrefixResult&out){
 if(alias(w,r,out) || !w.loaded() || phase<0 || phase>7 ||
    (profile!=Variant::base && profile!=Variant::alien_crossfire) || ((w.game().data_version<11)!=(profile==Variant::base)))return LogicError::bad_state;
 const auto*f=w.faction((size_t)phase);if(!f)return LogicError::bad_state;
 ProductionPrefixResult next{};next.faction=*f;auto& n=next.faction;
 n.best_mineral_output=0;n.energy_surplus_total=0;n.facility_maint_total=0;n.turn_commerce_income=0;
 TechnologyEffectsResult effects;auto e=world_technology_effects(w,r,phase,effects);if(e!=LogicError::ok)return e;
 n.tech_commerce_bonus=effects.commerce;n.tech_fungus_nutrient=effects.fungus_nutrient;n.tech_fungus_mineral=effects.fungus_mineral;n.tech_fungus_energy=effects.fungus_energy;n.tech_fungus_unused=effects.fungus_unused;
 ProductionLoanFacts loans{};loans.faction=phase;loans.alive_mask=w.game().preferences.FactionStatus[1];
 for(unsigned i=0;i<8;++i){const auto*other=w.faction(i);if(!other)return LogicError::bad_state;loans.credits[i]=other->energy_credits;loans.sanctions[i]=other->sanction_turns;loans.balance[i]=f->loan_balance[i];loans.payment[i]=f->loan_payment[i];loans.diplomacy[i]=f->diplo_status[i];}
 ProductionLoanResult settled;e=production_loans(loans,settled);if(e!=LogicError::ok)return e;
 for(unsigned i=0;i<8;++i){next.credits[i]=settled.credits[i];n.loan_balance[i]=settled.balance[i];}
 n.energy_credits=settled.credits[(unsigned)phase];next.energy_before_bases=settled.energy_before_bases;n.player_flags&=~0x20u;
 for(auto&v:n.social_support)v=0;
 for(auto&row:n.social_psych)for(auto&v:row)v=0;
 for(auto&v:n.social_effic)v=0;
 n.unk_45=n.unk_46=n.unk_47=n.nutrient_surplus_total=n.labs_total=0;
 out=next;return LogicError::ok;
}
}
