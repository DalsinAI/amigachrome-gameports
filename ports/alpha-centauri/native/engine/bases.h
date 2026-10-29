/* SPDX-License-Identifier: GPL-3.0-or-later
 * Base bookkeeping (Thinker Mod src/base.h subset, MIT; see
 * engine/THINKER-LICENSE). Production, growth, psych and upkeep are WP2.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

int __cdecl mod_base_init(int faction_id, int x, int y);
void __cdecl mod_base_kill(int base_id);
inline int base_init(int faction_id, int x, int y) { return mod_base_init(faction_id, x, y); }
inline void base_kill(int base_id) { mod_base_kill(base_id); }
void __cdecl set_base(int base_id);
void __cdecl base_mark(int base_id);
bool has_facility(FacilityId item_id, int base_id);
bool has_free_facility(FacilityId item_id, int faction_id);
int __cdecl redundant(FacilityId item_id, int faction_id);
int __cdecl has_fac(FacilityId item_id, int base_id, int queue_count);
int __cdecl has_fac_built(FacilityId item_id, int base_id);
void __cdecl set_fac(FacilityId item_id, int base_id, bool add);
int __cdecl replay_base(int event, int x, int y, int faction_id);

} // namespace engine
} // namespace alpha
