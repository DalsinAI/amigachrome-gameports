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
  }
  return "unknown error";
}
}
