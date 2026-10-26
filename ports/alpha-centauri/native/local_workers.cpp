/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
#include "local_accounting_tables.h"
#include "local_workers_snapshot.h"
#include <new>
namespace alpha {namespace {
bool overlaps(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(n>UINTPTR_MAX-x||m>UINTPTR_MAX-y)return true;return x<y+m&&y<x+n;}
bool disjoint(const OriginalWorld&w,const void*p,size_t n){return !overlaps(p,n,&w,sizeof w)&&!overlaps(p,n,w.original_bytes(),w.original_size())&&!overlaps(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile))&&!overlaps(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle))&&!overlaps(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase))&&!overlaps(p,n,w.prototype(0),512*sizeof(OriginalPrototype))&&!overlaps(p,n,w.faction(0),8*sizeof(OriginalFactionState))&&!overlaps(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
LocalOrderError translated(StableIntakeError e){switch(e){case StableIntakeError::ok:return LocalOrderError::ok;case StableIntakeError::out_of_memory:return LocalOrderError::no_memory;case StableIntakeError::overflow:return LocalOrderError::overflow;case StableIntakeError::bad_state:case StableIntakeError::bad_buffer:return LocalOrderError::bad_state;default:return LocalOrderError::unsupported;}}
OriginalBase accounted(const OriginalBase&before,const WorldEnergyResult&r){
 OriginalBase b=before;const auto&n=r.resources.food.fresh.nutrients;const auto&m=r.resources.minerals;const auto&e=r.energy.accounting;
 b.nutrients_accumulated=n.stored;b.nutrient_intake_2=n.intake;b.nutrient_consumption=n.consumption;b.nutrient_surplus=n.surplus;
 b.eco_damage=m.eco_damage;b.mineral_intake_2=m.intake;b.mineral_consumption=m.consumption;b.mineral_surplus=m.surplus;b.mineral_surplus_final=m.final_surplus;b.mineral_inefficiency=m.inefficiency;
 b.energy_intake_2=e.intake;b.energy_consumption=e.consumption;b.energy_surplus=e.surplus;b.energy_inefficiency=e.inefficiency;b.economy_total=e.economy;b.psych_total=e.psych;b.labs_total=e.labs;b.unk_total=e.unknown_total;
 b.talent_total=r.energy.population.talents;b.drone_total=r.energy.population.drones;b.superdrone_total=r.energy.population.superdrones;
 for(unsigned i=0;i<2;++i)b.specialist_types[i]=(int32_t)r.energy.specialist_types[i];
 return b;
}
bool changed(const OriginalBase&a,const OriginalBase&b){
#define ACCOUNTING_FIELD(name,offset) if(a.name!=b.name)return true;
#define ACCOUNTING_SPECIALISTS(offset) for(unsigned i=0;i<2;++i)if(a.specialist_types[i]!=b.specialist_types[i])return true;
#define WORKER_FIELD(name,offset) if(a.name!=b.name)return true;
#include "local_accounting_fields.inc"
#include "local_worker_fields.inc"
#undef ACCOUNTING_FIELD
#undef ACCOUNTING_SPECIALISTS
#undef WORKER_FIELD
 return false;
}
struct Workspace {WorkerAccountingPlan plan;OriginalWorld clone;uint8_t*bytes=nullptr;~Workspace(){free(bytes);}};
}
LocalOrderError LocalProductionSession::load_local_worker_actions(const uint8_t*save,size_t n,const uint8_t*text,size_t nt,Variant variant,bool expansion,int32_t human,uint32_t seed,size_t limit){
 if(!identity_||revision_==UINT64_MAX)return LocalOrderError::overflow;
 if(variant!=Variant::base||expansion)return LocalOrderError::unsupported;
 if(limit<sizeof(UnitDefinitions))return LocalOrderError::no_memory;
 LocalProductionSession next;const auto error=next.load_local_accounting(save,n,text,nt,variant,expansion,human,seed,limit-sizeof(UnitDefinitions));
 if(error!=LocalOrderError::ok)return error;
 void*storage=malloc(sizeof(UnitDefinitions));if(!storage)return LocalOrderError::no_memory;
 next.data_->units=new(storage)UnitDefinitions(next.data_->accounting->units);
 Data*previous=data_;data_=next.data_;next.data_=previous;++revision_;suspended_=false;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::prepare_workers(size_t id,const WorldEnergyStage&stage,WorkerAccountingPlan&output,size_t work_limit)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!data_->accounting)return LocalOrderError::unsupported;
 if(overlaps(&output,sizeof output,this,sizeof(*this))||overlaps(&output,sizeof output,data_,sizeof(Data))||overlaps(&output,sizeof output,data_->accounting,sizeof(LocalAccountingTables))||overlaps(&output,sizeof output,&stage,sizeof stage)||!disjoint(data_->world,&output,sizeof output))return LocalOrderError::bad_state;
 const auto*before=data_->world.base(id);if(!before)return LocalOrderError::bad_state;
 if(before->faction_id!=data_->local_human)return LocalOrderError::not_owner;
 if((uint32_t)before->state_flags&0x10000000u)return LocalOrderError::unsupported;
 if(stage.upkeep_stage!=0||stage.resources.intake.variant!=data_->profile||stage.resources.intake.expansion_enabled!=data_->expansion_enabled)return LocalOrderError::unsupported;
 const size_t n=data_->world.original_size(),memory=data_->world.memory_bytes();
 if(work_limit<sizeof(Workspace)||n>work_limit-sizeof(Workspace)||memory>work_limit-sizeof(Workspace)-n)return LocalOrderError::no_memory;
 auto*work=static_cast<Workspace*>(malloc(sizeof(Workspace)));if(!work)return LocalOrderError::no_memory;
 new(work)Workspace{};
 auto evaluate=[&]()->LocalOrderError {
  auto&p=work->plan;const auto&tables=*data_->accounting;
  p.owner_identity_=identity_;p.revision_=revision_;p.base_=id;p.stage_=stage;p.work_limit_=work_limit;
  auto error=world_human_allocation(data_->world,data_->technology,data_->rules,tables.resources,tables.units,tables.citizens,id,stage.resources.intake,p.allocation_);
  if(error!=StableIntakeError::ok)return translated(error);
  work->bytes=static_cast<uint8_t*>(malloc(n));if(!work->bytes)return LocalOrderError::no_memory;
  LocalSnapshotReceipt snapshot{};const auto encoded=encode_local_workers_snapshot(data_->world,work->bytes,n,snapshot);
  if(encoded==LocalSnapshotError::out_of_memory)return LocalOrderError::no_memory;
  if(encoded!=LocalSnapshotError::ok)return LocalOrderError::bad_world;
  const auto loaded=work->clone.load(work->bytes,n,snapshot.runtime_seed,memory);
  if(loaded==WorldLoadError::out_of_memory||loaded==WorldLoadError::memory_limit)return LocalOrderError::no_memory;
  if(loaded!=WorldLoadError::ok)return LocalOrderError::bad_world;
  auto&base=work->clone.bases_[id];const auto&a=p.allocation_.allocation;const auto&intake=p.allocation_.intake;
  base.worked_tiles=(int32_t)a.worked;base.specialist_total=a.specialists;base.specialist_adjust=a.adjustment;
  base.state_flags=(int32_t)intake.state_flags;for(unsigned i=0;i<2;++i)base.specialist_types[i]=(int32_t)a.specialist_types[i];
  base.nutrient_intake=base.nutrient_intake_2=intake.intake[0];base.mineral_intake=base.mineral_intake_2=intake.intake[1];base.energy_intake=base.energy_intake_2=intake.intake[2];base.unused_intake=base.unused_intake_2=0;
  // Commerce deliberately observes these fresh FIRST intake fields, while all
  // other bases retain their actual current values at this explicit phase.
  error=world_energy(work->clone,data_->technology,data_->rules,data_->world_rules,tables.resources,tables.units,tables.citizens,id,stage,p.accounting_);
  if(error!=StableIntakeError::ok)return translated(error);
  p.after_=accounted(base,p.accounting_);output=p;return LocalOrderError::ok;
 };
 const auto error=evaluate();work->~Workspace();free(work);return error;
}
LocalOrderError LocalProductionSession::commit_workers(const WorkerAccountingPlan&plan,WorkerAccountingReceipt&output){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(overlaps(&output,sizeof output,this,sizeof(*this))||overlaps(&output,sizeof output,data_,sizeof(Data))||overlaps(&output,sizeof output,data_->accounting,data_->accounting?sizeof(LocalAccountingTables):0)||overlaps(&output,sizeof output,&plan,sizeof plan)||!disjoint(data_->world,&output,sizeof output))return LocalOrderError::bad_state;
 if(!plan.owner_identity_||plan.owner_identity_!=identity_||plan.revision_!=revision_)return LocalOrderError::stale_plan;
 void*storage=malloc(sizeof(WorkerAccountingPlan));if(!storage)return LocalOrderError::no_memory;
 auto*verified=new(storage)WorkerAccountingPlan{};
 const auto error=prepare_workers(plan.base_,plan.stage_,*verified,plan.work_limit_);
 if(error!=LocalOrderError::ok){verified->~WorkerAccountingPlan();free(storage);return error;}
 auto&base=data_->world.bases_[plan.base_];const bool differs=changed(base,verified->after_);
 if(differs&&revision_==UINT64_MAX){verified->~WorkerAccountingPlan();free(storage);return LocalOrderError::overflow;}
 WorkerAccountingReceipt receipt{differs,differs,differs,revision_,plan.base_};
 if(differs){base=verified->after_;++revision_;receipt.revision=revision_;}
 verified->~WorkerAccountingPlan();free(storage);output=receipt;return LocalOrderError::ok;
}
}
