/* SPDX-License-Identifier: GPL-3.0-or-later. See citizens_text.h. */
#include "citizens_text.h"
namespace alpha {
static unsigned char lower(unsigned char c) {
 return c>='A' && c<='Z' ? (unsigned char)(c+'a'-'A') : c;
}
static bool equal(const char* a,const char* b) {
 while (*a && lower((unsigned char)*a)==lower((unsigned char)*b)) { ++a; ++b; }
 return lower((unsigned char)*a)==lower((unsigned char)*b);
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
static bool overlap(const void* a,size_t n,const void* b,size_t m) {
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if (n>UINTPTR_MAX-x || m>UINTPTR_MAX-y) return true;
 return n && m && (x<=y ? y-x<n : x-y<m);
}
static bool row(Text& text) {
 while (text.next()) if (*text.line() && *text.line()!=';') return true;
 return false;
}
static Error technology(Text& text,const Rules& rules,int32_t& value) {
 char key[8];
 if (!text.item_literal(key,sizeof key) || !*key) return Error::bad_key;
 if (equal(key,"None")) { value=-1; return Error::ok; }
 if (equal(key,"Disable")) { value=-2; return Error::ok; }
 for (size_t i=0;i<technology_count;++i) if (equal(key,rules.technologies[i].key)) {
  value=(int32_t)i; return Error::ok;
 }
 return Error::unknown_prerequisite;
}
Error read_citizen_definitions(const uint8_t* bytes,size_t length,const Rules& rules,
 Variant variant,CitizenDefinitions& output) {
 if (overlap(bytes,length,&output,sizeof output) || overlap(&rules,sizeof rules,&output,sizeof output)) return Error::bad_row;
 output=CitizenDefinitions{};
 if (!bytes || !length) return Error::missing_section;
 if (rules.count!=technology_count) return Error::wrong_count;
 if (variant!=Variant::base && variant!=Variant::alien_crossfire) return Error::bad_row;
 Text text(bytes,length);
 if (!text.section("CITIZENS")) return text.error();
 for (size_t i=0;i<citizen_definition_count;++i) {
  if (!row(text) || *text.line()=='#') return text.error()==Error::ok ? Error::wrong_count : text.error();
  auto& out=output.rows[i]; out.prerequisite=out.obsolete=-2;
  if (!text.item_literal(out.singular,sizeof out.singular) || !text.item_literal(out.plural,sizeof out.plural)) return Error::bad_row;
  if (i<specialist_definition_count) {
   auto error=technology(text,rules,out.prerequisite); if (error!=Error::ok) return error;
   error=technology(text,rules,out.obsolete); if (error!=Error::ok) return error;
   int32_t* fields[]={&out.economy,&out.psych,&out.labs};
   for (auto field:fields) { char token[128]; if (!text.item_literal(token,sizeof token) || !number(token,*field)) return Error::bad_number; }
  }
 }
 if (row(text) && *text.line()!='#') return Error::wrong_count;
 if (text.error()!=Error::ok) return text.error();
 output.count=citizen_definition_count; output.variant=variant; return Error::ok;
}
}
