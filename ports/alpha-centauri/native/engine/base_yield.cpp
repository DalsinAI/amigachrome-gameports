/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: tile resource yields used by the base computation (resource_yield,
 * mod_crop_yield, mod_mine_yield, mod_energy_yield), ported verbatim from
 * Thinker Mod src/map.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: bonus_at() is the WP3 mod_bonus_at(); the asserts that compared
 * each result with the terranx.exe routine are removed.
 */
#include "engine.h"
#include "base_wp2.h"

namespace alpha {
namespace engine {

const int ManifoldHarmonicsBonus[][3] = {
    {0,0,0}, // Planet < 0
    {0,0,1}, // Planet = 0
    {1,0,1}, // Planet = 1
    {1,1,1}, // Planet = 2
    {1,1,2}, // Planet > 2
};

int __cdecl resource_yield(BaseResType type, int faction_id, int base_id, int x, int y) {
    if (type == RSC_NUTRIENT) {
        return mod_crop_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_MINERAL) {
        return mod_mine_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_ENERGY) {
        return mod_energy_yield(faction_id, base_id, x, y, 0);
    } else if (type == RSC_UNUSED) {
        *BaseTerraformReduce = 0;
    }
    return 0;
}

int __cdecl mod_crop_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    int value = 0;
    int alt = sq->alt_level();
    bool has_limit = false;
    bool bonus_landmark = false;
    bool bonus_nutrient = mod_bonus_at(x, y) == RES_NUTRIENT;
    bool is_base = sq->is_base();
    int planet = Factions[faction_id].SE_planet_pending;

    if ((alt >= ALT_SHORE_LINE && sq->landmarks & LM_JUNGLE && !(sq->landmarks & LM_DISABLE))
    || (alt < ALT_SHORE_LINE && sq->landmarks & LM_FRESH)) {
        bonus_landmark = true;
    }
    if (is_base) {
        value = ResInfo->base_sq.nutrient;
        if (bonus_nutrient) {
            value = 2 * ResInfo->base_sq.nutrient;
        }
        if (bonus_landmark) {
            value++;
        }
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.nutrient;
        }
    }
    else if (sq->items & BIT_THERMAL_BORE) {
        value = ResInfo->borehole_sq.nutrient;
        if (bonus_nutrient) {
            value += ResInfo->bonus_sq.nutrient;
        }
    }
    else if (sq->items & BIT_MONOLITH) {
        value = ResInfo->monolith_sq.nutrient;
        if (bonus_nutrient) {
            value += ResInfo->bonus_sq.nutrient;
        }
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][0];
        }
    }
    else if (!flag && sq->is_fungus()) {
        int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_nutrient;
        value = clamp(fungus_val, 0, 99);
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][0];
        }
    } else {
        has_limit = true;
        if (alt < ALT_SHORE_LINE) {
            value = ResInfo->ocean_sq.nutrient;
            if (bonus_nutrient) {
                value = ResInfo->bonus_sq.nutrient + ResInfo->ocean_sq.nutrient;
            }
            if (bonus_landmark)
                value++;
            if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
                if (sq->items & BIT_FARM) {
                    value += ResInfo->improved_sea.nutrient;
                    if (has_fac_built(FAC_AQUAFARM, base_id)) {
                        value++;
                    }
                }
                if (sq->items & BIT_MINE && value > 1) {
                    value += Rules->nutrient_effect_mine_sq;
                }
            }
        } else {
            if (sq->items & BIT_FOREST) {
                value = ResInfo->forest_sq.nutrient
                    + (bonus_nutrient ? ResInfo->bonus_sq.nutrient : 0);
                if (has_fac_built(FAC_TREE_FARM, base_id)) {
                    value++;
                }
                if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
                    value++;
                }
                if (bonus_landmark) {
                    value++;
                }
            } else {
                if (sq->is_rocky()) {
                    value = 0;
                } else {
                    value = (sq->is_rainy() ? 2 : (sq->is_moist() ? 1 : 0));
                }
                if (bonus_nutrient) {
                    value += ResInfo->bonus_sq.nutrient;
                }
                if (bonus_landmark) {
                    value++;
                }
                if (value < 0) {
                    value = 0;
                }
                if (sq->items & BIT_FARM && !sq->is_rocky()) {
                    value += ResInfo->improved_land.nutrient;
                }
                if (sq->items & BIT_MINE && value > 1) {
                    value += Rules->nutrient_effect_mine_sq;
                }
            }
        }
        if (sq->items & BIT_SOIL_ENRICHER) {
            value += (conf.soil_improve_value ? conf.soil_improve_value : value / 2);
        }
        if (sq->items & BIT_CONDENSER) {
            value += (conf.soil_improve_value ? conf.soil_improve_value : value / 2);
        }
    }
    if (has_limit && value > conf.tile_output_limit[0] && !bonus_nutrient && !(sq->items & BIT_CONDENSER)
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_nutrients_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[0]);
        value = conf.tile_output_limit[0];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_BUMPER) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_FAMINE && value) {
            value--;
        }
    }
    // Original: assert against terranx.exe crop_yield removed.
    return value;
}

