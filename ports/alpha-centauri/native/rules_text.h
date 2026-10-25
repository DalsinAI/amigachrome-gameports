/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native boundary adapted from OpenSMACX text.cpp/alpha.cpp.
 * OpenSMACX: Copyright (C) 2013-2021 Brendan Casey.
 * Native adaptation: Copyright (C) 2026 AmigaChrome contributors.
 */
#ifndef AC_ALPHA_RULES_TEXT_H
#define AC_ALPHA_RULES_TEXT_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
enum class Error { ok, missing_section, line_too_long, bad_row, bad_number,
                   bad_key, duplicate_key, unknown_prerequisite, wrong_count, too_many_bonuses };
class Text {
 public:
  Text(const uint8_t* bytes, size_t length);
  bool section(const char* name);
  bool next();
  bool item(char* output, size_t capacity);
  bool item_literal(char* output, size_t capacity);
  bool end_items() const;
  const char* line() const { return line_; }
  Error error() const { return error_; }
 private:
  bool item_impl(char*,size_t,bool semicolon_comments);
  const uint8_t* bytes_;
  size_t length_, position_, cursor_;
  char line_[512];
  Error error_;
};
constexpr size_t technology_count = 89;
struct Technology {
  char name[128], key[8], prerequisite_key[2][8];
  int32_t priorities[4], prerequisites[2];
  uint32_t flags;
};
struct Rules {
  Technology technologies[technology_count];
  size_t count, active, edges;
};
/* State is owned by the caller. File input and rendering stay outside this layer.
 * Failure does not qualify partially filled output: inspect only on Error::ok.
 */
Error read_technologies(const uint8_t* bytes, size_t length, Rules& output);
enum class Variant { base, alien_crossfire };
/* All values have explicit width. These are owned native objects, not the
 * original executable's packed structs or a serialized save-game format.
 * Expansion-only fields are zero in the base variant and must not be used.
 */
#define RULE_ROW(name, low, high) int32_t name;
#define RULE_ITEM(name, low, high) int32_t name;
#define RULE_TECH(name) int32_t name;
#define ALPHA_EXPANSION_FIELDS
struct BasicRules {
#include "basic_fields.inc"
};
#undef ALPHA_EXPANSION_FIELDS
struct WorldRules {
#include "world_fields.inc"
};
#undef RULE_ROW
#undef RULE_ITEM
#undef RULE_TECH
/* Requires a successfully parsed technology table from the same input.
 * Numeric ranges clamp like OpenSMACX; malformed tokens/rows are errors.
 * Both sections must succeed before either output may be used.
 */
Error read_basic_rules(const uint8_t* bytes, size_t length, const Rules& technologies,
                       Variant variant, BasicRules& basic, WorldRules& world);
constexpr size_t resource_info_count=9;
struct ResourceValues { int32_t nutrients,minerals,energy,unused; };
struct ResourceInfo { ResourceValues rows[resource_info_count]; };
/* RESOURCEINFO positional rows: ocean,base,bonus,forest,recycling,land
 * improvement,sea improvement,monolith,borehole. Signed source values remain
 * unmodified; consumers must validate domains before arithmetic. Trailing
 * annotations are ignored like the source; improved-land mineral/energy
 * wildcard columns are zero (unused). Other malformed numeric cells fail. */
Error read_resource_info(const uint8_t* bytes,size_t length,ResourceInfo& output);
constexpr size_t social_effect_count = 11;
constexpr size_t social_category_count = 4;
constexpr size_t social_model_count = 4;
constexpr size_t maximum_bonus_keys = 41;
struct SocialModel {
  char name[128];
  int32_t prerequisite, effects[social_effect_count];
};
struct SocialRules {
  char effect_keys[social_effect_count][24], effect_labels[social_effect_count][24];
  char categories[social_category_count][128];
  SocialModel models[social_category_count][social_model_count];
  char bonus_keys[maximum_bonus_keys][24];
  size_t bonus_count;
};
/* SOCIO and BONUSNAMES only. Base has 29 bonus keys; expansion has 41.
 * Requires the qualified technology table from the same input. Partial output
 * must not be used after failure. These lookups do not apply faction bonuses.
 */
Error read_social_rules(const uint8_t* bytes, size_t length, const Rules& technologies,
                        Variant variant, SocialRules& output);
