/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FACILITY_OPTIONS_H
#define ALPHA_WORLD_FACILITY_OPTIONS_H
#include "facility_availability.h"
#include "original_world.h"
namespace alpha {
struct WorldFacilityOptions { uint32_t count;uint8_t available[134];bool adjacent_ocean; };
/* Read-only saved-world binding, not active production/session authority.
 * Explicit runtime edition/expansion flag; legacy-to-expansion fixups are not
 * invented. base=-1 is the original global predicate; faction1..7 and caller
 * queue_count0..10 are explicit, not derived from saved queue_size. Cross-owner
 * base inspection is allowed, as in the original predicate; it grants no order.
 * Unsupported non-ASCII SMAX faction filenames reject instead of guessing the
 * Win32 locale's CharUpperA behavior. Output and world remain unchanged on error.
 */
LogicError world_facility_options(const OriginalWorld&,const Rules& technologies,
 const FacilityDefinitions&,Variant runtime_profile,bool expansion_enabled,
 int32_t faction,int32_t base,int32_t queue_count,WorldFacilityOptions& output);
}
#endif