int __cdecl mod_mine_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    bool has_limit = true;
    bool bonus_landmark = false;
    bool bonus_mineral = mod_bonus_at(x, y) == RES_MINERAL;
    bool is_base = sq->is_base();
    int alt = sq->alt_level();
    int planet = Factions[faction_id].SE_planet_pending;

    if ((sq->landmarks & LM_CRATER && sq->code_at() < 9)
    || (sq->landmarks & LM_VOLCANO && sq->code_at() < 9)
    || (sq->landmarks & LM_FOSSIL && sq->code_at() < 6)
    || (sq->landmarks & LM_CANYON)) {
        bonus_landmark = !(sq->landmarks & LM_DISABLE);
    }
    int value = bonus_landmark + (bonus_mineral ? ResInfo->bonus_sq.mineral : 0);

    if (is_base) {
        value += ResInfo->base_sq.mineral;
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.mineral;
        }
        has_limit = false;
    } else {
        if (sq->items & BIT_MONOLITH) {
            value = ResInfo->monolith_sq.mineral;
            if (bonus_mineral) {
                value += ResInfo->bonus_sq.mineral;
            }
            if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
                value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][1];
            }
            has_limit = false;
        }
        else if (sq->items & BIT_THERMAL_BORE) {
            value = ResInfo->borehole_sq.mineral;
            if (bonus_mineral) {
                value += ResInfo->bonus_sq.mineral;
            }
        }
        else if (sq->is_fungus()) {
            int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_mineral;
            value = clamp(fungus_val, 0, 99);
            if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
                value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][1];
            }
            has_limit = false;
        }
        else if (alt >= ALT_SHORE_LINE) {
            int modifier = sq->val3 >> 6;
            if (sq->items & BIT_FOREST) {
                value += ResInfo->forest_sq.mineral;
            } else if (!(sq->items & BIT_MINE) && !flag) {
                value += (modifier > 0);
            } else {
                value += modifier;
                if (!modifier) {
                    modifier = 1;
                }
                if (bonus_mineral || bonus_landmark) {
                    modifier++;
                }
                if (!(sq->items & BIT_ROAD) && !flag
                && modifier > Rules->limit_mineral_inc_for_mine_wo_road) {
                    *BaseTerraformReduce += (modifier - Rules->limit_mineral_inc_for_mine_wo_road);
                    modifier = Rules->limit_mineral_inc_for_mine_wo_road;
                }
                value += modifier;
            }
        }
        else {
            value += ResInfo->ocean_sq.mineral;
            if (alt == ALT_OCEAN_SHELF && MFactions[faction_id].is_aquatic()
            && conf.aquatic_bonus_minerals) {
                value++;
            }
            if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
                if (sq->items & BIT_MINE || flag) {
                    value += ResInfo->improved_sea.mineral;
                    if (has_tech(Rules->tech_preq_mining_platform_bonus, faction_id) ) {
                        value++;
                    }
                    if (has_fac_built(FAC_SUBSEA_TRUNKLINE, base_id)) {
                        value++;
                    }
                }
            }
        }
    }
    if (has_limit && value > conf.tile_output_limit[1] && !bonus_mineral
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_minerals_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[1]);
        value = conf.tile_output_limit[1];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_INDUSTRY) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_BUST && value) {
            value--;
        }
    }
    // Original: assert against terranx.exe mine_yield removed.
    return value;
}

