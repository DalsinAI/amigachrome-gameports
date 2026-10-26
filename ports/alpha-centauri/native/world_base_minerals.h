/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_BASE_MINERALS_H
#define ALPHA_WORLD_BASE_MINERALS_H
#include "base_minerals.h"
#include "world_convoy_nutrients.h"
namespace alpha {
struct WorldMineralStage {
 StableIntakeStage intake;
 int32_t forces_maintenance; // fresh support-engine result, never saved consumption
};
struct WorldMineralResult {
 WorldConvoyFlows convoys;
 StableIntakeResult fresh;
 MineralFacts facts;
 MineralResult minerals;
};
/* Read-only owned-world proposal. Computes actual convoy contributions then
 * fresh stable-human intake/raw ecology. The only supplied accounting fact is
 * fresh forces maintenance; callers must own its support stage. All physical
 * facilities, project effects, faction ecology, satellite/technology/difficulty,
 * sea-level/native-life, Voice/perihelion and orbital-queue facts are bound here.
 * WorldRules must be parsed from the same original rules input.
 * Inspection stage0 only: no allocator, support engine, actor mutation, mineral
 * accrual, production completion, turn commit or save serialization is implied.
 * Failure and aliases into any input/owned world region preserve output.
 */
StableIntakeError world_base_minerals(const OriginalWorld&,const Rules&,
 const BasicRules&,const WorldRules&,const ResourceInfo&,size_t base,
 const WorldMineralStage&,WorldMineralResult&);
}
#endif
