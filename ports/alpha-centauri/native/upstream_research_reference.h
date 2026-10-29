/* SPDX-License-Identifier: GPL-3.0-or-later
 * Test-only recovered source reference, not linked into the native query.
 * OpenSMACX (C)2013-2021 Brendan Casey, technology.cpp has_tech/tech_avail,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Function bodies retained;
 * BOOL/__cdecl and fixed-address globals replaced with owned test adapters.
 */
#ifndef ALPHA_UPSTREAM_RESEARCH_REFERENCE_H
#define ALPHA_UPSTREAM_RESEARCH_REFERENCE_H
#include "research_options.h"
namespace research_reference {
using BOOL=bool;
constexpr int TechNone=-1,TechDisabled=-2,MaxTechnologyNum=89;
constexpr int TECH_PRPSYCH=78,TECH_FLDMOD=79,TECH_ADAPDOC=80,TECH_ADAPECO=81,TECH_BIOADAP=82,TECH_SENTRES=83,TECH_SECMANI=84,TECH_NEWMISS=85,TECH_BFG9000=86;
struct Entry {int preq_tech_1,preq_tech_2;};
static Entry Technology[89];static uint8_t GameTechAchieved[89];static int expansion;static int* ExpansionEnabled=&expansion;
static void setup(const alpha::OriginalWorld& world,const alpha::Rules& tech,alpha::Variant variant){
 expansion=variant==alpha::Variant::alien_crossfire;
 for(unsigned i=0;i<89;++i){Technology[i]={tech.technologies[i].prerequisites[0],tech.technologies[i].prerequisites[1]};GameTechAchieved[i]=world.game().preferences.TechOwners[i];}
}
static BOOL has_tech(int tech_id, int faction_id) {
    if (faction_id <= 0) {
        return false;
    }
    if (tech_id == TechNone) {
        return true;
    }
    if (tech_id < 0 || tech_id >= (MaxTechnologyNum - 1)
        || Technology[tech_id].preq_tech_1 == TechDisabled
        || (Technology[tech_id].preq_tech_2 == TechDisabled
            && Technology[tech_id].preq_tech_1 != TechNone)) {
        return false;
    }
    return ((1 << faction_id) & GameTechAchieved[tech_id]) != 0;
}
static BOOL tech_avail(uint32_t tech_id, int faction_id) {
    if (has_tech(tech_id, faction_id) || tech_id >= MaxTechnologyNum || (!*ExpansionEnabled
        && (tech_id == TECH_PRPSYCH || tech_id == TECH_FLDMOD || tech_id == TECH_ADAPDOC
            || tech_id == TECH_ADAPECO || tech_id == TECH_BIOADAP || tech_id == TECH_SENTRES
            || tech_id == TECH_SECMANI || tech_id == TECH_NEWMISS || tech_id == TECH_BFG9000))) {
        return false;
    }
    int preq_tech_1 = Technology[tech_id].preq_tech_1;
    int preq_tech_2 = Technology[tech_id].preq_tech_2;
    if (preq_tech_1 < TechNone || preq_tech_2 < TechNone) {
        return false;
    }
    return (has_tech(preq_tech_1, faction_id) && has_tech(preq_tech_2, faction_id));
}
}
#endif
