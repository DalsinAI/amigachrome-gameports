/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented observed original automatic-name grammar contract.
 * Definitions/layout from qualified GPL readers and Thinker(MIT).
 */
#ifndef ALPHA_AUTOMATIC_PROTOTYPE_NAME_H
#define ALPHA_AUTOMATIC_PROTOTYPE_NAME_H
#include "custom_prototype_construction.h"
namespace alpha {
struct PrototypeNameLabels {char scout[128],recon[128],sea[128],amphibious1[128],amphibious2[128],artillery1[128],artillery2[128];};
static_assert(sizeof(PrototypeNameLabels)==896 && offsetof(PrototypeNameLabels,sea)==256 && offsetof(PrototypeNameLabels,amphibious1)==384 && offsetof(PrototypeNameLabels,artillery1)==640,"original label positions preserved");
struct PrototypeName {char bytes[32];};
struct PrototypeLogicalName {char bytes[256];};
struct PrototypeNameSuffix {char token[128];};
enum class PrototypeNameError {ok,bad_state,overflow,collision_context_required,original_would_not_return};
/* Automatic localized name for a constructed custom candidate, virtualized at
 * id64..511 during original active-design comparisons. No naming mutation or
 * authority. Collision paths remain held; some read adjacent runtime context.
 * They fail closed without an invented suffix/choice. All input ranges disjoint from output;
 * errors preserve output and the complete World/rules/definitions/labels. */
PrototypeNameError world_automatic_prototype_name(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const PrototypeNameLabels&,int32_t id,const OriginalPrototype&,PrototypeName&);
/* Complete terminating base suffix resolution with actual owned history prefix.
 * Localized token is original label919. SMAX remains context-held. Refuses a
 * deterministic original nonreturning alternate loop rather than inventing a
 * name. Original primary9 exhaustion can return a duplicate; uniqueness is not
 * promised. No World/session mutation, allocation or award. */
PrototypeNameError world_resolve_automatic_prototype_name(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const PrototypeNameLabels&,const PrototypeNameSuffix&,int32_t id,
 const OriginalPrototype&,PrototypeName&);
/* Original helper logical output, including safe names longer than31 bytes.
 * Caller strncpy32/forced NUL/flag clearing and all proposal effects remain
 * separate required stages. Same base-only suffix/runtime/alias guards. */
PrototypeNameError world_resolve_automatic_prototype_logical_name(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const PrototypeNameLabels&,const PrototypeNameSuffix&,int32_t id,
 const OriginalPrototype&,PrototypeLogicalName&);
}
#endif
