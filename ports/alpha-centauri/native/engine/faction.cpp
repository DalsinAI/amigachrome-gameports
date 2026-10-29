/* SPDX-License-Identifier: GPL-3.0-or-later
 * Faction accessors ported verbatim from Thinker Mod src/faction.cpp,
 * has_tech from src/tech.cpp and valid_player/valid_triad from src/game.cpp
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Extracted as whole functions; edit by hand from here on.
 */
#include "engine.h"

namespace alpha {
namespace engine {

bool has_chassis(int faction_id, VehChassis chs) {
    return has_tech(Chassis[chs].preq_tech, faction_id);
}

bool has_weapon(int faction_id, VehWeapon wpn) {
    return has_tech(Weapon[wpn].preq_tech, faction_id);
}

bool has_project(FacilityId item_id) {
    return project_base(item_id) >= 0;
}

bool has_project(FacilityId item_id, int faction_id) {
    int base_id = project_base(item_id);
    return base_id >= 0 && faction_id >= 0 && Bases[base_id].faction_id == faction_id;
}

int project_base(FacilityId item_id) {
    assert(item_id >= SP_ID_First && item_id <= SP_ID_Last);
    return (item_id >= SP_ID_First && item_id <= SP_ID_Last ?
        SecretProjects[item_id - SP_ID_First] : SP_Unbuilt);
}

int facility_count(FacilityId item_id, int faction_id) {
    assert(item_id > 0 && item_id < SP_ID_First);
    int n = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && has_fac_built(item_id, i)) {
            n++;
        }
    }
    return n;
}

int prod_count(int item_id, int faction_id, int base_skip_id) {
    /* Prototypes must be predefined units or created by the faction. */
    assert(valid_player(faction_id));
    assert(item_id < 0
        || item_id/MaxProtoFactionNum == 0
        || item_id/MaxProtoFactionNum == faction_id);
    int n = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id
        && base->item() == item_id
        && i != base_skip_id) {
            n++;
        }
    }
    return n;
}

/*
Determine if the specified faction is controlled by a human player or computer AI.
*/
bool is_human(int faction_id) {
    assert(faction_id >= 0);
    return FactionStatus[0] & (1 << faction_id);
}

/*
Determine if the specified faction is a Progenitor faction (Caretakers / Usurpers).
*/
bool is_alien(int faction_id) {
    assert(faction_id >= 0);
    return *ExpansionEnabled && MFactions[faction_id].rule_flags & RFLAG_ALIEN;
}

/*
Determine if the specified faction is alive or whether they've been eliminated.
*/
bool is_alive(int faction_id) {
    assert(faction_id >= 0);
    return FactionStatus[1] & (1 << faction_id);
}

void set_human(int faction_id, bool active) {
    assert(faction_id >= 0);
    if (active) {
        FactionStatus[0] |= (1 << faction_id);
    } else {
        FactionStatus[0] &= ~(1 << faction_id);
    }
}

void set_alive(int faction_id, bool active) {
    assert(faction_id >= 0);
    if (active) {
        FactionStatus[1] |= (1 << faction_id);
    } else {
        FactionStatus[1] &= ~(1 << faction_id);
    }
}

/*
Exclude native life since Thinker AI routines don't apply to them.
*/
bool thinker_enabled(int faction_id) {
    return faction_id > 0 && faction_id <= conf.factions_enabled && !is_human(faction_id);
}

bool thinker_move_upkeep(int faction_id) {
    if (is_human(faction_id)) {
        return faction_id > 0 && conf.manage_player_units;
    }
    return faction_id > 0 && faction_id <= conf.factions_enabled;
}

bool at_war(int faction1, int faction2) {
    return faction1 != faction2 && faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA;
}

bool has_pact(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0
        && Factions[faction1].diplo_status[faction2] & DIPLO_PACT;
}

bool both_neutral(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0 && faction1 != faction2
        && !(Factions[faction1].diplo_status[faction2] & (DIPLO_PACT|DIPLO_VENDETTA));
}

bool both_non_enemy(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0 && faction1 != faction2
        && !(Factions[faction1].diplo_status[faction2] & DIPLO_VENDETTA);
}

bool want_revenge(int faction1, int faction2) {
    return faction1 >= 0 && faction2 >= 0 && faction1 != faction2
        && Factions[faction1].diplo_status[faction2] & (DIPLO_ATROCITY_VICTIM | DIPLO_WANT_REVENGE);
}

