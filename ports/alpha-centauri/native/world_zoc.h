/* SPDX-License-Identifier: GPL-3.0-or-later. Original zoc_sea/zoc_move;
 * OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Casey2013-2021. */
#ifndef AC_ALPHA_WORLD_ZOC_H
#define AC_ALPHA_WORLD_ZOC_H
#include "original_world.h"
#include "game_logic.h"
namespace alpha {
/* Read-only original ordered neighbour inspection. MultiplayerActive is an
 * explicit runtime fact, never inferred from a save or authority to move.
 * move_base_exemption reproduces zoc_move's base-bit/storedowner shortcut.
 * Output0 clear, else first blocking TILE owner+1, not necessarily actor owner.
 * Strict whole-world stack validation refuses original veh_at repair/UI paths.
 * No movement legality, retreat, events, RNG, authority or World writes. */
LogicError world_zone_of_control(const OriginalWorld&,int32_t x,int32_t y,int32_t faction,bool multiplayer_active,bool move_base_exemption,uint32_t&);
}
#endif
