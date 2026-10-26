/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented observed original automatic-name grammar contract.
 * Definitions/layout from qualified GPL readers and Thinker(MIT).
 */
#ifndef ALPHA_AUTOMATIC_PROTOTYPE_NAME_H
#define ALPHA_AUTOMATIC_PROTOTYPE_NAME_H
#include "custom_prototype_construction.h"
namespace alpha {
struct PrototypeNameLabels {char scout[128],recon[128],foil[128],artillery1[128],artillery2[128],marine1[128],marine2[128];};
struct PrototypeName {char bytes[32];};
enum class PrototypeNameError {ok,bad_state,overflow,collision_context_required};
/* Automatic localized name for a constructed custom candidate, virtualized at
 * id64..511 during original active-design comparisons. No naming mutation or
 * authority. Collision paths remain held; some read adjacent runtime context.
 * They fail closed without an invented suffix/choice. All input ranges disjoint from output;
 * errors preserve output and the complete World/rules/definitions/labels. */
PrototypeNameError world_automatic_prototype_name(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const PrototypeNameLabels&,int32_t id,const OriginalPrototype&,PrototypeName&);
}
#endif
