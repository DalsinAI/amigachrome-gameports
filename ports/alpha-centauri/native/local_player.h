/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_PLAYER_H
#define ALPHA_LOCAL_PLAYER_H
#include "local_production_session.h"
#include "local_actions_snapshot.h"
namespace alpha {
// UI adapter for the existing bounded, base-game local action controller.
// Checkpoints are explicitly our format, not original-engine compatible saves.
class LocalPlayer {
 public:
  LocalOrderError load(const uint8_t*,size_t,const uint8_t*,size_t,uint32_t seed);
  LocalOrderError resume(const uint8_t*,size_t,const uint8_t*,size_t);
  void close(){session_.close();pending_=false;dirty_=false;}
  const OriginalWorld* world()const{return session_.world();}
  bool dirty()const{return dirty_;}
  LocalOrderError step(size_t vehicle,int32_t dx,int32_t dy,LocalStepReceipt&);
  LocalOrderError next_facility(int32_t base);
  LocalOrderError confirm_facility(FacilityOrderReceipt&);
  void cancel_facility(){pending_=false;}
  const FacilityOrderPreview* proposal()const{return pending_?&plan_.preview():nullptr;}
  size_t checkpoint_size()const;
  LocalSnapshotError checkpoint(void*,size_t)const;
  // Call only after the complete checkpoint has been written and closed.
  void saved(){dirty_=false;}
 private:
  LocalProductionSession session_;
  FacilityOrderPlan plan_;
  bool pending_=false,dirty_=false;
  uint32_t rules_crc_=0;
};
}
#endif