/* Index or -1; ASCII case-insensitive. Use only successfully parsed state. */
int social_effect(const SocialRules& state, const char* key);
int social_category(const SocialRules& state, const char* key);
int social_model(const SocialRules& state, size_t category, const char* key);
int bonus_kind(const SocialRules& state, const char* key);
enum class FactionBonusKind : int32_t {
  tech=0, unit=1, facility=2, social=3, immunity=4, impunity=5, penalty=6,
  fungus_nutrient=7, fungus_minerals=8, fungus_energy=9, robust=10, votes=11,
  free_facility=12, revolt=13, no_drone=14, free_ability=15, probe_cost=16,
  defense=17, offense=18
};
struct FactionBonus { FactionBonusKind kind; int32_t value, modifier; };
struct Ideology { int32_t category, model, effect; };
struct Faction {
  char formal_name_faction[40], desc_name_faction[24], noun_faction[24], name_leader[24];
  int32_t noun_gender, is_noun_plural, is_leader_female;
  int32_t ai_fight, ai_power, ai_tech, ai_wealth, ai_growth;
  int32_t rule_tech_selected, rule_morale, rule_research, rule_drone, rule_talent,
    rule_energy, rule_interest, rule_population, rule_hurry, rule_techcost,
    rule_psi, rule_sharetech, rule_commerce;
  uint32_t rule_flags;
  FactionBonus bonuses[8];
  size_t bonus_count;
  Ideology ideology[2];
  char adj_name_faction[128], abbreviation[128];
  char assistant_name[24], scientist_name[24], assistant_city[24], title_leader[24];
  char adj_leader[128], adj_insult_leader[128], adj_faction[128], adj_insult_faction[128];
  char insult_leader[24];
};
enum class IdeologyMatch { exact, localized_first_four };
/* Reads the first eight faction rows; remaining narrative/art/base-name data
 * is outside this API. Search section first, then filename section on absence.
 * Strict numeric/unknown-key errors replace upstream atoi/silent omissions.
 * Uses OpenSMACX's corrected AI ideology-emphasis value, not original -1 bug.
 * Tables must be qualified together. Discard partial output after any error.
 */
Error read_faction(const uint8_t* bytes, size_t length, const char* search_section,
                   const char* filename_section, const Rules& technologies,
                   const SocialRules& social, IdeologyMatch matching, Faction& output);
struct ChassisNoun { char name[128]; int32_t gender, plural; };
struct ChassisDefinition {
  ChassisNoun nouns[6]; // offensive1/2, defensive1/2, large offensive/defensive
  int32_t speed, triad, range, missile, cargo, cost, prerequisite;
};
struct ReactorDefinition { char name[128], short_name[128]; int32_t power, prerequisite; };
struct WeaponDefinition {
  char name[128], short_name[128];
  int32_t offense, mode, cost, icon, prerequisite;
};
struct ArmorDefinition {
  char name[128], short_name[128];
  int32_t defense, mode, cost, prerequisite;
};
struct AbilityDefinition {
  char name[128], abbreviation[128], description[256];
  int32_t cost_factor, prerequisite;
  uint32_t flags;
};
struct UnitBlueprint {
  char name[32];
  int32_t chassis, weapon, armor, plan, cost, carry, prerequisite, icon, reactor;
  uint32_t abilities;
};
struct UnitDefinitions {
  ChassisDefinition chassis[9];
  ReactorDefinition reactors[4];
  WeaponDefinition weapons[26];
  ArmorDefinition armor[14];
  AbilityDefinition abilities[29];
  UnitBlueprint units[64];
  size_t weapon_count, armor_count, ability_count, unit_count;
};
/* Variant-specific component tables and up to64 declared blueprints. Unit cost,
 * plan/carry overrides remain raw: make_proto derivation is a separate step.
 * Qualified technology input required; discard partial output after failure.
 */
Error read_unit_definitions(const uint8_t* bytes, size_t length, const Rules& technologies,
                            Variant variant, UnitDefinitions& output);
constexpr size_t facility_capacity = 134;
struct FacilityDefinition {
  char name[128], effect[192];
  int32_t cost, maintenance, prerequisite;
  int32_t declared_free_technology, effective_free_technology;
  int32_t priorities[5]; // file order: fight, power, tech, wealth, growth
};
struct FacilityDefinitions {
  FacilityDefinition slots[facility_capacity];
  size_t slot_count, first_repeatable, first_project, project_count;
};
/* Original ordinal facility/project tables, slot0unused: base72slots, SMAX134.
 * Original loader overwrites every free-tech entry with Disable; retain the
 * configured token separately without enabling the later source enhancement.
 * Caller-owned storage; no allocation. Only inspect output after Error::ok;
 * on failure slot_count remains0 and partially decoded fields are unqualified.
 * Costs/maintenance/priorities retain signed32 raw values, not build legality.
 */
Error read_facility_definitions(const uint8_t*,size_t,const Rules&,Variant,
                                FacilityDefinitions&);
const char* error_name(Error error);
}
#endif
