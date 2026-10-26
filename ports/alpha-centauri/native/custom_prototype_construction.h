/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned adaptation of OpenSMACX veh.cpp make_proto, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * and Thinker veh.cpp (MIT, THINKER-LICENSE.txt); original differences retained.
 */
#ifndef ALPHA_CUSTOM_PROTOTYPE_CONSTRUCTION_H
#define ALPHA_CUSTOM_PROTOTYPE_CONSTRUCTION_H
#include "original_world.h"
#include "prototype.h"
namespace alpha {
struct CustomPrototypeRequest {int32_t id,chassis,weapon,armor,reactor;uint32_t abilities;};
/* Complete make_proto custom-design construction proposal, preserving original
 * name/prerequisite/unused bytes. No slot selection, retirement, naming, pruning,
 * manufacture, session/World mutation or discovery award. Actual pre-write
 * model availability and source original technology semantics apply; id64 is
 * custom, unlike an OpenSMACX <=64 reconstruction defect. Candidate Air/Police
 * plan queries include actual eligible SMAX FREEABIL without mutating World or
 * storing invented raw ability bits. All inputs/output disjoint; errors preserve
 * output. Checked byte/int storage ranges reject overflow rather than wrap.
 */
LogicError world_construct_custom_prototype(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,const CustomPrototypeRequest&,OriginalPrototype&);
}
#endif
