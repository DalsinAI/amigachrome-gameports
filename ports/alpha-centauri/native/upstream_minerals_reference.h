/* SPDX-License-Identifier: GPL-3.0-or-later
 * TEST ONLY: OpenSMACX base.cpp::base_minerals body, Brendan Casey
 * (C)2013-2021, pinned2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Numeric state/callback adapters bind explicit fixture facts.
 * Three recovered unsigned types and combined penalty rounding are corrected
 * to the unchanged original instruction behavior; see private helper controls.
 * These are explicit adaptations, not an unmodified-source equivalence claim; no original
 * process, Windows address, unimplemented game callback or native ABI claim.
 */
#ifndef ALPHA_UPSTREAM_MINERALS_REFERENCE_H
#define ALPHA_UPSTREAM_MINERALS_REFERENCE_H
#include "base_minerals.h"
namespace minerals_reference {
struct Base {uint8_t faction_id_current;int8_t population_size;int32_t mineral_intake_2,mineral_consumption,mineral_surplus,mineral_inefficiency,mineral_surplus_final,eco_damage,queue_production_id[10];};
struct Player {int32_t planet_ecology;int32_t satellites_mineral;int32_t major_atrocities,diff_level,tech_ranking,theory_of_everything;struct {int32_t planet;}soc_effect_pending;};
struct Builder {int32_t sea_level_rises;};
static const alpha::MineralInput* context;
static Base base_storage,*base_ptr,* *BaseCurrent=&base_ptr;
static Player PlayersData[8];static Builder builder,*WorldBuilder=&builder;
static int32_t selected,*BaseIDCurrentSelected=&selected,maintenance,*BaseCurrentForcesMaintCost=&maintenance;
static int32_t BaseCurrentConvoyTo[4],BaseCurrentConvoyFrom[4],TectonicDetonationCount[8];
static int32_t sea,*MapSeaLevel=&sea,diff,*DiffLevelCurrent=&diff;
static int32_t life,*MapNativeLifeForms=&life;
static uint32_t rules,*GameRules=&rules,state,*GameState=&state;
enum {RSC_MINERALS=1,FAC_QUANTUM_CONVERTER=21,FAC_ROBOTIC_ASSEMBLY_PLANT=19,FAC_GENEJACK_FACTORY=22,FAC_NANOREPLICATOR=20,FAC_CENTAURI_PRESERVE=31,FAC_TEMPLE_OF_PLANET=32,FAC_SKY_HYDRO_LAB=65,FAC_NESSUS_MINING_STATION=66,FAC_ORBITAL_POWER_TRANS=67,FAC_ORBITAL_DEFENSE_POD=68,SP_SINGULARITY_INDUCTOR=28,SP_BULK_MATTER_TRANSMITTER=29,SP_PHOLUS_MUTAGEN=16,SP_SPACE_ELEVATOR=27,DLVL_TALENT=2,DLVL_LIBRARIAN=3,DLVL_TRANSCEND=5,DLVL_SPECIALIST=1,RULES_VICTORY_TRANSCENDENCE=0x800,STATE_PERIHELION_ACTIVE=0x100000};
static int32_t range(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
static bool is_human(uint32_t){return context->human;}
static bool ascending(uint32_t){return context->ascending;}
static bool has_fac_built(int id,int){switch(id){case 21:return context->quantum_converter;case 19:return context->robotic_assembly;case 22:return context->genejack;case 20:return context->nanoreplicator;case 31:return context->centauri_preserve;case 32:return context->temple;}return false;}
static bool has_project(int id,uint32_t){switch(id){case 28:return context->singularity_inductor;case 29:return context->bulk_matter;case 16:return context->pholus_mutagen;case 27:return context->space_elevator;}return false;}
void original_body() {
    uint32_t faction_id = (*BaseCurrent)->faction_id_current;
    (*BaseCurrent)->mineral_intake_2 += BaseCurrentConvoyTo[RSC_MINERALS];
    int32_t mineral_bonus = (has_fac_built(FAC_QUANTUM_CONVERTER, *BaseIDCurrentSelected)
        || has_project(SP_SINGULARITY_INDUCTOR, faction_id)) ? 1 : 0;
    if (has_fac_built(FAC_ROBOTIC_ASSEMBLY_PLANT, *BaseIDCurrentSelected)) {
        mineral_bonus++;
    }
    if (has_fac_built(FAC_GENEJACK_FACTORY, *BaseIDCurrentSelected)) {
        mineral_bonus++;
    }
    if (has_fac_built(FAC_NANOREPLICATOR, *BaseIDCurrentSelected)) {
        mineral_bonus++;
    }
    if (has_project(SP_BULK_MATTER_TRANSMITTER, faction_id)) {
        mineral_bonus++;
    }
    (*BaseCurrent)->mineral_intake_2 = ((*BaseCurrent)->mineral_intake_2 * (mineral_bonus + 2)) / 2;
    (*BaseCurrent)->mineral_consumption = *BaseCurrentForcesMaintCost
        + BaseCurrentConvoyFrom[RSC_MINERALS];
    (*BaseCurrent)->mineral_surplus = (*BaseCurrent)->mineral_intake_2
        - (*BaseCurrent)->mineral_consumption;
    (*BaseCurrent)->mineral_inefficiency = 0; // ?
    (*BaseCurrent)->mineral_surplus -= (*BaseCurrent)->mineral_inefficiency; // ?
    (*BaseCurrent)->mineral_surplus_final = (*BaseCurrent)->mineral_surplus;
    (*BaseCurrent)->eco_damage /= 8;
    int planet_eco_factor = PlayersData[faction_id].planet_ecology + 16;
    if ((*BaseCurrent)->eco_damage > 0) {
        int planet_modifier = (*BaseCurrent)->eco_damage;
        if ((*BaseCurrent)->eco_damage >= planet_eco_factor) {
            planet_modifier = planet_eco_factor;
        }
        planet_eco_factor -= planet_modifier;
        (*BaseCurrent)->eco_damage -= planet_modifier;
    }
    int eco_dmg_reduction = (has_fac_built(FAC_NANOREPLICATOR, *BaseIDCurrentSelected)
        || has_project(SP_SINGULARITY_INDUCTOR, faction_id)) ? 2 : 1;
    if (has_fac_built(FAC_CENTAURI_PRESERVE, *BaseIDCurrentSelected)) {
        eco_dmg_reduction++;
    }
    if (has_fac_built(FAC_TEMPLE_OF_PLANET, *BaseIDCurrentSelected)) {
        eco_dmg_reduction++;
    }
    if (has_project(SP_PHOLUS_MUTAGEN, faction_id)) {
        eco_dmg_reduction++;
    }
    (*BaseCurrent)->eco_damage += ((*BaseCurrent)->mineral_intake_2 - planet_eco_factor
        - range(PlayersData[faction_id].satellites_mineral, 0, (*BaseCurrent)->population_size))
        / eco_dmg_reduction;
    if (is_human(faction_id)) {
        int denominator = range(*MapSeaLevel, 0, 100) / range(WorldBuilder->sea_level_rises, 1, 100) + eco_dmg_reduction;
        (*BaseCurrent)->eco_damage += (PlayersData[faction_id].major_atrocities * 5) / denominator;
        (*BaseCurrent)->eco_damage += (TectonicDetonationCount[faction_id] * 5) / denominator;
    }
    if ((*BaseCurrent)->eco_damage < 0) {
        (*BaseCurrent)->eco_damage = 0;
    }
    if (ascending(faction_id) && *GameRules & RULES_VICTORY_TRANSCENDENCE) {
        (*BaseCurrent)->eco_damage *= 2;
    }
    if (*GameState & STATE_PERIHELION_ACTIVE) {
        (*BaseCurrent)->eco_damage *= 2;
    }
    int32_t diff_factor;
    if (is_human(faction_id)) {
        int diff_lvl = PlayersData[faction_id].diff_level;
        diff_factor = !diff_lvl ? DLVL_TALENT
            : ((diff_lvl <= DLVL_LIBRARIAN) ? DLVL_LIBRARIAN : DLVL_TRANSCEND);
    } else {
        diff_factor = range(6 - *DiffLevelCurrent, DLVL_SPECIALIST, DLVL_LIBRARIAN);
    }
    (*BaseCurrent)->eco_damage = ((PlayersData[faction_id].tech_ranking
        - PlayersData[faction_id].theory_of_everything)
        * (3 - range(PlayersData[faction_id].soc_effect_pending.planet, -3, 2))
        * (*MapNativeLifeForms + 1) * (*BaseCurrent)->eco_damage * diff_factor) / 6;
    (*BaseCurrent)->eco_damage = ((*BaseCurrent)->eco_damage + 50) / 100;
    int queue_id;
    if (has_project(SP_SPACE_ELEVATOR, faction_id) // orbital facilities double mineral prod rate
        && (queue_id = (*BaseCurrent)->queue_production_id[0], queue_id == -FAC_SKY_HYDRO_LAB
            || queue_id == -FAC_NESSUS_MINING_STATION || queue_id == -FAC_ORBITAL_POWER_TRANS
            || queue_id == -FAC_ORBITAL_DEFENSE_POD)) {
        (*BaseCurrent)->mineral_intake_2 *= 2;
        (*BaseCurrent)->mineral_surplus = // doesn't update mineral_surplus_final?
            (*BaseCurrent)->mineral_intake_2 - (*BaseCurrent)->mineral_consumption;
    }
}

static alpha::MineralResult calculate(const alpha::MineralInput& in){
 context=&in;base_storage={};base_ptr=&base_storage;base_storage.faction_id_current=1;
 base_storage.population_size=(int8_t)in.population;base_storage.mineral_intake_2=in.intake;
 base_storage.eco_damage=in.raw_eco_damage;base_storage.queue_production_id[0]=in.queue_id;
 auto& p=PlayersData[1];p={};p.planet_ecology=in.clean_minerals_modifier;p.satellites_mineral=in.satellites;
 p.major_atrocities=in.atrocities;p.diff_level=in.faction_difficulty;p.tech_ranking=in.tech_ranking;
 p.theory_of_everything=in.transcendent_techs;p.soc_effect_pending.planet=in.planet;
 builder.sea_level_rises=in.sea_level_rises;maintenance=in.maintenance;BaseCurrentConvoyTo[1]=in.convoy_to;BaseCurrentConvoyFrom[1]=in.convoy_from;
 TectonicDetonationCount[1]=in.tectonic_detonations;sea=in.sea_level;diff=in.global_difficulty;life=in.native_life;
 rules=in.transcendence_enabled?0x800u:0;state=in.perihelion?0x100000u:0;original_body();
 const int32_t bonus=(in.quantum_converter || in.singularity_inductor)+in.robotic_assembly+in.genejack+in.nanoreplicator+in.bulk_matter;
 return {base_storage.mineral_intake_2,base_storage.mineral_consumption,base_storage.mineral_surplus,base_storage.mineral_surplus_final,base_storage.eco_damage,bonus,base_storage.mineral_inefficiency};
}
}
#endif
