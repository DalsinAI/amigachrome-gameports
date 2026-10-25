/* SPDX-License-Identifier: GPL-3.0-or-later
 * Test fingerprint only, not a game save format. Explicit byte order avoids
 * making a matching host/68k result depend on native struct layout or pointers.
 */
#ifndef AC_ALPHA_PROBE_DIGEST_H
#define AC_ALPHA_PROBE_DIGEST_H
#include "rules_text.h"
static uint32_t rules_digest(const alpha::Rules& rules) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < rules.count; ++i) {
    const auto& t = rules.technologies[i];
    const char* strings[] = {t.name,t.key};
    for (const char* s : strings) {
      do { h = (h ^ (uint8_t)*s) * 16777619u; } while (*s++);
    }
    uint32_t words[7] = {(uint32_t)t.priorities[0],(uint32_t)t.priorities[1],
      (uint32_t)t.priorities[2],(uint32_t)t.priorities[3],
      (uint32_t)t.prerequisites[0],(uint32_t)t.prerequisites[1],t.flags};
    for (uint32_t w : words) for (unsigned shift=0; shift<32; shift+=8)
      h = (h ^ ((w >> shift)&255u))*16777619u;
  }
  return h;
}
#endif
