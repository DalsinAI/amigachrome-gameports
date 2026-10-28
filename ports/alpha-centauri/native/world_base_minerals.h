/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_BASE_MINERALS_H
#define ALPHA_WORLD_BASE_MINERALS_H
#include "upkeep_base_minerals.h"
#include "world_convoy_nutrients.h"
namespace alpha {
struct UpkeepWorldMineralStage {
 StableIntakeStage intake;
 int32_t forces_maintenance; // fresh support-engine result, never saved consumption
};
struct UpkeepWorldMineralResult {
 WorldConvoyFlows convoys;
 StableIntakeResult fresh;
 MineralFacts facts;
 UpkeepMineralResult minerals;
};
struct CurrentMineralInputs {
 Variant variant;
 int32_t fresh_intake,convoy_to,convoy_from,forces_maintenance,raw_eco_damage;
};
struct WorldMineralAccounting {MineralFacts facts;UpkeepMineralResult minerals;};
/* Internal current-stage binding after worker assignment. Inputs must be fresh
 * current proposed tile/convoy/support/ecology outputs, never saved totals.
 * This does not compute/qualify those upstream stages or grant turn authority.
 * Derives all real owner/facility/project/map facts and applies recovered leaf.
 * No mutation; all world/rule/input/output overlaps reject atomically. */
StableIntakeError world_mineral_accounting(const OriginalWorld&,const WorldRules&,
 size_t base,const CurrentMineralInputs&,WorldMineralAccounting&);
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
 const UpkeepWorldMineralStage&,UpkeepWorldMineralResult&);
}
#endif
