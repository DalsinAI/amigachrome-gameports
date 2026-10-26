/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_ARTIFACT_CAPTURE_H
#define AC_ALPHA_ARTIFACT_CAPTURE_H
#include "world_casualty.h"
#include "rules_text.h"
namespace alpha {
struct ArtifactCapturePreview {size_t actor,recipient;uint8_t old_faction,new_faction;uint32_t speed;};
/* Original battle_kill early capture0 route. Both chassis must be LAND and the
 * destination tile land (altitude>=96); water/stack_check8 held. Validated
 * matching unit/technology tables and road rate are supplied explicitly.
 * Owns current records, copied caller pools and runtime. No deletion/count
 * decrement, interlude, reward income, session commit, save or player attack.
 */
class ArtifactCapturePlan {
 public:
  bool ready()const{return storage_.ready();}
  const OriginalWorld*frame()const{return storage_.frame();}
  const RemovalRuntime*runtime()const{return storage_.runtime();}
  const OriginalVehicle*before()const{return storage_.removed();}
  const CasualtyInputs*accumulators()const{return ready()?&inputs_:nullptr;}
  const ArtifactCapturePreview*preview()const{return ready()?&preview_:nullptr;}
 private:
  friend RemovalError stage_artifact_capture(const OriginalWorld&,size_t,const RemovalRuntime&,const UnitDefinitions&,const Rules&,int32_t,const CasualtyInputs&,ArtifactCapturePlan&,size_t);
  RemovalError prepare(const OriginalWorld&,size_t,const RemovalRuntime&,const UnitDefinitions&,const Rules&,int32_t,const CasualtyInputs&,size_t);
  void swap(ArtifactCapturePlan&);
  VehicleRemovalPlan storage_;
  CasualtyInputs inputs_{};
  ArtifactCapturePreview preview_{};
};
RemovalError stage_artifact_capture(const OriginalWorld&,size_t victim,const RemovalRuntime&,const UnitDefinitions&,const Rules&,int32_t road_rate,const CasualtyInputs&,ArtifactCapturePlan&,size_t memory_limit=64u*1024u*1024u);
}
#endif
