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
const char* error_name(Error error);
}
#endif
