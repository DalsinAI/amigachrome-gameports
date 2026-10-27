/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_ORIGINAL_WORLD_H
#define AC_ALPHA_ORIGINAL_WORLD_H
#include "original_faction.h"
namespace alpha {
enum class WorldLoadError { ok,bad_format,memory_limit,out_of_memory,bad_reference };
/* Owns the original container AND decoded native records. No caller-buffer or
 * serialized PC pointer survives import. Opaque blocks stay in owned bytes for
 * later interpretation, not fabricated initialization. Version IDs remain raw.
 * A caller-supplied world RNG seed is explicit: ordinary save tick/map seeds
 * are not silently substituted for the missing runtime random state.
 * Read-only import foundation, no turn/AI/actions or save-writer claim.
 */
class OriginalWorld {
 public:
  OriginalWorld()=default;
  ~OriginalWorld(){clear();}
  OriginalWorld(const OriginalWorld&)=delete;
  OriginalWorld& operator=(const OriginalWorld&)=delete;
  WorldLoadError load(const uint8_t* bytes,size_t length,uint32_t runtime_seed,
                      size_t memory_limit=64u*1024u*1024u);
  void clear();
  bool loaded()const{return raw_!=nullptr;}
  const OriginalGameView& game()const{return view_;}
  const uint8_t* original_bytes()const{return raw_;}
  size_t original_size()const{return length_;}
  size_t memory_bytes()const{return memory_;}
  uint32_t runtime_seed()const{return random_seed_;}
  const OriginalMapTile* tile(size_t i)const{return loaded() && i<view_.map.info.tile_count?tiles_+i:nullptr;}
  const OriginalVehicle* vehicle(size_t i)const{return loaded() && i<view_.vehicle_count?vehicles_+i:nullptr;}
  const OriginalBase* base(size_t i)const{return loaded() && i<view_.base_count?bases_+i:nullptr;}
  const OriginalPrototype* prototype(size_t i)const{return loaded() && i<512?prototypes_+i:nullptr;}
  const OriginalFactionState* faction(size_t i)const{return loaded() && i<8?factions_+i:nullptr;}
  const OriginalFactionDefinition* faction_definition(size_t i)const{return loaded() && i<8?definitions_+i:nullptr;}
 private:
  friend class LocalProductionSession;
  friend class WorldActions;
  friend class FacilityProductionStage;
  friend class HumanUpkeepTransaction;
  void swap(OriginalWorld& other);
  uint8_t* raw_=nullptr;
  OriginalMapTile* tiles_=nullptr;
  OriginalVehicle* vehicles_=nullptr;
  OriginalBase* bases_=nullptr;
  OriginalPrototype* prototypes_=nullptr;
  OriginalFactionState* factions_=nullptr;
  OriginalFactionDefinition* definitions_=nullptr;
  OriginalGameView view_{};
  size_t length_=0,memory_=0;
  uint32_t random_seed_=0;
};
const char* world_load_error_name(WorldLoadError error);
}
#endif
