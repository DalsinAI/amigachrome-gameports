/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_FACILITY_UPKEEP_SESSION_H
#define ALPHA_FACILITY_UPKEEP_SESSION_H
#include "facility_production_stage.h"
#include "facility_production_snapshot.h"
#include "world_stable_intake.h"
namespace alpha {
enum class UpkeepSessionError {ok,bad_state,bad_rules,bad_world,bad_checkpoint,no_memory};
/* Owned, engine-internal construction segment. Each accepted base can run once,
 * including after checkpoint/resume. Completed bases retain mandatory resource
 * recomputation/production-choice events. Neither event has a public clear API.
 * This is not a complete base upkeep, persistent turn scheduler, EndTurn action
 * or original Windows save format. Starting a segment is an explicit engine
 * boundary; never use start() instead of resume() to continue a saved segment.
 * Checksums detect corruption, not hostile edits or intentional save rollback.
 */
class FacilityUpkeepSession {
 public:
  FacilityUpkeepSession()=default;
  ~FacilityUpkeepSession(){close();}
  FacilityUpkeepSession(const FacilityUpkeepSession&)=delete;
  FacilityUpkeepSession& operator=(const FacilityUpkeepSession&)=delete;
  UpkeepSessionError start(const uint8_t* save,size_t length,const uint8_t* rules,size_t rules_length,
   uint32_t runtime_seed,size_t memory_limit=64u*1024u*1024u);
  UpkeepSessionError resume(const uint8_t* checkpoint,size_t length,const uint8_t* rules,size_t rules_length,
   size_t memory_limit=64u*1024u*1024u);
  void close();
  const OriginalWorld* world()const;
  bool processed(size_t base)const;
  bool recompute_pending(size_t base)const;
  bool production_choice_pending(size_t base)const;
  FacilityProductionError apply_fresh_materials(size_t base,const StableIntakeStage&,FacilityStageReceipt&);
  size_t checkpoint_size()const;
  LocalSnapshotError checkpoint(void* output,size_t capacity)const;
 private:
  struct Data;
  Data* data_=nullptr;
  static UpkeepSessionError prepare(const uint8_t*,size_t,const uint8_t*,size_t,uint32_t,size_t,Data*&);
};
}
#endif
