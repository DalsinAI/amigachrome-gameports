/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_PRODUCTION_SESSION_H
#define ALPHA_LOCAL_PRODUCTION_SESSION_H
#include "world_facility_options.h"
#include "world_production_retool.h"
#include "world_actions.h"
#include "local_accounting.h"
#include "local_workers.h"
#include "local_unit_skip.h"
#include "world_unit_readiness.h"
namespace alpha {
class LocalProductionSession;
enum class LocalOrderError {ok,bad_state,bad_rules,bad_world,no_memory,not_human,not_owner,unavailable,
 unsupported,stale_plan,suspended,overflow};
struct FacilityOrderPreview {
 int32_t base,facility,previous_item,proposed_item,mineral_before,mineral_baseline,mineral_loss,mineral_after;
 bool changes_state;
};
/* A plan is tied to the issuing session's lifetime and revision. Copying is
 * allowed; fields that authorize a commit are private. No user-supplied token
 * establishes live original-engine/network/modal authority. */
class FacilityOrderPlan {
 public:
  const FacilityOrderPreview& preview()const{return preview_;}
 private:
  friend class LocalProductionSession;
  uint64_t owner_identity_=0;
  uint64_t revision_=0;
  FacilityOrderPreview preview_{};
};
struct LocalStepIntent {size_t vehicle;int32_t x,y;};
/* A bound command intent, not an eligibility/price preview. The complete
 * original-world subset gates are evaluated transactionally by commit. */
class LocalStepCommand {
 public:
  const LocalStepIntent& intent()const{return intent_;}
 private:
  friend class LocalProductionSession;
  uint64_t owner_identity_=0,revision_=0;
  LocalStepIntent intent_{};
};
struct LocalStepReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;StepResult result;};
struct FacilityOrderReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;FacilityOrderPreview applied;};
/* Explicit new offline-local planning controller. Owns its imported world and
 * parsed rules; exposes const inspection only. The current frontend does not
 * instantiate it. One owning engine thread is required. This is not the
 * original turn/AI/network/modal runtime. A separate explicit base-only import
 * enables the bounded clear-land action under the same revision authority.
 * Raw imports require saved map owner and active faction to match the human.
 * The explicit repaired resume entry instead derives and selects MapWin owner.
 * Caller explicitly selects the saved current human faction and runtime
 * edition. Orders are limited to a local owned base with a single queue slot.
 * No unit-production eligibility, completion, upkeep, turn or UI implementation.
 * Decoded records mutate atomically; the owned original wire remains baseline.
 * Existing movement-only encoding rejects these base/faction changes; production
 * persistence must be qualified separately before exposing a player save.
 */
class LocalProductionSession {
 public:
  LocalProductionSession();
  ~LocalProductionSession(){close();}
  LocalProductionSession(const LocalProductionSession&)=delete;
  LocalProductionSession& operator=(const LocalProductionSession&)=delete;
  LocalOrderError load_local(const uint8_t* save,size_t save_length,const uint8_t* rules,size_t rules_length,
   Variant profile,bool expansion_enabled,int32_t local_human,uint32_t runtime_seed,
   size_t memory_limit=64u*1024u*1024u);
  // Explicit base-game action-enabled import. A production-only import never
  // gains movement capability from a separate caller-supplied rules object.
  LocalOrderError load_local_land_actions(const uint8_t* save,size_t save_length,const uint8_t* rules,size_t rules_length,
   Variant profile,bool expansion_enabled,int32_t local_human,uint32_t runtime_seed,
   size_t memory_limit=64u*1024u*1024u);
  // Original record repairs + offline single-human resume to MapWin owner.
  // Derives the owner from the file; rejects multiple humans/dead owner.
  // base_actions enables only the existing base-game bounded commands.
  // The owned immutable wire becomes the repaired, owner-selected command
  // baseline; external input stays unchanged. No phase/end-turn authority.
  // memory_limit bounds each stage plus its live private staging allocations,
  // excluding the prior session, input, and inherited validator scratch.
  LocalOrderError load_resumed_commands(const uint8_t*,size_t,const uint8_t*,size_t,
   Variant,bool expansion_enabled,bool base_actions,uint32_t runtime_seed,
   size_t memory_limit=64u*1024u*1024u);
  bool land_actions_enabled()const;
  // Explicit table-owning, offline accounting capability. No movement is
  // enabled by this import. Failure preserves the previous session entirely.
  LocalOrderError load_local_accounting(const uint8_t*,size_t,const uint8_t*,size_t,
   Variant,bool,int32_t,uint32_t,size_t memory_limit=64u*1024u*1024u);
  bool accounting_enabled()const;
  // Explicit combined base-game import, owning both same-input action and
  // accounting tables. Existing accounting-only imports never gain movement.
  LocalOrderError load_local_worker_actions(const uint8_t*,size_t,const uint8_t*,size_t,
   Variant,bool,int32_t,uint32_t,size_t memory_limit=64u*1024u*1024u);
  LocalOrderError prepare_accounting(size_t,const WorldEnergyStage&,BaseAccountingPlan&)const;
  LocalOrderError commit_accounting(const BaseAccountingPlan&,BaseAccountingReceipt&);
  // Offline accounting imports also own the tables for worker allocation.
  // work_limit bounds current wire + decoded clone + private plan workspace;
  // called yield/accounting components retain their own bounded scratch.
  LocalOrderError prepare_workers(size_t,const WorldEnergyStage&,WorkerAccountingPlan&,
   size_t work_limit=64u*1024u*1024u)const;
  LocalOrderError commit_workers(const WorkerAccountingPlan&,WorkerAccountingReceipt&);
  // Preparation binds an intent to this session lifetime/revision; it does
  // not certify a legal move or quote a price. LocalOrderError::ok from commit
  // means evaluated; StepResult may explicitly reject the move without writes.
  LocalOrderError prepare_clear_land_step(size_t vehicle,int32_t x,int32_t y,LocalStepCommand&)const;
  LocalOrderError commit_clear_land_step(const LocalStepCommand&,LocalStepReceipt&);
  LocalOrderError unit_readiness(size_t,UnitReadiness&)const;
  LocalOrderError prepare_skip_unit(size_t,LocalSkipCommand&)const;
  LocalOrderError commit_skip_unit(const LocalSkipCommand&,LocalSkipReceipt&);
  void close();
  const OriginalWorld* world()const;
  uint64_t revision()const{return revision_;}
  int32_t local_human()const;
  bool suspended()const{return suspended_;}
  LocalOrderError set_suspended(bool value);
  LocalOrderError prepare_facility(int32_t base,int32_t facility,FacilityOrderPlan& output)const;
  LocalOrderError commit(const FacilityOrderPlan&,FacilityOrderReceipt& output);
 private:
  LocalOrderError load_impl(const uint8_t*,size_t,const uint8_t*,size_t,Variant,bool,int32_t,uint32_t,size_t,bool);
  struct Data;
  Data* data_=nullptr;
  uint64_t identity_=0,revision_=0;
  bool suspended_=false;
};
const char* local_order_error_name(LocalOrderError);
}
#endif
