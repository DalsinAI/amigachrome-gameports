/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules_text.h"
int main(int argc, char** argv) {
  if (argc != 3 || (strcmp(argv[1], "base") && strcmp(argv[1], "expansion"))) {
    fprintf(stderr, "usage: basic-probe base|expansion alpha.txt\n"); return 20;
  }
  FILE* file = fopen(argv[2], "rb");
  if (!file) { fprintf(stderr, "cannot open rules file\n"); return 20; }
  if (fseek(file, 0, SEEK_END)) { fclose(file); return 20; }
  long length = ftell(file);
  if (length <= 0 || length > 1024*1024 || fseek(file, 0, SEEK_SET)) { fclose(file); return 20; }
  uint8_t* data = (uint8_t*)malloc((size_t)length);
  if (!data) { fclose(file); return 20; }
  size_t read = fread(data, 1, (size_t)length, file); fclose(file);
  static alpha::Rules tech;
  alpha::BasicRules basic;
  alpha::WorldRules world;
  if (read != (size_t)length) { free(data); return 20; }
  auto error = alpha::read_technologies(data, read, tech);
  if (error == alpha::Error::ok)
    error = alpha::read_basic_rules(data, read, tech,
      strcmp(argv[1], "base") ? alpha::Variant::alien_crossfire : alpha::Variant::base, basic, world);
  free(data);
  if (error != alpha::Error::ok) { fprintf(stderr, "rules: %s\n", alpha::error_name(error)); return 20; }
  // Print every named value, avoiding struct-layout-dependent fingerprints.
#define RULE_ROW(name, low, high) printf("basic." #name "=%ld\n", (long)basic.name);
#define RULE_ITEM(name, low, high) RULE_ROW(name, low, high)
#define RULE_TECH(name) RULE_ROW(name, 0, 0)
#define ALPHA_EXPANSION_FIELDS
#include "basic_fields.inc"
#undef ALPHA_EXPANSION_FIELDS
#undef RULE_ROW
#undef RULE_ITEM
#undef RULE_TECH
#define RULE_ROW(name, low, high) printf("world." #name "=%ld\n", (long)world.name);
#define RULE_ITEM(name, low, high) RULE_ROW(name, low, high)
#define RULE_TECH(name) RULE_ROW(name, 0, 0)
#include "world_fields.inc"
#undef RULE_ROW
#undef RULE_ITEM
#undef RULE_TECH
  return 0;
}
