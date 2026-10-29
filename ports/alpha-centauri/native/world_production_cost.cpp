/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_cost.h"
#include "original_human_cost.h"
#include "prototype.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool aliases(const OriginalWorld& w,const UnitDefinitions& d,const FacilityDefinitions& f,
 const BasicRules& b,const void* out,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;
 if(effects_range_overlap(out,n,&w,sizeof w) || effects_range_overlap(out,n,&d,sizeof d) ||
 effects_range_overlap(out,n,&f,sizeof f) || effects_range_overlap(out,n,&b,sizeof b))return true;
 if(!w.loaded())return false;
 const auto& g=w.game();
 return w.overlaps_byte_storage(out,n) ||
 effects_array_overlap(out,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile)) ||
 effects_array_overlap(out,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle)) ||
 effects_array_overlap(out,n,w.base(0),g.base_count,sizeof(OriginalBase)) ||
 effects_array_overlap(out,n,w.prototype(0),512,sizeof(OriginalPrototype)) ||
 effects_array_overlap(out,n,w.faction(0),8,sizeof(OriginalFactionState)) ||
 effects_array_overlap(out,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}
}
LogicError world_production_cost(const OriginalWorld& w,const UnitDefinitions& d,
 const FacilityDefinitions& facilities,const BasicRules& rules,Variant profile,
 size_t base_id,int32_t item,WorldProductionCost& output){
 if(aliases(w,d,facilities,rules,&output,sizeof output))return LogicError::bad_state;
 const bool expansion=profile==Variant::alien_crossfire;
 if(!w.loaded() || (profile!=Variant::base && !expansion) ||
 ((w.game().data_version>=11)!=expansion) ||
 d.weapon_count!=(expansion?26u:21u) || d.armor_count!=(expansion?14u:10u) ||
 d.ability_count!=(expansion?29u:24u) || facilities.slot_count!=(expansion?134u:72u) ||
 facilities.first_repeatable!=(expansion?65u:34u) || facilities.first_project!=(expansion?70u:39u))return LogicError::bad_state;
 const auto* b=w.base(base_id);
 if(!b || b->faction_id<1 || b->faction_id>7 ||
 !(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)b->faction_id)))return LogicError::bad_state;
 const auto* faction=w.faction((size_t)b->faction_id);if(!faction)return LogicError::bad_state;
 OriginalHumanCostInput facts{rules.nutrient_cost_multi,rules.mineral_cost_multi,
 w.game().map.info.world_settings[0],faction->SE_growth_pending,faction->SE_industry_pending,false,false};
 OriginalHumanCosts factors{};auto error=original_human_costs(facts,factors);if(error!=LogicError::ok)return error;
 WorldProductionCost next{factors.mineral,0,0,0,0};
 if(item<0){
  if(item==(-2147483647-1) || (size_t)(-item)>=facilities.slot_count)return LogicError::bad_state;
  next.item_cost=facilities.slots[-item].cost;
 }else{
  const auto* design=w.prototype((size_t)item);if(!design)return LogicError::bad_state;
  UnitProductionCostInput in{profile,item,design->cost,0,0,0,(uint8_t)design->plan,0,false,
   (design->unit_flags&4u)!=0,true,
   (b->facilities_built[35/8]&(1u<<(35%8)))!=0,
   (b->facilities_built[10/8]&(1u<<(10%8)))!=0,
   rules.extra_pct_cost_proto_land,rules.extra_pct_cost_proto_sea,rules.extra_pct_cost_proto_air};
  // Preserve original short circuits; unused design components/owner fields
  // cannot invalidate a typed design or the base edition's predefined path.
  if(expansion && item<64){
   if(design->weapon_id>=d.weapon_count)return LogicError::bad_state;
   in.offense=d.weapons[design->weapon_id].offense;
  }
  if(item>=64 && !in.prototype_complete){
   uint32_t component;error=calculate_prototype_cost(d,design->chassis_id,
    design->weapon_id,design->armor_id,0,design->reactor_id,component);
   if(error!=LogicError::ok)return error;
   if(component>2147483647u)return LogicError::overflow;
   in.component_base_cost=(int32_t)component;next.component_base_cost=(int32_t)component;
   in.triad=d.chassis[design->chassis_id].triad;
   const auto* owner=w.faction((size_t)item/64);
   const auto* definition=w.faction_definition((size_t)item/64);
   if(!owner || !definition)return LogicError::bad_state;
   in.design_owner_difficulty=owner->diff_level;
   in.design_owner_free_prototypes=(definition->rule_flags&0x200u)!=0;
  }
  UnitProductionCostResult price{};error=unit_production_cost(in,price);
  if(error!=LogicError::ok)return error;
  next.item_cost=price.cost;next.prototype_surcharge=price.prototype_surcharge;
 }
 error=production_mineral_requirement(next.item_cost,next.mineral_factor,next.mineral_requirement);
 if(error!=LogicError::ok)return error;
 output=next;return LogicError::ok;
}
}
