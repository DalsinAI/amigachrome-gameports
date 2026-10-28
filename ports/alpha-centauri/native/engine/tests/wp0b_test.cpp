/* SPDX-License-Identifier: GPL-3.0-or-later
 * Rules loader checks (WP0b), driven by test_engine_wp0b.py.
 *
 *   wp0b_test unit
 *       string helpers, parse_string, variant slots, failure without a bound
 *       state; prints "<n> unit checks passed".
 *   wp0b_test load DIR base|expansion [twice]
 *       engine_load_rules(DIR) into a fresh state and print every loaded
 *       table as key=value lines. Exit 0 on success, 20 on a rejected load
 *       (error=<text> printed). "twice" loads into a second state first and
 *       checks the first one is unchanged by the second load.
 */
#include "../engine.h"

#include <memory>

using namespace alpha::engine;

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    checks++; \
    long long va_ = (long long)(a), vb_ = (long long)(b); \
    if (va_ != vb_) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK_EQ failed: %s = %lld, %s = %lld\n", \
            __FILE__, __LINE__, #a, va_, #b, vb_); \
    } \
} while (0)

#define CHECK_STR(a, b) do { \
    checks++; \
    if (strcmp((a), (b))) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK_STR failed: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
    } \
} while (0)

static int unit_checks() {
    CHECK_EQ(stoi("42"), 42);
    CHECK_EQ(stoi("-7"), -7);
    CHECK_EQ(stoi("0x1F"), 31);
    CHECK_EQ(stoi("0b101"), 5);
    CHECK_EQ(stoi("0d12"), 12);
    CHECK_EQ(btoi("000101"), 5);
    CHECK_EQ(btoi(""), 0);
    char buf[64];
    strcpy(buf, "   padded text   ");
    purge_spaces(buf);
    CHECK_STR(buf, "padded text");
    strcpy(buf, "   ");
    purge_spaces(buf);
    CHECK_STR(buf, "");
    strcpy(buf, "line\r\n");
    kill_lf(buf);
    CHECK_STR(buf, "line");
    strcpy(buf, "Name \t ");
    strtrail(buf);
    CHECK_STR(buf, "Name");
    CHECK(_stricmp("AbC", "aBc") == 0);
    CHECK(_stricmp("abc", "abd") < 0);
    CHECK(_strnicmp("Econ", "ECONOMY", 4) == 0);
    CHECK(Strings_put("same") == Strings_put("same"));

    /* Without a bound state the loader refuses. */
    CHECK_EQ(engine_load_rules(".", 1), 1);
    CHECK(strstr(engine_rules_error(), "no EngineState") != nullptr);

    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    parse_says(0, "Farm", -1, -1);
    parse_num(1, 12);
    char out[StrBufLen] = "";
    CHECK_EQ(parse_string("Build $STR0 ($NUM1) $$ $WORD0", out), 0);
    CHECK_STR(out, "Build Farm (12) $ Farm");

    *ExpansionEnabled = 1;
    CHECK_EQ(rules_weapon_slot(12), 12);
    CHECK_EQ(rules_facility_slot(40), 40);
    *ExpansionEnabled = 0;
    CHECK_EQ(rules_weapon_slot(11), WPN_SINGULARITY_LASER);
    CHECK_EQ(rules_weapon_slot(12), WPN_STRING_DISRUPTOR);
    CHECK_EQ(rules_weapon_slot(20), WPN_ALIEN_ARTIFACT);
    CHECK_EQ(rules_facility_slot(33), FAC_PSI_GATE);
    CHECK_EQ(rules_facility_slot(34), FAC_SKY_HYDRO_LAB);
    CHECK_EQ(rules_facility_slot(38), FAC_STOCKPILE_ENERGY);
    CHECK_EQ(rules_facility_slot(39), FAC_HUMAN_GENOME_PROJECT);
    CHECK_EQ(rules_facility_slot(71), FAC_ASCENT_TO_TRANSCENDENCE);
    CHECK_EQ(rules_counts(0).facilities, 71);
    CHECK_EQ(rules_counts(1).facilities, SP_ID_Last);

    /* Missing directory: the error names labels.txt and the state keeps
     * non-null strings. */
    CHECK_EQ(engine_load_rules("/nonexistent-rules-dir", 1), 1);
    CHECK(strstr(engine_rules_error(), "labels") != nullptr);
    CHECK(Weapon[0].name != nullptr && Facility[FAC_HEADQUARTERS].name != nullptr);
    engine_bind(nullptr);
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}

