/* SPDX-License-Identifier: GPL-3.0-or-later
 * OpenSMACX alpha.cpp, Brendan Casey, plus original edition loader inspection.
 */
#ifndef ALPHA_CITIZENS_TEXT_H
#define ALPHA_CITIZENS_TEXT_H
#include "rules_text.h"
namespace alpha {
constexpr size_t citizen_definition_count=10, specialist_definition_count=7;
struct CitizenDefinition {
 char singular[128],plural[128];
 int32_t prerequisite,obsolete,economy,psych,labs;
};
struct CitizenDefinitions {
 CitizenDefinition rows[citizen_definition_count];
 size_t count;
 Variant variant;
};
/* Requires qualified technologies from the same input. Ten ordinal rows;
 * only the first seven consume technology/bonus fields. Unused trailing
 * columns are ignored. Names use literal comma splitting (semicolon retained).
 * Strict checked numeric syntax replaces the original permissive conversion.
 * On non-alias errors count is zero; other partial output is unqualified. Input/output
 * overlap rejects before writes. Last three rows' numeric fields are owned
 * defaults (-2/-2/0/0/0), not values assigned by the original loader.
 * No availability, specialist selection, World, accounting or turn operation.
 */
Error read_citizen_definitions(const uint8_t*,size_t,const Rules&,Variant,CitizenDefinitions&);
}
#endif
