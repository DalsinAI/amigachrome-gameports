/* SPDX-License-Identifier: GPL-3.0-or-later
 * Weak fallbacks for functions WP8 calls that other packages own and that are
 * not declared in engine_wp.h. Each is replaced by the owning package's strong
 * definition as soon as that package is linked in (same pattern as WP2's
 * base_deps.cpp and WP3's wp3_deps.cpp). Until then they raise
 * EV_NOT_IMPLEMENTED and return a neutral value.
 * Signatures are Thinker Mod's (dabe62f2, MIT; see engine/THINKER-LICENSE).
 * Integration: BaseCommerceImport, mon_winning_unify, mon_killed_faction
 * (WP2) and scan_prototypes (WP5) have landed; their fallbacks were removed.
 */
#include "engine.h"

#define WP8_WEAK __attribute__((weak))

namespace alpha {
namespace engine {

/* WP3 path.cpp: do two factions share a continent (region 0 = any)? */
WP8_WEAK int __cdecl contiguous(int UNUSED(faction_id_1), int UNUSED(faction_id_2),
int UNUSED(region), int UNUSED(flag)) {
    not_implemented("contiguous");
    return 0;
}

} // namespace engine
} // namespace alpha