static void kv(const char* key, long long value) {
    printf("%s=%lld\n", key, value);
}

static void ks(const char* key, const char* value) {
    printf("%s=%s\n", key, value ? value : "(null)");
}

static void dump() {
    char k[128];
#define KV(fmt_, idx_, value_) do { snprintf(k, sizeof(k), fmt_, idx_); kv(k, (long long)(value_)); } while (0)
#define KS(fmt_, idx_, value_) do { snprintf(k, sizeof(k), fmt_, idx_); ks(k, value_); } while (0)
    kv("expansion", *ExpansionEnabled);
    kv("labels.count", TextLabels->label_count);
    for (int i = 0; i < TextLabels->label_count; i++) KS("labels.%d", i, label_get(i));
    for (int i = 0; i < MaxTechnologyNum; i++) {
        KS("tech.%d.name", i, Tech[i].name);
        KS("tech.%d.short", i, Tech[i].short_name);
        KV("tech.%d.ai_power", i, Tech[i].AI_power);
        KV("tech.%d.ai_tech", i, Tech[i].AI_tech);
        KV("tech.%d.ai_wealth", i, Tech[i].AI_wealth);
        KV("tech.%d.ai_growth", i, Tech[i].AI_growth);
        KV("tech.%d.preq1", i, Tech[i].preq_tech1);
        KV("tech.%d.preq2", i, Tech[i].preq_tech2);
        KV("tech.%d.flags", i, Tech[i].flags);
    }
    kv("tech_valid", *TechValidCount);
    kv("tech_commerce", *TechCommerceCount);
    /* #RULES in file order */
    const int32_t rules_order[] = {
        Rules->move_rate_roads, Rules->nutrient_intake_req_citizen,
        Rules->artillery_dmg_numerator, Rules->artillery_dmg_denominator,
        Rules->artillery_max_rng, Rules->max_airdrop_rng_wo_orbital_insert,
        Rules->nutrient_cost_multi, Rules->mineral_cost_multi, Rules->tech_discovery_rate,
        Rules->limit_mineral_inc_for_mine_wo_road, Rules->nutrient_effect_mine_sq,
        Rules->min_base_size_specialists, Rules->drones_induced_genejack_factory,
        Rules->pop_limit_wo_hab_complex, Rules->pop_limit_wo_hab_dome,
        Rules->extra_cost_prototype_land, Rules->extra_cost_prototype_sea,
        Rules->extra_cost_prototype_air,
        Rules->psi_combat_ratio_atk[TRIAD_LAND], Rules->psi_combat_ratio_def[TRIAD_LAND],
        Rules->psi_combat_ratio_atk[TRIAD_SEA], Rules->psi_combat_ratio_def[TRIAD_SEA],
        Rules->psi_combat_ratio_atk[TRIAD_AIR], Rules->psi_combat_ratio_def[TRIAD_AIR],
        Rules->starting_energy_reserve, Rules->combat_bonus_intrinsic_base_def,
        Rules->combat_bonus_atk_road, Rules->combat_bonus_atk_higher_elevation,
        Rules->combat_penalty_atk_lower_elevation, Rules->combat_mobile_open_ground,
        Rules->combat_mobile_def_in_rough, Rules->combat_infantry_vs_base,
        Rules->combat_penalty_atk_airdrop, Rules->combat_bonus_fanatic,
        Rules->combat_land_vs_sea_artillery, Rules->combat_artillery_bonus_altitude,
        Rules->combat_bonus_trance_vs_psi, Rules->combat_bonus_empath_song_vs_psi,
        Rules->combat_penalty_air_supr_vs_ground, Rules->combat_bonus_air_supr_vs_air,
        Rules->combat_penalty_non_combat_vs_combat, Rules->combat_comm_jammer_vs_mobile,
        Rules->combat_bonus_vs_ship_port, Rules->combat_aaa_bonus_vs_air,
        Rules->combat_defend_sensor, Rules->combat_psi_bonus_per_planet,
        Rules->retool_penalty_prod_change, Rules->retool_strictness, Rules->retool_exemption,
        Rules->min_turns_between_councils, Rules->minerals_harvesting_forest,
        Rules->territory_max_dist_base, Rules->turns_corner_global_energy_market,
        Rules->tech_preq_improv_fungus, Rules->tech_preq_ease_fungus_mov,
        Rules->tech_preq_build_road_fungus, Rules->tech_preq_allow_2_spec_abil,
        Rules->tech_preq_allow_3_nutrients_sq, Rules->tech_preq_allow_3_minerals_sq,
        Rules->tech_preq_allow_3_energy_sq, Rules->tech_preq_orb_insert_wo_space,
        Rules->tech_preq_mining_platform_bonus, Rules->tech_preq_economic_victory,
        Rules->tgl_probe_steal_tech, Rules->tgl_humans_always_contact_net,
        Rules->tgl_humans_always_contact_pbem, Rules->max_dmg_percent_arty_base_bunker,
        Rules->max_dmg_percent_arty_open, Rules->max_dmg_percent_arty_sea,
        Rules->freq_global_warming_numerator, Rules->freq_global_warming_denominator,
        Rules->normal_start_year, Rules->normal_end_year_low_three_diff,
        Rules->normal_end_year_high_three_diff, Rules->tgl_oblit_base_atrocity,
        Rules->base_size_subspace_gen, Rules->subspace_gen_req,
    };
    for (size_t i = 0; i < sizeof(rules_order) / sizeof(rules_order[0]); i++) KV("rules.%d", (int)i, rules_order[i]);
    const int32_t world_order[] = {
        WorldBuilder->land_base, WorldBuilder->land_mod, WorldBuilder->continent_base,
        WorldBuilder->continent_mod, WorldBuilder->hills_base, WorldBuilder->hills_mod,
        WorldBuilder->plateau_base, WorldBuilder->plateau_mod, WorldBuilder->rivers_base,
        WorldBuilder->rivers_rain_mod, WorldBuilder->solar_energy, WorldBuilder->thermal_band,
        WorldBuilder->thermal_deviance, WorldBuilder->global_warming, WorldBuilder->sea_level_rises,
        WorldBuilder->cloudmass_peaks, WorldBuilder->cloudmass_hills, WorldBuilder->rainfall_coeff,
        WorldBuilder->deep_water, WorldBuilder->shelf, WorldBuilder->plains, WorldBuilder->beach,
        WorldBuilder->hills, WorldBuilder->peaks, WorldBuilder->fungus,
        WorldBuilder->cont_size_ratio1, WorldBuilder->cont_size_ratio2, WorldBuilder->cont_size_ratio3,
        WorldBuilder->cont_size_ratio4, WorldBuilder->cont_size_ratio5, WorldBuilder->islands,
    };
    for (size_t i = 0; i < sizeof(world_order) / sizeof(world_order[0]); i++) KV("world.%d", (int)i, world_order[i]);
    for (int i = 0; i < MaxTerrainNum; i++) {
        KS("terrain.%d.name", i, Terraform[i].name);
        KV("terrain.%d.preq", i, Terraform[i].preq_tech);
        KS("terrain.%d.name_sea", i, Terraform[i].name_sea);
        KV("terrain.%d.preq_sea", i, Terraform[i].preq_tech_sea);
        KV("terrain.%d.rate", i, Terraform[i].rate);
        KV("terrain.%d.bit", i, Terraform[i].bit);
        KS("terrain.%d.shortcuts", i, Terraform[i].shortcuts);
    }
    for (int i = 0; i < MaxOrderNum; i++) {
        KS("order.%d.order", i, Order[i].order);
        KS("order.%d.order_sea", i, Order[i].order_sea);
        KS("order.%d.letter", i, Order[i].letter);
    }
    const ResValue* res[MaxResourceInfoNum] = {
        &ResInfo->ocean_sq, &ResInfo->base_sq, &ResInfo->bonus_sq,
        &ResInfo->forest_sq, &ResInfo->recycling_tanks, &ResInfo->improved_land,
        &ResInfo->improved_sea, &ResInfo->monolith_sq, &ResInfo->borehole_sq,
    };
    for (int i = 0; i < MaxResourceInfoNum; i++) {
        KV("resinfo.%d.nutrient", i, res[i]->nutrient);
        KV("resinfo.%d.mineral", i, res[i]->mineral);
        KV("resinfo.%d.energy", i, res[i]->energy);
        KV("resinfo.%d.unused", i, res[i]->unused);
    }
    for (int i = 0; i < MaxTimeControlNum; i++) {
        KS("time.%d.name", i, TimeControl[i].name);
        KV("time.%d.turn", i, TimeControl[i].turn);
        KV("time.%d.base", i, TimeControl[i].base);
        KV("time.%d.unit", i, TimeControl[i].unit);
        KV("time.%d.event", i, TimeControl[i].event);
        KV("time.%d.extra", i, TimeControl[i].extra);
        KV("time.%d.refresh", i, TimeControl[i].refresh);
        KV("time.%d.accumulated", i, TimeControl[i].accumulated);
    }
    for (int i = 0; i < MaxChassisNum; i++) {
        const CChassis& c = Chassis[i];
        const char* names[6] = {c.offsv1_name, c.offsv2_name, c.defsv1_name, c.defsv2_name,
            c.offsv_name_lrg, c.defsv_name_lrg};
        int genders[6] = {c.offsv1_gender, c.offsv2_gender, c.defsv1_gender, c.defsv2_gender,
            c.offsv_gender_lrg, c.defsv_gender_lrg};
        int plurals[6] = {c.offsv1_is_plural, c.offsv2_is_plural, c.defsv1_is_plural,
            c.defsv2_is_plural, c.offsv_is_plural_lrg, c.defsv_is_plural_lrg};
        for (int j = 0; j < 6; j++) {
            snprintf(k, sizeof(k), "chassis.%d.noun.%d.name", i, j); ks(k, names[j]);
            snprintf(k, sizeof(k), "chassis.%d.noun.%d.gender", i, j); kv(k, genders[j]);
            snprintf(k, sizeof(k), "chassis.%d.noun.%d.plural", i, j); kv(k, plurals[j]);
        }
        KV("chassis.%d.speed", i, c.speed);
        KV("chassis.%d.triad", i, c.triad);
        KV("chassis.%d.range", i, c.range);
        KV("chassis.%d.missile", i, c.missile);
        KV("chassis.%d.cargo", i, c.cargo);
        KV("chassis.%d.cost", i, c.cost);
        KV("chassis.%d.prerequisite", i, c.preq_tech);
    }
    for (int i = 0; i < MaxReactorNum; i++) {
        KS("reactors.%d.name", i, Reactor[i].name);
        KS("reactors.%d.short_name", i, Reactor[i].name_short);
        KV("reactors.%d.power", i, Reactor[i].power);
        KV("reactors.%d.prerequisite", i, Reactor[i].preq_tech);
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        KS("weapon.%d.name", i, Weapon[i].name);
        KS("weapon.%d.short_name", i, Weapon[i].name_short);
        KV("weapon.%d.offense", i, Weapon[i].offense_value);
        KV("weapon.%d.mode", i, Weapon[i].mode);
        KV("weapon.%d.cost", i, Weapon[i].cost);
        KV("weapon.%d.icon", i, Weapon[i].icon);
        KV("weapon.%d.prerequisite", i, Weapon[i].preq_tech);
    }
    for (int i = 0; i < MaxArmorNum; i++) {
        KS("armor.%d.name", i, Armor[i].name);
        KS("armor.%d.short_name", i, Armor[i].name_short);
        KV("armor.%d.defense", i, Armor[i].defense_value);
        KV("armor.%d.mode", i, Armor[i].mode);
        KV("armor.%d.cost", i, Armor[i].cost);
        KV("armor.%d.prerequisite", i, Armor[i].preq_tech);
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        KS("abilities.%d.name", i, Ability[i].name);
        KV("abilities.%d.cost_factor", i, Ability[i].cost);
        KV("abilities.%d.prerequisite", i, Ability[i].preq_tech);
        KS("abilities.%d.abbreviation", i, Ability[i].abbreviation);
        KV("abilities.%d.flags", i, Ability[i].flags);
        KS("abilities.%d.description", i, Ability[i].description);
    }
    for (int i = 0; i < MaxMoraleNum; i++) {
        KS("morale.%d.name", i, Morale[i].name);
        KS("morale.%d.lifecycle", i, Morale[i].name_lifecycle);
    }
    for (int i = 0; i < MaxDefenseModeNum; i++) {
        KS("defmode.%d.name", i, DefenseMode[i].name);
        KS("defmode.%d.hyphened", i, DefenseMode[i].hyphened);
        KS("defmode.%d.abbrev", i, DefenseMode[i].abbrev);
        KS("defmode.%d.letter", i, DefenseMode[i].letter);
    }
    for (int i = 0; i < MaxOffenseModeNum; i++) {
        KS("offmode.%d.name", i, OffenseMode[i].name);
        KS("offmode.%d.hyphened", i, OffenseMode[i].hyphened);
        KS("offmode.%d.abbrev", i, OffenseMode[i].abbrev);
        KS("offmode.%d.letter", i, OffenseMode[i].letter);
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        if (!(Units[i].unit_flags & UNIT_ACTIVE)) {
            continue;
        }
        KS("units.%d.name", i, Units[i].name);
        KV("units.%d.chassis", i, Units[i].chassis_id);
        KV("units.%d.weapon", i, Units[i].weapon_id);
        KV("units.%d.armor", i, Units[i].armor_id);
        KV("units.%d.prerequisite", i, Units[i].preq_tech);
        KV("units.%d.icon", i, Units[i].icon_offset);
        KV("units.%d.abilities", i, Units[i].ability_flags);
        KV("units.%d.reactor", i, Units[i].reactor_id);
    }
    for (int i = 1; i <= SP_ID_Last; i++) {
        KS("facility.%d.name", i, Facility[i].name);
        KV("facility.%d.cost", i, Facility[i].cost);
        KV("facility.%d.maint", i, Facility[i].maint);
        KV("facility.%d.preq", i, Facility[i].preq_tech);
        KV("facility.%d.free", i, Facility[i].free_tech);
        KS("facility.%d.effect", i, Facility[i].effect);
        KV("facility.%d.ai_fight", i, Facility[i].AI_fight);
        KV("facility.%d.ai_power", i, Facility[i].AI_power);
        KV("facility.%d.ai_tech", i, Facility[i].AI_tech);
        KV("facility.%d.ai_wealth", i, Facility[i].AI_wealth);
        KV("facility.%d.ai_growth", i, Facility[i].AI_growth);
    }
    for (int i = 0; i < MaxCompassNum; i++) KS("compass.%d", i, Compass[i]);
    for (int i = 0; i < MaxPlanNum; i++) {
        KS("plan.%d.short", i, PlansShortName[i]);
        KS("plan.%d.full", i, PlansFullName[i]);
    }
    for (int i = 0; i < MaxTriadNum; i++) KS("triad.%d", i, TriadName[i]);
    for (int i = 0; i < MaxResourceNum; i++) {
        KS("resource.%d.singular", i, ResName[i].name_singular);
        KS("resource.%d.plural", i, ResName[i].name_plural);
    }
    for (int i = 0; i < MaxEnergyNum; i++) {
        KS("energy.%d.abbrev", i, Energy[i].abbrev);
        KS("energy.%d.name", i, Energy[i].name);
    }
    for (int i = 0; i < MaxCitizenNum; i++) {
        KS("citizen.%d.singular", i, Citizen[i].singular_name);
        KS("citizen.%d.plural", i, Citizen[i].plural_name);
        KV("citizen.%d.preq", i, Citizen[i].preq_tech);
        KV("citizen.%d.obsol", i, Citizen[i].obsol_tech);
        KV("citizen.%d.econ", i, Citizen[i].econ_bonus);
        KV("citizen.%d.psych", i, Citizen[i].psych_bonus);
        KV("citizen.%d.labs", i, Citizen[i].labs_bonus);
    }
    for (int i = 0; i < MaxSocialEffectNum; i++) {
        KS("effect.%d.key", i, SocialParam[i].set1);
        KS("effect.%d.label", i, SocialParam[i].set2);
    }
    for (int c = 0; c < MaxSocialCatNum; c++) {
        KS("category.%d", c, SocialField[c].field_name);
        for (int m = 0; m < MaxSocialModelNum; m++) {
            snprintf(k, sizeof(k), "model.%d.%d.name", c, m); ks(k, SocialField[c].soc_name[m]);
            snprintf(k, sizeof(k), "model.%d.%d.tech", c, m); kv(k, SocialField[c].soc_preq_tech[m]);
            for (int e = 0; e < MaxSocialEffectNum; e++) {
                snprintf(k, sizeof(k), "model.%d.%d.effect.%d", c, m, e);
                kv(k, SocialField[c].soc_effect[m].values[e]);
            }
        }
    }
    for (int i = 0; i < MaxBonusNameNum; i++) KS("bonus.%d", i, BonusName[i].key);
    for (int i = 0; i < MaxDiffNum; i++) KS("diff.%d", i, Difficulty[i]);
    for (int i = 0; i < MaxMandateNum; i++) {
        KS("mandate.%d.name", i, Mandate[i].name);
        KS("mandate.%d.caps", i, Mandate[i].name_caps);
    }
    for (int i = 0; i < MaxMoodNum; i++) KS("mood.%d", i, Mood[i]);
    for (int i = 0; i < MaxReputeNum; i++) KS("repute.%d", i, Repute[i]);
    for (int i = 0; i < MaxMightNum; i++) {
        KS("might.%d.adj_start", i, Might[i].adj_start);
        KS("might.%d.adj", i, Might[i].adj);
    }
    for (int i = 0; i < MaxProposalNum; i++) {
        KS("proposal.%d.name", i, Proposal[i].name);
        KV("proposal.%d.preq", i, Proposal[i].preq_tech);
        KS("proposal.%d.description", i, Proposal[i].description);
    }
    for (int i = 0; i < MaxNaturalNum; i++) {
        KS("natural.%d.name", i, Natural[i].name);
        KS("natural.%d.short", i, Natural[i].name_short);
    }
    kv("faction_pool", (long long)faction_pool.size());
    kv("missing_factions", engine_rules_missing_factions());
    for (int i = 1; i < MaxPlayerNum; i++) {
        const MFaction& f = MFactions[i];
        char p[32];
        snprintf(p, sizeof(p), "faction.%d.", i);
        auto fs = [&](const char* key, const char* value) {
            snprintf(k, sizeof(k), "%s%s", p, key); ks(k, value);
        };
        auto fv = [&](const char* key, long long value) {
            snprintf(k, sizeof(k), "%s%s", p, key); kv(k, value);
        };
        fs("filename", f.filename);
        fs("search_key", f.search_key);
        fs("formal_name_faction", f.formal_name_faction);
        fs("desc_name_faction", f.desc_name_faction);
        fs("noun_faction", f.noun_faction);
        fv("noun_gender", f.noun_gender);
        fv("is_noun_plural", f.is_noun_plural);
        fs("name_leader", f.name_leader);
        fv("is_leader_female", f.is_leader_female);
        fv("ai_fight", f.AI_fight);
        fv("ai_power", f.AI_power);
        fv("ai_tech", f.AI_tech);
        fv("ai_wealth", f.AI_wealth);
        fv("ai_growth", f.AI_growth);
        fv("rule_tech_selected", f.rule_tech_selected);
        fv("rule_morale", f.rule_morale);
        fv("rule_research", f.rule_research);
        fv("rule_drone", f.rule_drone);
        fv("rule_talent", f.rule_talent);
        fv("rule_energy", f.rule_energy);
        fv("rule_interest", f.rule_interest);
        fv("rule_population", f.rule_population);
        fv("rule_hurry", f.rule_hurry);
        fv("rule_techcost", f.rule_techcost);
        fv("rule_psi", f.rule_psi);
        fv("rule_sharetech", f.rule_sharetech);
        fv("rule_commerce", f.rule_commerce);
        fv("rule_flags", f.rule_flags);
        fv("bonus_count", f.faction_bonus_count);
        for (int b = 0; b < f.faction_bonus_count && b < MaxBonusNum; b++) {
            char key[48];
            snprintf(key, sizeof(key), "bonus.%d.kind", b); fv(key, f.faction_bonus_id[b]);
            snprintf(key, sizeof(key), "bonus.%d.value", b); fv(key, f.faction_bonus_val1[b]);
            snprintf(key, sizeof(key), "bonus.%d.modifier", b);
            fv(key, f.faction_bonus_id[b] == RULE_SOCIAL || f.faction_bonus_id[b] == RULE_ROBUST
                || f.faction_bonus_id[b] == RULE_IMMUNITY || f.faction_bonus_id[b] == RULE_IMPUNITY
                || f.faction_bonus_id[b] == RULE_PENALTY ? f.faction_bonus_val2[b] : 0);
        }
        fv("ideology.0.category", f.soc_priority_category);
        fv("ideology.0.model", f.soc_priority_model);
        fv("ideology.0.effect", f.soc_priority_effect);
        fv("ideology.1.category", f.soc_opposition_category);
        fv("ideology.1.model", f.soc_opposition_model);
        fv("ideology.1.effect", f.soc_opposition_effect);
        fs("adj_name_faction", f.adj_name_faction);
        fs("assistant_name", f.assistant_name);
        fs("scientist_name", f.scientist_name);
        fs("assistant_city", f.assistant_city);
        fs("title_leader", f.title_leader);
        fs("adj_leader", f.adj_leader);
        fs("adj_insult_leader", f.adj_insult_leader);
        fs("adj_faction", f.adj_faction);
        fs("adj_insult_faction", f.adj_insult_faction);
        fs("insult_leader", f.insult_leader);
    }
#undef KV
#undef KS
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "unit")) {
        return unit_checks();
    }
    if (argc >= 4 && !strcmp(argv[1], "load")) {
        int expansion = !strcmp(argv[3], "expansion");
        bool twice = argc >= 5 && !strcmp(argv[4], "twice");
        std::unique_ptr<EngineState> s(new EngineState());
        engine_reset(*s);
        engine_bind(s.get());
        game_srand(1);
        random_reseed(1);
        if (engine_load_rules(argv[2], expansion)) {
            printf("error=%s\n", engine_rules_error());
            return 20;
        }
        if (twice) {
            /* A second state loaded with the other variant must not change
             * the record tables of the first; reloading the first gives the
             * same tables (checked by the caller comparing outputs). */
            std::unique_ptr<EngineState> other(new EngineState());
            engine_reset(*other);
            engine_bind(other.get());
            engine_load_rules(argv[2], !expansion);
            engine_bind(s.get());
            if (engine_load_rules(argv[2], expansion)) {
                printf("error=%s\n", engine_rules_error());
                return 20;
            }
        }
        dump();
        engine_bind(nullptr);
        return 0;
    }
    fprintf(stderr, "usage: %s unit | load DIR base|expansion [twice]\n", argv[0]);
    return 2;
}