int __cdecl mod_energy_yield(int faction_id, int base_id, int x, int y, int flag) {
    MAP* sq = mapsq(x, y);
    if (!sq) {
        return 0;
    }
    bool is_fungus = false;
    bool has_limit = true;
    bool bonus_energy = mod_bonus_at(x, y) == RES_ENERGY;
    bool is_base = sq->is_base();
    int economy = Factions[faction_id].SE_economy_pending
        + (base_id >= 0 && Bases[base_id].golden_age_active() ? 1 : 0);
    int planet = Factions[faction_id].SE_planet_pending;
    int alt = sq->alt_level();
    int value = 0;

    if (is_base) {
        bool is_hq = has_fac_built(FAC_HEADQUARTERS, base_id);
        if (has_fac_built(FAC_PRESSURE_DOME, base_id)
        || has_fac_built(FAC_RECYCLING_TANKS, base_id)) {
            value += ResInfo->recycling_tanks.energy;
        }
        if (economy > 4) {
            value += 4;
        } else if (economy > 3) {
            value += 2;
        } else if (economy >= 0) {
            value += (economy == 1); // +1 each square is added later
        } else {
            value += economy;
            if (!is_hq || economy < -1 || Factions[faction_id].base_count == 1) {
                value++;
            }
        }
        value += ResInfo->base_sq.energy + is_hq;
        if (*GovernorFaction == faction_id) {
            value++;
        }
        has_limit = false;
    }
    // Monolith takes priority over any fungus values on the same tile.
    // Modify the game to not apply 2 resource yield restrictions on monolith energy.
    // This limit does not apply on nutrients/minerals produced by monoliths.
    else if (sq->items & BIT_MONOLITH) {
        value = ResInfo->monolith_sq.energy;
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][2];
        }
        has_limit = false;
    }
    else if (sq->items & BIT_THERMAL_BORE) {
        value = ResInfo->borehole_sq.energy;
    }
    else if (sq->is_fungus()) {
        int fungus_val = clamp(planet, -3, 0) + Factions[faction_id].tech_fungus_energy;
        value = clamp(fungus_val, 0, 99);
        if (has_project(FAC_MANIFOLD_HARMONICS, faction_id)) {
            value += ManifoldHarmonicsBonus[clamp(planet + 1, 0, 4)][2];
        }
        is_fungus = true;
        has_limit = false;
    }
    else if (alt < ALT_SHORE_LINE) {
        if (alt == ALT_OCEAN_SHELF || *ExpansionEnabled) {
            value = ResInfo->ocean_sq.energy;
            if (sq->items & BIT_SOLAR || flag) {
                value += ResInfo->improved_sea.energy;
                if (has_fac_built(FAC_THERMOCLINE_TRANSDUCER, base_id)) {
                    value++;
                }
            }
        }
        if (sq->items & BIT_FOREST) {
            value--;
        }
    }
    else if (sq->items & BIT_FOREST) {
        value = ResInfo->forest_sq.energy;
        if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
            value++;
        }
    }
    else {
        if (sq->items & (BIT_ECH_MIRROR|BIT_SOLAR) || flag) {
            value += 1 + max(0, alt - ALT_SHORE_LINE);
        }
        // Fix crash issue by adding null pointer check for CurrentBase
        if (sq->items & BIT_SOLAR && *CurrentBase) {
            for (auto& m : iterate_tiles(x, y, 1, 9)) {
                if (m.sq->items & BIT_ECH_MIRROR
                && m.sq->owner == (*CurrentBase)->faction_id) {
                    value++;
                }
            }
        }
    }
    if (!is_fungus) {
        if (sq->items & BIT_RIVER && alt >= ALT_SHORE_LINE) {
            value++;
        }
        if (bonus_energy) {
            value += ResInfo->bonus_sq.energy;
        }
        if ((sq->landmarks & LM_VOLCANO && sq->code_at() < 9)
        || sq->landmarks & (LM_URANIUM|LM_GEOTHERMAL|LM_RIDGE)) {
            value += !(sq->landmarks & LM_DISABLE);
        }
        if (base_id >= 0 && project_base(FAC_MERCHANT_EXCHANGE) == base_id) {
            value++;
        }
        if (economy >= 2) {
            value++;
        }
        if (is_base) {
            value = clamp(value, 0, 99);
            *BaseTerraformEnergy = value;
        }
    }
    if (has_limit && value > conf.tile_output_limit[2] && !bonus_energy
    && (faction_id < 0 || !has_tech(Rules->tech_preq_allow_3_energy_sq, faction_id))) {
        *BaseTerraformReduce += (value - conf.tile_output_limit[2]);
        value = conf.tile_output_limit[2];
    }
    if (base_id >= 0) {
        if (Bases[base_id].event_flags & BEVENT_HEAT_WAVE) {
            value++;
        }
        if (Bases[base_id].event_flags & BEVENT_CLOUD_COVER && value) {
            value--;
        }
    }
    if (*DustCloudDuration && value) {
        value--;
    }
    bool solar_flares;
    if (*ControlUpkeepA) {
        solar_flares = *SolarFlaresEvent & 2;
    } else {
        solar_flares = *SolarFlaresEvent & 1;
    }
    value = (solar_flares ? 3 * value : value);
    // Original: assert against terranx.exe energy_yield removed.
    return value;
}

} // namespace engine
} // namespace alpha
