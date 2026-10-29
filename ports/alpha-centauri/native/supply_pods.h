/* SPDX-License-Identifier: GPL-3.0-or-later. Original goody_at;
 * OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Casey2013-2021. */
#ifndef AC_ALPHA_SUPPLY_PODS_H
#define AC_ALPHA_SUPPLY_PODS_H
#include "original_map.h"
#include "game_logic.h"
namespace alpha {
/* Original ordinary EXE initialized coefficients are61,11 in both editions.
 * Explicit runtime data, not state initialization or movement/combat RNG. */
struct PodCoefficients {int32_t difference,average;};
/* Return original0none/1ordinary/2special. No pod opening/effects/RNG/authority. */
LogicError tile_supply_pod(const OriginalMapInfo&,const OriginalMapTile&,int32_t x,int32_t y,uint32_t game_rules,const PodCoefficients&,uint32_t&);
}
#endif
