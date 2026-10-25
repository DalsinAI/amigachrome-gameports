/* SPDX-License-Identifier: GPL-3.0-or-later
 * See rules_text.h for attribution. Follows OpenSMACX's two-pass read_tech
 * and comma-token interface; replaces Windows I/O, string tables and absolute
 * addresses with bounded memory input and caller-owned state.
 */
#include "rules_text.h"
namespace alpha {
static unsigned char lower(unsigned char c) {
  return c >= 'A' && c <= 'Z' ? (unsigned char)(c + ('a'-'A')) : c;
}
static bool equal(const char* a, const char* b) {
  while (*a && lower((unsigned char)*a) == lower((unsigned char)*b)) { ++a; ++b; }
  return lower((unsigned char)*a) == lower((unsigned char)*b);
}
static bool space(unsigned char c) { return c == ' ' || c == '\t'; }
Text::Text(const uint8_t* bytes, size_t length)
  : bytes_(bytes), length_(length), position_(0), cursor_(0), error_(Error::ok) {
  line_[0] = 0;
}
bool Text::next() {
  if (error_ != Error::ok || position_ >= length_) return false;
  size_t n = 0;
  while (position_ < length_ && bytes_[position_] != '\n' && bytes_[position_] != '\r') {
    const uint8_t c = bytes_[position_++];
    if (!c) { error_ = Error::bad_row; return false; }
    if (n + 1 >= sizeof(line_)) { error_ = Error::line_too_long; return false; }
    line_[n++] = (char)c;
  }
  if (position_ < length_) {
    const uint8_t c = bytes_[position_++];
    if (c == '\r' && position_ < length_ && bytes_[position_] == '\n') ++position_;
  }
  while (n && space((unsigned char)line_[n-1])) --n;
  size_t begin = 0;
  while (begin < n && space((unsigned char)line_[begin])) ++begin;
  for (size_t i = begin; i < n; ++i) line_[i-begin] = line_[i];
  line_[n-begin] = 0;
  cursor_ = 0;
  return true;
}
bool Text::section(const char* name) {
  position_ = cursor_ = 0; error_ = Error::ok;
  if (*name == '#') ++name;
  while (next()) if (line_[0] == '#' && equal(line_+1, name)) return true;
  if (error_ == Error::ok) error_ = Error::missing_section;
  return false;
}
bool Text::item(char* output, size_t capacity) {
  if (!capacity || error_ != Error::ok) return false;
  size_t first = cursor_;
  while (line_[first] && space((unsigned char)line_[first])) ++first;
  size_t end = first;
  while (line_[end] && line_[end] != ',' && line_[end] != ';') ++end;
  cursor_ = line_[end] == ',' ? end+1 : end;
  size_t last = end;
  while (last > first && space((unsigned char)line_[last-1])) --last;
  if (last-first >= capacity) { error_ = Error::bad_row; return false; }
  for (size_t i = first; i < last; ++i) output[i-first] = line_[i];
  output[last-first] = 0;
  return true;
}
bool Text::end_items() const {
  if (error_ != Error::ok) return false;
  size_t p = cursor_;
  while (space((unsigned char)line_[p])) ++p;
  return !line_[p] || line_[p] == ';';
}
static bool number(const char* p, int32_t& result) {
  bool negative = false;
  if (*p == '-' || *p == '+') negative = *p++ == '-';
  if (!*p) return false;
  uint32_t base = 10;
  if (!negative && p[0] == '0' && p[1]) {
    unsigned char prefix = lower((unsigned char)p[1]);
    if (prefix == 'b' || prefix == 'x' || prefix == 'd') {
      base = prefix == 'b' ? 2u : prefix == 'x' ? 16u : 10u;
      p += 2;
      if (!*p) return false;
    }
  }
  uint32_t n = 0, limit = negative ? 0x80000000u : 0x7fffffffu;
  do {
    unsigned char c = lower((unsigned char)*p++);
    uint32_t digit;
    if (c >= '0' && c <= '9') digit = c-'0';
    else if (c >= 'a' && c <= 'f') digit = c-'a'+10u;
    else return false;
    if (digit >= base || n > (limit-digit)/base) return false;
    n = n*base + digit;
  } while (*p);
  result = negative ? (n == 0x80000000u ? (-2147483647-1) : -(int32_t)n) : (int32_t)n;
  return true;
}
static bool binary(const char* p, uint32_t& result) {
  if (!*p) return false;
  uint32_t n = 0;
  do {
    if ((*p != '0' && *p != '1') || n > 0x7fffffffu) return false;
    n = n*2u + (uint32_t)(*p++-'0');
  } while (*p);
  result = n; return true;
}
static bool data_line(Text& text) {
  while (text.next()) if (*text.line() && *text.line() != ';') return true;
  return false;
}
Error read_technologies(const uint8_t* bytes, size_t length, Rules& out) {
  out.count = out.active = out.edges = 0;
  if (!bytes || !length) return Error::missing_section;
  Text text(bytes, length);
  if (!text.section("TECHNOLOGY")) return text.error();
  /* Preserve the source's fixed row count and resolve forward keys in pass 2. */
  while (data_line(text)) {
    if (text.line()[0] == '#') break;
    if (out.count == technology_count) return Error::wrong_count;
    Technology& t = out.technologies[out.count];
    char token[128];
    if (!text.item(t.name, sizeof(t.name)) || !*t.name ||
        !text.item(t.key, sizeof(t.key)) || !*t.key) return Error::bad_key;
    for (size_t j = 0; j < out.count; ++j)
      if (equal(t.key, out.technologies[j].key)) return Error::duplicate_key;
    for (unsigned i = 0; i < 4; ++i) {
      if (!text.item(token, sizeof(token)) || !number(token, t.priorities[i])) return Error::bad_number;
    }
    for (unsigned i = 0; i < 2; ++i)
      if (!text.item(t.prerequisite_key[i], sizeof(t.prerequisite_key[i])) ||
          !*t.prerequisite_key[i]) return Error::bad_key;
    if (!text.item(token, sizeof(token)) || !binary(token, t.flags)) return Error::bad_number;
    if (!text.end_items()) return Error::bad_row;
    ++out.count;
  }
  if (text.error() != Error::ok) return text.error();
  if (out.count != technology_count) return Error::wrong_count;
  for (size_t row = 0; row < out.count; ++row) {
    Technology& t = out.technologies[row];
    for (unsigned i = 0; i < 2; ++i) {
      const char* key = t.prerequisite_key[i];
      if (equal(key, "None")) t.prerequisites[i] = -1;
      else if (equal(key, "Disable")) t.prerequisites[i] = -2;
      else {
        size_t found = 0;
        while (found < out.count && !equal(key, out.technologies[found].key)) ++found;
        if (found == out.count) return Error::unknown_prerequisite;
        t.prerequisites[i] = (int32_t)found;
      }
    }
    if (t.prerequisites[0] != -2 && t.prerequisites[1] != -2) {
      ++out.active;
      out.edges += (t.prerequisites[0] >= 0) + (t.prerequisites[1] >= 0);
    }
  }
  return Error::ok;
}
/* A new input row cannot discard unconsumed tokens from the previous row.
 * Reaching another section early is a structural error, never a numeric zero.
 */
class RuleInput {
 public:
  explicit RuleInput(Text& text) : text_(text), pending_(false), error_(Error::ok) {}
  bool row() {
    if (!end_row()) return false;
    if (!data_line(text_)) {
      error_ = text_.error() == Error::ok ? Error::bad_row : text_.error();
      return false;
    }
    if (text_.line()[0] == '#') { error_ = Error::bad_row; return false; }
    pending_ = true;
    return true;
  }
  bool numeric(int32_t& value, int32_t low, int32_t high) {
    char token[128];
    if (!text_.item(token, sizeof(token)) || !number(token, value)) {
      error_ = Error::bad_number; return false;
    }
    value = value < low ? low : value > high ? high : value;
    return true;
  }
  bool technology(int32_t& value, const Rules& rules) {
    if (!row()) return false;
    char token[8];
    if (!text_.item(token, sizeof(token)) || !*token) { error_ = Error::bad_key; return false; }
    if (equal(token, "None")) { value = -1; return true; }
    if (equal(token, "Disable")) { value = -2; return true; }
    for (size_t i = 0; i < rules.count; ++i)
      if (equal(token, rules.technologies[i].key)) { value = (int32_t)i; return true; }
    error_ = Error::unknown_prerequisite;
    return false;
  }
  bool finish() {
    if (!end_row()) return false;
    if (data_line(text_) && text_.line()[0] != '#') { error_ = Error::bad_row; return false; }
    if (text_.error() != Error::ok) { error_ = text_.error(); return false; }
    return true;
  }
  Error error() const { return error_; }
 private:
  bool end_row() {
    if (!pending_) return true;
    if (!text_.end_items()) { error_ = Error::bad_row; return false; }
    pending_ = false;
    return true;
  }
  Text& text_;
  bool pending_;
  Error error_;
};
Error read_resource_info(const uint8_t* bytes,size_t length,ResourceInfo& output) {
  if(!bytes || !length)return Error::missing_section;
  Text text(bytes,length);if(!text.section("RESOURCEINFO"))return text.error();
  ResourceInfo result{};
  for(size_t i=0;i<resource_info_count;++i) {
    if(!data_line(text) || text.line()[0]=='#')return text.error()==Error::ok?Error::wrong_count:text.error();
    char token[128];if(!text.item(token,sizeof(token)) || !*token)return Error::bad_row;
    int32_t* values[]={&result.rows[i].nutrients,&result.rows[i].minerals,&result.rows[i].energy,&result.rows[i].unused};
    for(unsigned column=0;column<4;++column){
      if(!text.item(token,sizeof(token)))return Error::bad_number;
      // The original table marks the unused improved-land mineral/energy
      // columns "*". Source text_item_number converts these to zero.
      if(i==5 && (column==1 || column==2) && equal(token,"*"))*values[column]=0;
      else if(!number(token,*values[column]))return Error::bad_number;
    }
    // Source reads exactly four data columns; the remaining text is annotation
    // (including commas), not another resource field. Do not parse it as data.
  }
  if(data_line(text) && text.line()[0]!='#')return Error::wrong_count;
  if(text.error()!=Error::ok)return text.error();
  output=result;return Error::ok;
}
Error read_basic_rules(const uint8_t* bytes, size_t length, const Rules& technologies,
                       Variant variant, BasicRules& basic, WorldRules& world) {
  if (!bytes || !length) return Error::missing_section;
  if (technologies.count != technology_count) return Error::wrong_count;
  if (variant != Variant::base && variant != Variant::alien_crossfire) return Error::bad_row;
  basic = BasicRules{};
  world = WorldRules{};
  Text text(bytes, length);
  if (!text.section("RULES")) return text.error();
  RuleInput input(text);
#define RULE_ROW(name, low, high) if (!input.row() || !input.numeric(basic.name, low, high)) return input.error();
#define RULE_ITEM(name, low, high) if (!input.numeric(basic.name, low, high)) return input.error();
#define RULE_TECH(name) if (!input.technology(basic.name, technologies)) return input.error();
#include "basic_fields.inc"
#undef RULE_ROW
#undef RULE_ITEM
#undef RULE_TECH
  // Base Alpha Centauri has no subspace-generator fields. Do not read the
  // following section as two extra numbers or silently accept a SMAX ruleset.
  if (variant == Variant::alien_crossfire) {
    if (!input.row() || !input.numeric(basic.size_base_subspace_gen, 1, 999) ||
        !input.row() || !input.numeric(basic.subspace_generators_needed, 1, 999)) return input.error();
  }
  if (!input.finish()) return input.error();
  if (!text.section("WORLDBUILDER")) return text.error();
  RuleInput world_input(text);
#define RULE_ROW(name, low, high) if (!world_input.row() || !world_input.numeric(world.name, low, high)) return world_input.error();
#define RULE_ITEM(name, low, high) if (!world_input.numeric(world.name, low, high)) return world_input.error();
#define RULE_TECH(name) static_assert(false, "World rules must not contain technology references");
#include "world_fields.inc"
#undef RULE_ROW
#undef RULE_ITEM
#undef RULE_TECH
  return world_input.finish() ? Error::ok : world_input.error();
}
template<size_t N, size_t W>
static int find_key(const char (&keys)[N][W], const char* key, size_t count = N) {
  if (!key) return -1;
  for (size_t i = 0; i < count; ++i) if (equal(keys[i], key)) return (int)i;
  return -1;
}
int social_effect(const SocialRules& state, const char* key) {
  return find_key(state.effect_keys, key);
}
int social_category(const SocialRules& state, const char* key) {
  return find_key(state.categories, key);
}
int social_model(const SocialRules& state, size_t category, const char* key) {
  if (category >= social_category_count || !key) return -1;
  for (size_t i = 0; i < social_model_count; ++i)
    if (equal(state.models[category][i].name, key)) return (int)i;
  return -1;
}
int bonus_kind(const SocialRules& state, const char* key) {
  if (state.bonus_count > maximum_bonus_keys) return -1;
  return find_key(state.bonus_keys, key, state.bonus_count);
}
static Error next_rule_row(Text& text) {
  if (!data_line(text)) return text.error() == Error::ok ? Error::bad_row : text.error();
  return text.line()[0] == '#' ? Error::bad_row : Error::ok;
}
template<size_t N, size_t W>
static Error key_row(Text& text, char (&keys)[N][W], bool unique) {
  Error error = next_rule_row(text);
  if (error != Error::ok) return error;
  for (size_t i = 0; i < N; ++i) {
    if (!text.item(keys[i], W) || !*keys[i]) return Error::bad_key;
    if (unique && find_key(keys, keys[i], i) != -1) return Error::duplicate_key;
  }
  return text.end_items() ? Error::ok : Error::bad_row;
}
Error read_social_rules(const uint8_t* bytes, size_t length, const Rules& technologies,
                        Variant variant, SocialRules& out) {
  if (!bytes || !length) return Error::missing_section;
  if (technologies.count != technology_count) return Error::wrong_count;
  if (variant != Variant::base && variant != Variant::alien_crossfire) return Error::bad_row;
  out = SocialRules{};
  Text text(bytes, length);
  if (!text.section("SOCIO")) return text.error();
  Error error = key_row(text, out.effect_keys, true);
  if (error != Error::ok) return error;
  error = key_row(text, out.effect_labels, false);
  if (error != Error::ok) return error;
  error = key_row(text, out.categories, true);
  if (error != Error::ok) return error;
  for (size_t cat = 0; cat < social_category_count; ++cat) {
    for (size_t mod = 0; mod < social_model_count; ++mod) {
      error = next_rule_row(text);
      if (error != Error::ok) return error;
      SocialModel& model = out.models[cat][mod];
      if (!text.item(model.name, sizeof(model.name)) || !*model.name) return Error::bad_key;
      for (size_t prior = 0; prior < mod; ++prior)
        if (equal(out.models[cat][prior].name, model.name)) return Error::duplicate_key;
      char token[128];
      if (!text.item(token, sizeof(token)) || !*token) return Error::bad_key;
      if (equal(token, "None")) model.prerequisite = -1;
      else if (equal(token, "Disable")) model.prerequisite = -2;
      else {
        size_t i = 0;
        while (i < technologies.count && !equal(token, technologies.technologies[i].key)) ++i;
        if (i == technologies.count) return Error::unknown_prerequisite;
        model.prerequisite = (int32_t)i;
      }
      while (!text.end_items()) {
        if (!text.item(token, sizeof(token)) || !*token) return Error::bad_key;
        const char* key = token;
        int32_t amount = 0;
        while (*key == '+' || *key == '-') amount += *key++ == '+' ? 1 : -1;
        const int effect = social_effect(out, key);
        if (effect < 0) return Error::bad_key;
        // As upstream: repeated effects overwrite; an unsigned key means 0.
        // Faction SOCIAL bonuses have a different zero rule, implemented later.
        model.effects[effect] = amount;
      }
    }
  }
  if (data_line(text) && text.line()[0] != '#') return Error::bad_row;
  if (text.error() != Error::ok) return text.error();
  if (!text.section("BONUSNAMES")) return text.error();
  const size_t count = variant == Variant::base ? 29 : maximum_bonus_keys;
  for (size_t i = 0; i < count; ++i) {
    if (!(i % 8)) {
      if (i && !text.end_items()) return Error::bad_row;
      error = next_rule_row(text);
      if (error != Error::ok) return error;
    }
    if (!text.item(out.bonus_keys[i], sizeof(out.bonus_keys[i])) || !*out.bonus_keys[i]) return Error::bad_key;
    if (find_key(out.bonus_keys, out.bonus_keys[i], i) != -1) return Error::duplicate_key;
  }
  if (!text.end_items()) return Error::bad_row;
  if (data_line(text) && text.line()[0] != '#') return Error::bad_row;
  if (text.error() != Error::ok) return text.error();
  out.bonus_count = count;
  return Error::ok;
}
static bool faction_number(Text& text, int32_t& value) {
  char token[128];
  return text.item(token, sizeof(token)) && number(token, value);
}
static int technology_index(const Rules& rules, const char* key) {
  if (equal(key, "None")) return -1;
  if (equal(key, "Disable")) return -2;
  for (size_t i = 0; i < rules.count; ++i)
    if (equal(key, rules.technologies[i].key)) return (int)i;
  return -3;
}
static bool ideology_equal(const char* a, const char* b, IdeologyMatch matching) {
  if (matching == IdeologyMatch::exact) return equal(a, b);
  for (size_t i = 0; i < 4; ++i) {
    if (lower((unsigned char)a[i]) != lower((unsigned char)b[i])) return false;
    if (!a[i]) return true;
  }
  return true;
}
static bool no_ideology(const char* key) { return equal(key, "nil") || equal(key, "None"); }
Error read_faction(const uint8_t* bytes, size_t length, const char* search_section,
                   const char* filename_section, const Rules& technologies,
                   const SocialRules& social, IdeologyMatch matching, Faction& out) {
  if (!bytes || !length || !search_section || !*search_section || !filename_section || !*filename_section)
    return Error::missing_section;
  if (technologies.count != technology_count || (social.bonus_count != 29 && social.bonus_count != 41))
    return Error::wrong_count;
  if (matching != IdeologyMatch::exact && matching != IdeologyMatch::localized_first_four) return Error::bad_row;
  out = Faction{};
  out.rule_hurry = out.rule_techcost = 100;
  for (auto& ideology : out.ideology) { ideology.category = ideology.effect = -1; }
  Text text(bytes, length);
  if (!text.section(search_section)) {
    if (text.error() != Error::missing_section) return text.error();
    if (!text.section(filename_section)) return text.error();
  }
  Error error = next_rule_row(text);
  if (error != Error::ok) return error;
#define FACTION_STRING(field) if (!text.item(out.field, sizeof(out.field)) || !*out.field) return Error::bad_row
#define FACTION_NUMBER(field) if (!faction_number(text, out.field)) return Error::bad_number
  FACTION_STRING(formal_name_faction);
  FACTION_STRING(desc_name_faction);
  FACTION_STRING(noun_faction);
  char gender[24];
  if (!text.item(gender, sizeof(gender)) || !*gender) return Error::bad_key;
  out.noun_gender = lower((unsigned char)*gender) == 'f' ? 1 : lower((unsigned char)*gender) == 'n' ? 2 : 0;
  int32_t plural;
  if (!faction_number(text, plural)) return Error::bad_number;
  // Avoid signed overflow in upstream's number - 1 before clamping to Boolean.
  out.is_noun_plural = plural >= 2;
  FACTION_STRING(name_leader);
  if (!text.item(gender, sizeof(gender)) || !*gender) return Error::bad_key;
  out.is_leader_female = lower((unsigned char)*gender) == 'f';
  FACTION_NUMBER(ai_fight); FACTION_NUMBER(ai_power); FACTION_NUMBER(ai_tech);
  FACTION_NUMBER(ai_wealth); FACTION_NUMBER(ai_growth);
  if (!text.end_items()) return Error::bad_row;
  error = next_rule_row(text);
  if (error != Error::ok) return error;
  // Require a pair-bearing bonus row, as in all original factions. Empty
  // physical rows are whitespace and must not shift the ideology records.
  while (!text.end_items()) {
    char key[24], parameter[128];
    if (!text.item(key, sizeof(key)) || !*key ||
        !text.item(parameter, sizeof(parameter)) || !*parameter) return Error::bad_key;
    const int kind = bonus_kind(social, key);
    if (kind < 0) return Error::bad_key;
    int32_t value = 0, second = 0;
    int slot = -1;
    // BONUSNAMES position, not English spelling, determines the operation.
    switch (kind) {
      case 0:
        if (number(parameter, value) && value != 0) out.rule_tech_selected = value;
        else {
          value = technology_index(technologies, parameter);
          if (value == -3) return Error::unknown_prerequisite;
          slot = 0;
        }
        break;
      case 1:
        if (!number(parameter, out.rule_morale)) return Error::bad_number;
        if (!out.rule_morale) out.rule_flags |= 0x8000u;
        break;
      case 2: if (!number(parameter, out.rule_psi)) return Error::bad_number; break;
      case 4: if (!number(parameter, out.rule_research)) return Error::bad_number; break;
      case 5: if (!number(parameter, out.rule_drone)) return Error::bad_number; break;
      case 6: if (!number(parameter, out.rule_talent)) return Error::bad_number; break;
      case 7: if (!number(parameter, out.rule_energy)) return Error::bad_number; break;
      case 8:
        if (!number(parameter, out.rule_interest)) return Error::bad_number;
        out.rule_flags |= 0x4000u; break;
      case 9: if (!number(parameter, out.rule_commerce)) return Error::bad_number; break;
      case 10: if (!number(parameter, out.rule_population)) return Error::bad_number; break;
      case 11:
        if (!number(parameter, value)) return Error::bad_number;
        out.rule_hurry = value < 1 ? 1 : value > 1000 ? 1000 : value; break;
      case 13: if (!number(parameter, out.rule_techcost)) return Error::bad_number; break;
      case 14: if (!number(parameter, out.rule_sharetech)) return Error::bad_number; break;
      case 16: case 17: case 18: {
        const char* effect = parameter;
        while (*effect == '+' || *effect == '-') second += *effect++ == '+' ? 1 : -1;
        if (!second) second = 1;
        value = social_effect(social, effect);
        if (value < 0) return Error::bad_key;
        slot = kind == 16 ? 3 : kind == 17 ? 10 : 4;
        break;
      }
      case 19: case 20: {
        bool found = false;
        for (size_t cat = 0; cat < social_category_count; ++cat) {
          const int model = social_model(social, cat, parameter);
          if (model < 0) continue;
          if (found) return Error::duplicate_key; // ambiguous model in different categories
          found = true; value = (int32_t)cat; second = model;
        }
        if (!found) return Error::bad_key;
        slot = kind == 19 ? 5 : 6;
        break;
      }
      case 3: case 12: case 21: case 22: case 23: case 27:
      case 31: case 32: case 33: case 35: case 36: case 37: case 38:
        if (!number(parameter, value)) return Error::bad_number;
        switch (kind) {
          case 3: slot=2; break; case 12: slot=1; break;
          case 21: slot=7; break; case 22: slot=8; break; case 23: slot=9; break;
          case 27: slot=11; break; case 31: slot=12; break; case 32: slot=13; break;
          case 33: slot=14; break; case 35: slot=15; break; case 36: slot=16; break;
          case 37: slot=17; break; case 38: slot=18; break;
        }
        break;
      case 39:
        if (!number(parameter, out.rule_sharetech)) return Error::bad_number;
        out.rule_flags |= 0x20u; break;
      default: {
        uint32_t flag = 0;
        switch (kind) {
          case 15: flag=0x2000u; break; case 24: flag=0x1000u; break;
          case 25: flag=0x800u; break; case 26: flag=0x400u; break;
          case 28: flag=0x200u; break; case 29: flag=0x100u; break;
          case 30: flag=0x80u; break; case 34: flag=0x40u; break;
          case 40: flag=0x10u; break; default: return Error::bad_key;
        }
        out.rule_flags |= flag; break; // upstream ignores the flag parameter's value
      }
    }
    if (slot >= 0) {
      if (out.bonus_count == 8) return Error::too_many_bonuses;
      out.bonuses[out.bonus_count++] = { (FactionBonusKind)slot, value, second };
    }
  }
  for (auto& ideology : out.ideology) {
    error = next_rule_row(text);
    if (error != Error::ok) return error;
    char category[128], model[128], effect[24];
    if (!text.item(category, sizeof(category)) || !*category ||
        !text.item(model, sizeof(model)) || !*model ||
        !text.item(effect, sizeof(effect)) || !*effect || !text.end_items()) return Error::bad_row;
    if (!no_ideology(category)) {
      for (size_t i = 0; i < social_category_count; ++i)
        if (ideology_equal(category, social.categories[i], matching)) {
          if (ideology.category != -1) return Error::duplicate_key;
          ideology.category = (int32_t)i;
        }
      if (ideology.category == -1) return Error::bad_key;
      bool found = false;
      for (size_t i = 0; i < social_model_count; ++i)
        if (ideology_equal(model, social.models[ideology.category][i].name, matching)) {
          if (found) return Error::duplicate_key;
          found = true; ideology.model = (int32_t)i;
        }
      if (!found) return Error::bad_key;
    } else if (!no_ideology(model)) return Error::bad_key;
    if (!no_ideology(effect)) {
      ideology.effect = social_effect(social, effect);
      if (ideology.effect < 0) return Error::bad_key;
    }
  }
  error = next_rule_row(text); if (error != Error::ok) return error;
  FACTION_STRING(adj_name_faction);
  // Upstream ignores the optional abbreviation; retain it as owned data.
  if (!text.item(out.abbreviation, sizeof(out.abbreviation)) || !text.end_items()) return Error::bad_row;
  error = next_rule_row(text); if (error != Error::ok) return error;
  FACTION_STRING(assistant_name); FACTION_STRING(scientist_name); FACTION_STRING(assistant_city);
  if (!text.end_items()) return Error::bad_row;
  error = next_rule_row(text); if (error != Error::ok) return error;
  FACTION_STRING(title_leader); FACTION_STRING(adj_leader); FACTION_STRING(adj_insult_leader);
  FACTION_STRING(adj_faction); FACTION_STRING(adj_insult_faction);
  if (!text.end_items()) return Error::bad_row;
  error = next_rule_row(text); if (error != Error::ok) return error;
  FACTION_STRING(insult_leader);
#undef FACTION_STRING
#undef FACTION_NUMBER
  return text.end_items() ? Error::ok : Error::bad_row;
}
class DefinitionInput {
 public:
  DefinitionInput(Text& text, const Rules& technology) : text_(text), technology_(technology), error_(Error::ok) {}
  Error error() const { return error_; }
  bool section(const char* name) {
    if (!text_.section(name)) { error_ = text_.error(); return false; }
    return true;
  }
  bool row() { error_ = next_rule_row(text_); return error_ == Error::ok; }
  bool end() {
    if (!text_.end_items()) { error_ = Error::bad_row; return false; }
    return true;
  }
  bool finish() {
    if (!end()) return false;
    if (data_line(text_) && text_.line()[0] != '#') { error_ = Error::bad_row; return false; }
    if (text_.error() != Error::ok) { error_ = text_.error(); return false; }
    return true;
  }
  template<size_t N> bool string(char (&value)[N], bool required = true) {
    if (!text_.item(value, N) || (required && !*value)) { error_ = Error::bad_row; return false; }
    return true;
  }
  bool numeric(int32_t& value, int32_t low, int32_t high) {
    char token[128];
    if (!string(token) || !number(token, value) || value < low || value > high) {
      error_ = Error::bad_number; return false;
    }
    return true;
  }
  bool technology(int32_t& value) {
    char token[8];
    if (!string(token)) { error_ = Error::bad_key; return false; }
    value = technology_index(technology_, token);
    if (value == -3) { error_ = Error::unknown_prerequisite; return false; }
    return true;
  }
  bool bits(uint32_t& value) {
    char token[128];
    if (!string(token) || !binary(token, value)) { error_ = Error::bad_number; return false; }
    return true;
  }
  bool noun(ChassisNoun& value) {
    char grammar[24];
    if (!string(value.name) || !string(grammar)) return false;
    value.gender = lower((unsigned char)grammar[0]) == 'f' ? 1 : lower((unsigned char)grammar[0]) == 'n' ? 2 : 0;
    value.plural = grammar[1] == '2';
    return true;
  }
 private:
  Text& text_;
  const Rules& technology_;
  Error error_;
};
Error read_unit_definitions(const uint8_t* bytes, size_t length, const Rules& technologies,
                            Variant variant, UnitDefinitions& out) {
  if (!bytes || !length) return Error::missing_section;
  if (technologies.count != technology_count) return Error::wrong_count;
  if (variant != Variant::base && variant != Variant::alien_crossfire) return Error::bad_row;
  out = UnitDefinitions{};
  const bool expansion = variant == Variant::alien_crossfire;
  out.weapon_count = expansion ? 26 : 21;
  out.armor_count = expansion ? 14 : 10;
  out.ability_count = expansion ? 29 : 24;
  Text text(bytes, length);
  DefinitionInput input(text, technologies);
  if (!input.section("CHASSIS")) return input.error();
  for (size_t i = 0; i < 9; ++i) {
    auto& c = out.chassis[i];
    if (!input.row()) return input.error();
    for (size_t j = 0; j < 4; ++j) if (!input.noun(c.nouns[j])) return input.error();
    for (size_t j = 0; j < i; ++j) if (equal(out.chassis[j].nouns[0].name, c.nouns[0].name)) return Error::duplicate_key;
    if (!input.numeric(c.speed,0,255) || !input.numeric(c.triad,0,2) ||
        !input.numeric(c.range,0,255) || !input.numeric(c.missile,0,1) ||
        !input.numeric(c.cargo,0,255) || !input.numeric(c.cost,0,255) || !input.technology(c.prerequisite) ||
        !input.noun(c.nouns[4]) || !input.noun(c.nouns[5]) || !input.end()) return input.error();
  }
  if (!input.finish() || !input.section("REACTORS")) return input.error();
  for (auto& r : out.reactors) {
    if (!input.row() || !input.string(r.name) || !input.string(r.short_name) ||
        !input.numeric(r.power,0,65535) || !input.technology(r.prerequisite) || !input.end()) return input.error();
  }
  if (!input.finish() || !input.section("WEAPONS")) return input.error();
  for (size_t i = 0; i < out.weapon_count; ++i) {
    auto& w = out.weapons[i];
    if (!input.row() || !input.string(w.name) || !input.string(w.short_name) ||
        !input.numeric(w.offense,-128,127) || !input.numeric(w.mode,0,255) ||
        !input.numeric(w.cost,0,255) || !input.numeric(w.icon,-128,127) ||
        !input.technology(w.prerequisite) || !input.end()) return input.error();
    for (size_t j = 0; j < i; ++j) if (equal(out.weapons[j].short_name,w.short_name)) return Error::duplicate_key;
  }
  if (!input.finish() || !input.section("DEFENSES")) return input.error();
  for (size_t i = 0; i < out.armor_count; ++i) {
    auto& a = out.armor[i];
    if (!input.row() || !input.string(a.name) || !input.string(a.short_name) ||
        !input.numeric(a.defense,-128,127) || !input.numeric(a.mode,0,255) ||
        !input.numeric(a.cost,0,255) || !input.technology(a.prerequisite) || !input.end()) return input.error();
    for (size_t j = 0; j < i; ++j) if (equal(out.armor[j].short_name,a.short_name)) return Error::duplicate_key;
  }
  if (!input.finish() || !input.section("ABILITIES")) return input.error();
  for (size_t i = 0; i < out.ability_count; ++i) {
    auto& a = out.abilities[i];
    if (!input.row() || !input.string(a.name) || !input.numeric(a.cost_factor,(-2147483647-1),2147483647) ||
        !input.technology(a.prerequisite) || !input.string(a.abbreviation,false) ||
        !input.bits(a.flags) || !input.string(a.description,false) || !input.end()) return input.error();
  }
  if (!input.finish() || !input.section("UNITS") || !input.row()) return input.error();
  int32_t total;
  if (!input.numeric(total,0,64) || !input.end()) return input.error();
  out.unit_count = (size_t)total;
  for (size_t i = 0; i < out.unit_count; ++i) {
    auto& u = out.units[i];
    char chassis[128], weapon[128], armor[128];
    if (!input.row() || !input.string(u.name) || !input.string(chassis) ||
        !input.string(weapon) || !input.string(armor)) return input.error();
    u.chassis = u.weapon = u.armor = -1;
    for (size_t j = 0; j < 9; ++j) if (equal(chassis,out.chassis[j].nouns[0].name)) u.chassis = (int32_t)j;
    for (size_t j = 0; j < out.weapon_count; ++j) if (equal(weapon,out.weapons[j].short_name)) u.weapon = (int32_t)j;
    for (size_t j = 0; j < out.armor_count; ++j) if (equal(armor,out.armor[j].short_name)) u.armor = (int32_t)j;
    if (u.chassis < 0 || u.weapon < 0 || u.armor < 0) return Error::bad_key;
    if (!input.numeric(u.plan,-1,255) || !input.numeric(u.cost,0,255) ||
        !input.numeric(u.carry,0,255) || !input.technology(u.prerequisite) ||
        !input.numeric(u.icon,-128,127) || !input.bits(u.abilities)) return input.error();
    int32_t reactor = 0;
    // Source enhancement: the final reactor column is optional, including a
    // single terminal comma. Preserve stock Ogre defaults by prototype index.
    if (!text.end_items() && !input.numeric(reactor,0,4)) return input.error();
    u.reactor = reactor ? reactor : i == 17 ? 2 : i == 18 ? 3 : 1;
    if (!input.end()) return input.error();
  }
  return input.finish() ? Error::ok : input.error();
}
const char* error_name(Error e) {
  switch(e) {
    case Error::ok: return "ok";
    case Error::missing_section: return "missing section";
    case Error::line_too_long: return "line too long";
    case Error::bad_row: return "bad row";
    case Error::bad_number: return "bad number";
    case Error::bad_key: return "bad key";
    case Error::duplicate_key: return "duplicate key";
    case Error::unknown_prerequisite: return "unknown prerequisite";
    case Error::wrong_count: return "wrong technology count";
    case Error::too_many_bonuses: return "too many faction bonuses";
  }
  return "unknown error";
}
}
