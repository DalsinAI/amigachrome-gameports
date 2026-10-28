/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for functions WP8 calls that other packages own and that are
 * not declared in engine_wp.h. Each is replaced by the owning package's strong
 * definition as soon as that package is linked in (same pattern as WP2's
 * base_deps.cpp and WP3's wp3_deps.cpp). Until then they raise
 * EV_NOT_IMPLEMENTED and return a neutral value.
 * Signatures are Thinker Mod's (dabe62f2, MIT; see engine/THINKER-LICENSE).
 */
#include "engine.h"

#define WP8_WEAK __attribute__((weak))

namespace alpha {
namespace engine {

namespace {
int wp8_commerce_import[MaxPlayerNum] = {};
} // namespace

/* WP2: per-faction commerce of the base last computed by base_compute. */
WP8_WEAK int* BaseCommerceImport = wp8_commerce_import;

/* WP2 monument.cpp */
WP8_WEAK void __cdecl mon_winning_unify(int UNUSED(faction_id), int UNUSED(winner_id)) {
    not_implemented("mon_winning_unify");
}

WP8_WEAK void __cdecl mon_killed_faction(int UNUSED(faction_id), int UNUSED(killed_id)) {
    not_implemented("mon_killed_faction");
}

/* WP3 path.cpp: do two factions share a continent (region 0 = any)? */
WP8_WEAK int __cdecl contiguous(int UNUSED(faction_id_1), int UNUSED(faction_id_2),
int UNUSED(region), int UNUSED(flag)) {
    not_implemented("contiguous");
    return 0;
}

/* WP5 (exe-only): note the rival's prototypes for the diplomacy dialog. */
WP8_WEAK void __cdecl scan_prototypes(int UNUSED(faction_id), int UNUSED(faction_id_2)) {
    not_implemented("scan_prototypes");
}

} // namespace engine
} // namespace alpha
