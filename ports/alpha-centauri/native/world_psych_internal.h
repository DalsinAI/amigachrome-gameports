/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PSYCH_INTERNAL_H
#define ALPHA_WORLD_PSYCH_INTERNAL_H
#include "world_psych.h"
namespace alpha {namespace detail {
// Internal only. Public callers seal World/table/context/output ranges first.
// Facts and garrison are private disjoint working storage, not published outputs.
// The facts police pointer borrows the supplied garrison for the current call.
WorldPsychError resolve_world_psych_facts(const OriginalWorld&,const Rules&,
 const UnitDefinitions&,const BasicRules&,size_t,PoliceSnapshotProfile,uint32_t,
 BasePsychFacts&,WorldPoliceGarrison&);
}}
#endif
