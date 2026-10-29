/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_ACCOUNTING_H
#define ALPHA_LOCAL_ACCOUNTING_H
#include "world_energy.h"
namespace alpha {
class LocalProductionSession;
/* A current first-intake snapshot refresh, not a turn stage. Only upkeep0 is
 * accepted: faction totals/projection arrays cannot be incremented by refresh.
 * First intake fields, worker assignment/flags, actors, mineral stockpile, production
 * baseline and faction technology caches remain separate phase inputs.
 * All tables are privately parsed from the same rules text by the session.
 * The current frontend never instantiates this capability. */
class BaseAccountingPlan {
 public:
  size_t base()const{return base_;}
  const WorldEnergyResult& preview()const{return preview_;}
 private:
  friend class LocalProductionSession;
  uint64_t owner_identity_=0,revision_=0;
  size_t base_=0;
  WorldEnergyStage stage_{};
  WorldEnergyResult preview_{};
};
struct BaseAccountingReceipt {bool changed,repaint_required,persistence_required;uint64_t revision;size_t base;};
}
#endif
