/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_WORKERS_H
#define ALPHA_LOCAL_WORKERS_H
#include "world_human_allocation.h"
#include "world_energy.h"
namespace alpha {
class LocalProductionSession;
/* Private session/revision authority. Proposals combine actual worker selection
 * with the consequent stage0 resource/energy accounting on a private clone of
 * CURRENT owned state, including current movements and production orders.
 * The first intake fields are set before commerce; no saved intake feedback.
 * This does not advance a turn or enable frontend/player-save capability. */
class WorkerAccountingPlan {
 public:
  size_t base()const{return base_;}
  const WorldHumanAllocationResult& allocation()const{return allocation_;}
  const WorldEnergyResult& accounting()const{return accounting_;}
 private:
  friend class LocalProductionSession;
  uint64_t owner_identity_=0,revision_=0;
  size_t base_=0,work_limit_=0;
  WorldEnergyStage stage_{};
  WorldHumanAllocationResult allocation_{};
  WorldEnergyResult accounting_{};
  OriginalBase after_{};
};
struct WorkerAccountingReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;size_t base;};
}
#endif