bool allow_expand(int faction_id) {
    int bases = 0;
    if (*GameRules & RULES_SCN_NO_COLONY_PODS || *BaseCount >= MaxBaseNum) {
        return false;
    }
    if (is_human(faction_id)) {
        return true;
    }
    for (int i = 1; i < MaxPlayerNum && conf.expansion_autoscale > 0; i++) {
        if (is_human(i) && is_alive(i)) {
            bases = max(bases, Factions[i].base_count);
        }
    }
    if (conf.expansion_limit > 0) {
        int pods = 0;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction_id && veh->is_colony()) {
                pods++;
            }
        }
        return Factions[faction_id].base_count + pods < max(bases, conf.expansion_limit);
    }
    return true;
}

bool has_plr_rule(int faction_id, FactionRuleType rule, int* val1, int* val2) {
    MFaction* m = &MFactions[faction_id];
    for (int i = 0; i < m->faction_bonus_count; i++) {
        if (rule == m->faction_bonus_id[i]) {
            if (val1) *val1 = m->faction_bonus_val1[i];
            if (val2) *val2 = m->faction_bonus_val2[i];
            return true;
        }
    }
    return false;
}

bool has_active_veh(int faction_id, VehPlan plan) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && Units[veh->unit_id].plan == plan) {
            return true;
        }
    }
    return false;
}

int veh_count(int faction_id, int unit_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    int num = 0;
    for (int i = 0; i < *VehCount; i++) {
        if (Vehs[i].faction_id == faction_id && Vehs[i].unit_id == unit_id) {
            num++;
        }
    }
    return num;
}

int find_hq(int faction_id) {
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && has_fac_built(FAC_HEADQUARTERS, i)) {
            return i;
        }
    }
    return -1;
}

int __cdecl has_treaty(int faction_id_1, int faction_id_2, uint32_t status) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        return Factions[faction_id_1].diplo_status[faction_id_2] & status;
    } else {
        assert(0);
        return 0;
    }
}

int __cdecl has_agenda(int faction_id_1, int faction_id_2, uint32_t status) {
    if (faction_id_1 >= 0 && faction_id_1 < MaxPlayerNum
    && faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum) {
        return Factions[faction_id_1].diplo_agenda[faction_id_2] & status;
    } else {
        assert(0);
        return 0;
    }
}

char* __cdecl get_adjective(int faction_id) {
    return MFactions[faction_id].adj_name_faction;
}

char* __cdecl get_noun(int faction_id) {
    *PluralDefault = MFactions[faction_id].is_noun_plural;
    *GenderDefault = MFactions[faction_id].noun_gender;
    return MFactions[faction_id].noun_faction;
}

char* __cdecl get_name(int faction_id) {
    *PluralDefault = 0;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    return MFactions[faction_id].name_leader;
}

char* __cdecl get_title(int faction_id) {
    *PluralDefault = 0;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    return MFactions[faction_id].title_leader;
}

char* __cdecl get_adjective_leader(int faction_id) {
    return MFactions[faction_id].adj_leader;
}

char* __cdecl get_adjective_insult_leader(int faction_id) {
    return MFactions[faction_id].adj_insult_leader;
}

char* __cdecl get_adjective_faction(int faction_id) {
    return MFactions[faction_id].adj_faction;
}

char* __cdecl get_adjective_insult_faction(int faction_id) {
    return MFactions[faction_id].adj_insult_faction;
}

char* __cdecl get_insult_leader(int faction_id) {
    *PluralDefault = 0;
    *GenderDefault = MFactions[faction_id].is_leader_female;
    return MFactions[faction_id].insult_leader;
}

int __cdecl has_tech(int tech_id, int faction_id) {
    assert(faction_id >= 0 && faction_id < MaxPlayerNum);
    assert(tech_id >= TECH_Disable && tech_id <= TECH_TranT);
    if (faction_id <= 0) {
        return false;
    }
    if (tech_id == TECH_None) {
        return true;
    }
    if (tech_id < 0 || tech_id >= TECH_TranT
    || Tech[tech_id].preq_tech1 == TECH_Disable
    || (Tech[tech_id].preq_tech2 == TECH_Disable
    && Tech[tech_id].preq_tech1 != TECH_None)) {
        return false;
    }
    return (TechOwners[tech_id] & (1 << faction_id)) != 0;
}

bool valid_player(int faction_id) {
    return faction_id > 0 && faction_id < MaxPlayerNum;
}

bool valid_triad(int triad) {
    return (triad == TRIAD_LAND || triad == TRIAD_SEA || triad == TRIAD_AIR);
}

} // namespace engine
} // namespace alpha
