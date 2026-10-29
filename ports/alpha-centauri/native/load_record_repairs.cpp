/* SPDX-License-Identifier: GPL-3.0-or-later.
 * Original load_daemon/rebuild_* semantics; Thinker savegame.cpp/map.cpp,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af. See LOAD-REPAIRS.md.
 */
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
WorldLoadError OriginalWorld::load_with_record_repairs(const uint8_t* bytes,size_t length,
 uint32_t seed,const Rules& rules,size_t limit){
 if(rules.count!=technology_count)return WorldLoadError::bad_format;
 for(size_t i=0;i<technology_count;++i)for(unsigned j=0;j<2;++j)
  if(rules.technologies[i].prerequisites[j]<-2 || rules.technologies[i].prerequisites[j]>=89)
   return WorldLoadError::bad_format;
 OriginalWorld next;const auto result=next.load(bytes,length,seed,limit);
 if(result!=WorldLoadError::ok)return result;
 auto& g=next.view_;
 // Original loader leaves Planet's faction record and alive bit0 untouched.
 uint8_t alive=g.preferences.FactionStatus[1]&1u;
 for(unsigned f=1;f<8;++f){
  auto& state=next.factions_[f];state.player_flags&=~0x200u;
  if(state.satellites_nutrient || state.satellites_mineral || state.satellites_energy || state.satellites_ODP)
   state.player_flags|=0x200u;
  // Actual has_tech excludes future88 and retains None/Disable asymmetry.
  for(size_t i=0;i<88;++i){const auto& t=rules.technologies[i];
   if((t.flags&8u) && t.prerequisites[0]!=-2 &&
      !(t.prerequisites[1]==-2 && t.prerequisites[0]!=-1) &&
      (g.preferences.TechOwners[i]&(1u<<f)))state.player_flags|=0x200u;
  }
  bool present=false;
  for(size_t i=0;i<g.base_count && !present;++i)present=next.bases_[i].faction_id==(int32_t)f;
  for(size_t i=0;i<g.vehicle_count && !present;++i){const auto& v=next.vehicles_[i];
   present=v.faction_id==f && next.prototypes_[(size_t)v.unit_id].plan==8;
  }
  if(present)alive|=(uint8_t)(1u<<f);
 }
 g.preferences.FactionStatus[1]=alive;
 const size_t row=g.map.info.width/2;
 // Linear equivalent of original tile-first/record-ascending scans: reverse
 // application gives the lowest record index final priority. Vehicle repair
 // uses the OLD base bit; base repair follows it, preserving that source quirk.
 for(size_t i=0;i<g.map.info.tile_count;++i)next.tiles_[i].items&=~2u;
 for(size_t i=g.vehicle_count;i;--i){const auto& v=next.vehicles_[i-1];
  auto& tile=next.tiles_[(size_t)v.y*row+(size_t)v.x/2];tile.items|=2u;
  if(!(tile.items&1u))tile.val2=(uint8_t)((tile.val2&0xf0u)|(v.faction_id&15u));
 }
 for(size_t i=0;i<g.map.info.tile_count;++i)next.tiles_[i].items&=~1u;
 for(size_t i=g.base_count;i;--i){const auto& b=next.bases_[i-1];
  auto& tile=next.tiles_[(size_t)b.y*row+(size_t)b.x/2];tile.items|=1u;
  tile.val2=(uint8_t)((tile.val2&0xf0u)|((uint32_t)b.faction_id&15u));
 }
 swap(next);return WorldLoadError::ok;
}
}
