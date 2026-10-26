/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
#include <new>
#include <limits.h>
namespace alpha {
struct LocalAccountingTables {UnitDefinitions units;ResourceInfo resources;CitizenDefinitions citizens;};
namespace {
bool overlap(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(n>UINTPTR_MAX-x || m>UINTPTR_MAX-y)return true;
 return x<y+m && y<x+n;
}
bool disjoint(const OriginalWorld& w,const void* p,size_t n){
 return !overlap(p,n,&w,sizeof(w)) && !overlap(p,n,w.original_bytes(),w.original_size()) &&
 !overlap(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) &&
 !overlap(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) &&
 !overlap(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) &&
 !overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) &&
 !overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState)) &&
 !overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
LocalOrderError translated(StableIntakeError e){
 switch(e){case StableIntakeError::ok:return LocalOrderError::ok;
 case StableIntakeError::out_of_memory:return LocalOrderError::no_memory;
 case StableIntakeError::overflow:return LocalOrderError::overflow;
 case StableIntakeError::bad_state:case StableIntakeError::bad_buffer:return LocalOrderError::bad_state;
 default:return LocalOrderError::unsupported;}
}
OriginalBase patched(const OriginalBase& before,const WorldEnergyResult& r){
 OriginalBase b=before;const auto& n=r.resources.food.fresh.nutrients;
 const auto& m=r.resources.minerals;const auto& e=r.energy.accounting;
 b.nutrients_accumulated=n.stored;b.nutrient_intake_2=n.intake;
 b.nutrient_consumption=n.consumption;b.nutrient_surplus=n.surplus;
 b.eco_damage=m.eco_damage;b.mineral_intake_2=m.intake;
 b.mineral_consumption=m.consumption;b.mineral_surplus=m.surplus;
 b.mineral_surplus_final=m.final_surplus;b.mineral_inefficiency=m.inefficiency;
 b.energy_intake_2=e.intake;b.energy_consumption=e.consumption;
 b.energy_surplus=e.surplus;b.energy_inefficiency=e.inefficiency;
 b.economy_total=e.economy;b.psych_total=e.psych;b.labs_total=e.labs;b.unk_total=e.unknown_total;
 b.talent_total=r.energy.population.talents;b.drone_total=r.energy.population.drones;
 b.superdrone_total=r.energy.population.superdrones;
 for(size_t i=0;i<2;++i)b.specialist_types[i]=(int32_t)r.energy.specialist_types[i];
 return b;
}
bool changed(const OriginalBase& a,const OriginalBase& b){
#define ACCOUNTING_FIELD(name,offset) if(a.name!=b.name)return true;
#define ACCOUNTING_SPECIALISTS(offset) for(size_t i=0;i<2;++i)if(a.specialist_types[i]!=b.specialist_types[i])return true;
#include "local_accounting_fields.inc"
#undef ACCOUNTING_FIELD
#undef ACCOUNTING_SPECIALISTS
 return false;
}
}
LocalOrderError LocalProductionSession::load_local_accounting(const uint8_t* save,size_t length,
 const uint8_t* text,size_t text_length,Variant profile,bool expansion,int32_t human,uint32_t seed,size_t memory_limit){
 if(!identity_ || revision_==UINT64_MAX)return LocalOrderError::overflow;
 if(memory_limit<sizeof(LocalAccountingTables))return LocalOrderError::no_memory;
 LocalProductionSession next;
 auto error=next.load_local(save,length,text,text_length,profile,expansion,human,seed,memory_limit-sizeof(LocalAccountingTables));
 if(error!=LocalOrderError::ok)return error;
 void* storage=malloc(sizeof(LocalAccountingTables));if(!storage)return LocalOrderError::no_memory;
 auto* tables=new(storage)LocalAccountingTables{};next.data_->accounting=tables;
 if(read_unit_definitions(text,text_length,next.data_->technology,profile,tables->units)!=Error::ok ||
    read_resource_info(text,text_length,tables->resources)!=Error::ok ||
    read_citizen_definitions(text,text_length,next.data_->technology,profile,tables->citizens)!=Error::ok)return LocalOrderError::bad_rules;
 // Only the successfully validated private data is exchanged. This session's
 // lifetime identity never changes; both successful reload and suspension
 // invalidate all prior accounting, production and movement commands.
 Data* previous=data_;data_=next.data_;next.data_=previous;++revision_;suspended_=false;
 return LocalOrderError::ok;
}
bool LocalProductionSession::accounting_enabled()const{return data_ && data_->accounting;}
LocalOrderError LocalProductionSession::prepare_accounting(size_t base_id,const WorldEnergyStage& stage,BaseAccountingPlan& output)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!data_->accounting)return LocalOrderError::unsupported;
 if(overlap(&output,sizeof(output),this,sizeof(*this)) || overlap(&output,sizeof(output),data_,sizeof(Data)) ||
    overlap(&output,sizeof(output),data_->accounting,sizeof(LocalAccountingTables)) ||
    overlap(&output,sizeof(output),&stage,sizeof(stage)) || !disjoint(data_->world,&output,sizeof(output)))return LocalOrderError::bad_state;
 const auto* base=data_->world.base(base_id);if(!base)return LocalOrderError::bad_state;
 if(base->faction_id!=data_->local_human)return LocalOrderError::not_owner;
 if((uint32_t)base->state_flags&0x10000000u)return LocalOrderError::unsupported;
 if(stage.upkeep_stage!=0 || stage.resources.intake.variant!=data_->profile ||
    stage.resources.intake.expansion_enabled!=data_->expansion_enabled)return LocalOrderError::unsupported;
 BaseAccountingPlan plan;plan.owner_identity_=identity_;plan.revision_=revision_;plan.base_=base_id;plan.stage_=stage;
 const auto& tables=*data_->accounting;
 const auto error=world_energy(data_->world,data_->technology,data_->rules,data_->world_rules,
  tables.resources,tables.units,tables.citizens,base_id,stage,plan.preview_);
 if(error!=StableIntakeError::ok)return translated(error);
 output=plan;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit_accounting(const BaseAccountingPlan& plan,BaseAccountingReceipt& output){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(overlap(&output,sizeof(output),this,sizeof(*this)) || overlap(&output,sizeof(output),data_,sizeof(Data)) ||
    overlap(&output,sizeof(output),data_->accounting,data_->accounting?sizeof(LocalAccountingTables):0) ||
    overlap(&output,sizeof(output),&plan,sizeof(plan)) || !disjoint(data_->world,&output,sizeof(output)))return LocalOrderError::bad_state;
 if(!plan.owner_identity_ || plan.owner_identity_!=identity_ || plan.revision_!=revision_)return LocalOrderError::stale_plan;
 BaseAccountingPlan verified;auto error=prepare_accounting(plan.base_,plan.stage_,verified);
 if(error!=LocalOrderError::ok)return error;
 // The plan stores no writable public fields; re-evaluate against the owned
 // current world and private immutable tables, then commit one complete patch.
 auto& base=data_->world.bases_[plan.base_];const auto after=patched(base,verified.preview_);
 const bool differs=changed(base,after);if(differs && revision_==UINT64_MAX)return LocalOrderError::overflow;
 BaseAccountingReceipt receipt{differs,differs,differs,revision_,plan.base_};
 if(differs){base=after;++revision_;receipt.revision=revision_;}
 output=receipt;return LocalOrderError::ok;
}
}
