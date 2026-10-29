/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_VEHICLE_REMOVAL_H
#define AC_ALPHA_WORLD_VEHICLE_REMOVAL_H
#include "original_world.h"
#include "vehicle_removal.h"
namespace alpha {
/* Owns a current decoded world frame and removal runtime references together.
 * Successful staging updates only this frame: current count and VehCount agree,
 * arrays/counters/links follow the checked leaf. Original wire bytes/spans stay
 * immutable baseline, not a serialization of the staged current records.
 * No caller/session commit, battle_kill rewards, full kill lifecycle, persistence
 * or legal attack. Offline/log-disabled/invisible-window observations required.
 */
class VehicleRemovalPlan {
 public:
  bool ready()const{return ready_;}
  const OriginalWorld*frame()const{return ready_?&frame_:nullptr;}
  const RemovalRuntime*runtime()const{return ready_?&runtime_:nullptr;}
  const OriginalVehicle*removed()const{return ready_?&removed_:nullptr;}
 private:
  friend class CasualtyPlan;
  friend class ArtifactCapturePlan;
  friend RemovalError stage_vehicle_removal(const OriginalWorld&,size_t,const RemovalRuntime&,VehicleRemovalPlan&,size_t);
  RemovalError capture(const OriginalWorld&,size_t);
  RemovalError apply(size_t,const RemovalRuntime&);
  void swap(VehicleRemovalPlan&);
  OriginalWorld frame_;
  uint8_t history_[4096]{};
  RemovalRuntime runtime_{};
  OriginalVehicle removed_{};
  bool ready_=false;
};
RemovalError stage_vehicle_removal(const OriginalWorld&,size_t victim,const RemovalRuntime&,VehicleRemovalPlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
