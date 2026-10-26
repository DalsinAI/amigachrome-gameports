/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_UNIT_AVAILABILITY_H
#define ALPHA_WORLD_UNIT_AVAILABILITY_H
#include "unit_availability.h"
#include "original_world.h"
namespace alpha {
/* Read-only original-world binding; no production authority or queue mutation.
 * Exact runtime/wire edition required; ExpansionEnabled is explicit, even for
 * SMAX with its expansion mode disabled. Base=-1 is a global query. Cross-owner
 * base queries are allowed as in the original helper; this grants no order.
 * Base-present queries reject invalid chassis references conservatively even
 * when an earlier availability gate would reject. No-base ignores chassis.
 * Failure preserves output; aliases into any input/owned allocation reject.
 */
LogicError world_unit_availability(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,Variant,bool expansion_enabled,int32_t faction,
 int32_t base,int32_t unit,bool& output);
}
#endif
