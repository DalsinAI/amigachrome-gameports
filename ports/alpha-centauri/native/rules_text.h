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
                   bad_key, duplicate_key, unknown_prerequisite, wrong_count };
class Text {
 public:
  Text(const uint8_t* bytes, size_t length);
  bool section(const char* name);
  bool next();
  bool item(char* output, size_t capacity);
  bool end_items() const;
  const char* line() const { return line_; }
  Error error() const { return error_; }
 private:
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
const char* error_name(Error error);
}
#endif
