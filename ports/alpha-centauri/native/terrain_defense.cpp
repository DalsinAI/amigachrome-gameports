/* SPDX-License-Identifier: GPL-3.0-or-later. See terrain_defense.h. */
#include "terrain_defense.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError terrain_defense(const TerrainDefenseContext&c,TerrainDefense&out){
 if(c.altitude_byte>255||c.roughness_byte>255||c.owner_nibble>15||c.defender_faction<0||c.defender_faction>7||c.defender_prototype<0||c.defender_prototype>=512||c.defender_weapon< -128||c.defender_weapon>127||c.defender_triad<0||c.defender_triad>2||c.pholus_owner< -1||c.pholus_owner>7||
 (c.attacker_present&&(c.attacker_prototype<0||c.attacker_prototype>=512||c.attacker_weapon< -128||c.attacker_weapon>127||c.attacker_triad<0||c.attacker_triad>2||c.attacker_faction<0||c.attacker_faction>7))||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 const uint32_t altitude=c.altitude_byte&0xe0u;
 const bool fungus=(c.items&0x20u)&&altitude>=0x40u;
 const bool attacker_native=c.attacker_present&&c.attacker_prototype<64&&c.attacker_weapon<0;
 const bool attacker_pholus=c.attacker_present&&c.pholus_owner==c.attacker_faction;
 TerrainDefense result{2,TerrainDefenseLabel::unchanged};
 if(altitude<0x60u||((c.items&1u)&&c.owner_nibble<8)||(fungus&&c.attacker_present&&(c.attacker_faction==0||attacker_pholus||attacker_native))){out=result;return LogicError::ok;}
 uint32_t bonus=(c.roughness_byte&0xc0u)>0x40u?1u:0u;
 result.label=TerrainDefenseLabel::rocky;
 if(fungus&&!bonus&&c.defender_triad!=2&&!(c.attacker_present&&(attacker_native||attacker_pholus))){
  bonus=(c.pholus_owner==c.defender_faction||(c.defender_prototype<64&&c.defender_weapon<0))?2u:1u;
  result.label=TerrainDefenseLabel::fungus;
 }
 if((c.items&0x200000u)&&!bonus&&(!c.attacker_present||c.attacker_triad==0)){bonus=1;result.label=TerrainDefenseLabel::forest;}
 result.multiplier=bonus+2;out=result;return LogicError::ok;
}
}
