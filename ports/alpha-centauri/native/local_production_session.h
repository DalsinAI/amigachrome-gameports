/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_PRODUCTION_SESSION_H
#define ALPHA_LOCAL_PRODUCTION_SESSION_H
#include "world_facility_options.h"
#include "world_production_retool.h"
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
struct FacilityOrderReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;FacilityOrderPreview applied;};
/* Explicit new offline-local planning controller. Owns its imported world and
 * parsed rules; exposes const inspection only. The current frontend does not
 * instantiate it. One owning engine thread is required. This is not the
 * original turn/AI/network/modal runtime.
 * Both saved map owner and active turn faction must match the explicit human.
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
  void close();
  const OriginalWorld* world()const;
  uint64_t revision()const{return revision_;}
  int32_t local_human()const;
  bool suspended()const{return suspended_;}
  LocalOrderError set_suspended(bool value);
  LocalOrderError prepare_facility(int32_t base,int32_t facility,FacilityOrderPlan& output)const;
  LocalOrderError commit(const FacilityOrderPlan&,FacilityOrderReceipt& output);
 private:
  struct Data;
  Data* data_=nullptr;
  uint64_t identity_=0,revision_=0;
  bool suspended_=false;
};
const char* local_order_error_name(LocalOrderError);
}
#endif
