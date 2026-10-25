/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TECHNOLOGY_BROWSER_H
#define ALPHA_TECHNOLOGY_BROWSER_H
#include "rules_text.h"
namespace alpha {
/* Browses parsed original rules. It never grants technology or advances research. */
struct TechnologyTree {
 int16_t depth[technology_count];
 uint16_t order[technology_count];
 size_t count;
 bool build(const Rules&);
};
/* Original text file #TECH<n> paragraphs; owned byte-preserving output.
 * False for missing section, embedded NUL, overlong output or invalid index.
 */
bool technology_description(const uint8_t*,size_t,int,char*,size_t);
}
/* Separate native UI entry, callable from both front end and original world view. */
int alpha_technology_browser(const char* rules_path);
#endif
