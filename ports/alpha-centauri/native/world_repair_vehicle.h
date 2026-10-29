/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_REPAIR_VEHICLE_H
#define ALPHA_WORLD_REPAIR_VEHICLE_H
#include "repair_vehicle.h"
#include "original_world.h"
namespace alpha {
/* Read-only binding to CURRENT decoded records. Explicit phase/local faction
 * are query context, not authority; saved CurrentTurn supplies arithmetic only.
 * Derives actual base_at/territory predicate, base-owner breed modifiers,
 * physical facilities, phase-owned projects, original cargo capacity, and
 * edition-correct repair-bay grants from design-owner technology. The linked
 * stack must be reciprocal, acyclic, coordinate-consistent and contain every
 * colocated actor when the original repair path inspects it. No stack fixup.
 * All outputs reject aliases with World/rule storage before writes. No World
 * mutation, phase admission/ODP reset, draw delivery, movement refill, turn,
 * player command, or save authority. Foreign actors preserve their records.
 */
LogicError world_repair_vehicle(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 Variant profile,int32_t phase_faction,int32_t local_faction,size_t vehicle,RepairVehicleResult&);
}
#endif
