/* SPDX-License-Identifier: GPL-3.0-or-later. See facility_production.h for policy attribution. */
#ifndef ALPHA_FACILITY_PRODUCTION_STAGE_H
#define ALPHA_FACILITY_PRODUCTION_STAGE_H
#include "facility_production.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
struct FreshFacilityMinerals {int32_t intake,surplus,cost_factor;};
struct FacilityStageReceipt {
 int32_t base,facility,before;
 FacilityProductionResult production;
 bool recompute_required,production_choice_required;
};
/* Engine-internal, single-threaded restoration stage; never a public EndTurn.
 * Borrows the owned world and qualified matching rules for their full lifetime.
 * The caller supplies freshly computed mineral intake/surplus and the current
 * cost factor. Saved displayed totals do not qualify those facts. Existing
 * human/owner/availability gates remain mandatory. Only ordinary Creche2 and
 * Recycling3, single-slot queues and no governor are supported. Completion
 * leaves the original completed queue item awaiting a real player choice and
 * emits explicit recomputation/choice events, not fabricated allocation/UI.
 * Each base is accepted at most once per object. This ledger is NOT persistent
 * turn/replay authority: constructing another stage can apply another stage.
 * Do not expose this operation directly through LocalPlayer until a persistent
 * stage scheduler and fresh support/resource inputs are connected.
 */
class FacilityProductionStage {
 public:
  FacilityProductionStage(OriginalWorld&,const Rules&,const BasicRules&,const FacilityDefinitions&);
  FacilityProductionStage(const FacilityProductionStage&)=delete;
  FacilityProductionStage& operator=(const FacilityProductionStage&)=delete;
  FacilityProductionError apply(size_t base,const FreshFacilityMinerals&,FacilityStageReceipt&);
 private:
  OriginalWorld& world_;
  const Rules& technologies_;
  const BasicRules& rules_;
  const FacilityDefinitions& facilities_;
  const uint8_t* source_;
  size_t source_size_,base_count_;
  bool processed_[512]{};
};
}
#endif
