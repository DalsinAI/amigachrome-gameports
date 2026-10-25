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
    if (!text.item(token, sizeof(token)) || *token) return Error::bad_row;
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
