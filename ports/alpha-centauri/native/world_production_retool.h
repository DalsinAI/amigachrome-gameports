/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PRODUCTION_RETOOL_H
#define ALPHA_WORLD_PRODUCTION_RETOOL_H
#include "original_world.h"
#include "production_retool.h"
namespace alpha {
/* Read-only preview for an explicit proposed queue item, using actual saved
 * human mask/base flags/prior-production/mineral-baseline facts. Rejects edition
 * mismatches; no legacy-to-expansion fixup is invented. The proposed item is
 * classified, not certified buildable. No queue, minerals, RNG or wire writes.
 */
LogicError world_production_retool(const OriginalWorld&,const BasicRules&,Variant,
 size_t base_id,int32_t proposed_item,ProductionRetoolResult&);
}
#endif
