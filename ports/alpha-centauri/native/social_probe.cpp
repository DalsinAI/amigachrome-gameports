/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules_text.h"
int main(int argc, char** argv) {
  if (argc != 3 || (strcmp(argv[1], "base") && strcmp(argv[1], "expansion"))) {
    fprintf(stderr, "usage: social-probe base|expansion alpha.txt\n"); return 20;
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
  static alpha::SocialRules social;
  if (read != (size_t)length) { free(data); return 20; }
  auto error = alpha::read_technologies(data, read, tech);
  if (error == alpha::Error::ok)
    error = alpha::read_social_rules(data, read, tech,
      strcmp(argv[1], "base") ? alpha::Variant::alien_crossfire : alpha::Variant::base, social);
  free(data);
  if (error != alpha::Error::ok) { fprintf(stderr, "rules: %s\n", alpha::error_name(error)); return 20; }
  for (size_t i = 0; i < alpha::social_effect_count; ++i) {
    printf("effect.%lu.key=%s\n", (unsigned long)i, social.effect_keys[i]);
    printf("effect.%lu.label=%s\n", (unsigned long)i, social.effect_labels[i]);
  }
  for (size_t c = 0; c < alpha::social_category_count; ++c) {
    printf("category.%lu=%s\n", (unsigned long)c, social.categories[c]);
    for (size_t m = 0; m < alpha::social_model_count; ++m) {
      const auto& model = social.models[c][m];
      printf("model.%lu.%lu.name=%s\n", (unsigned long)c, (unsigned long)m, model.name);
      printf("model.%lu.%lu.tech=%ld\n", (unsigned long)c, (unsigned long)m, (long)model.prerequisite);
      for (size_t e = 0; e < alpha::social_effect_count; ++e)
        printf("model.%lu.%lu.effect.%lu=%ld\n", (unsigned long)c, (unsigned long)m, (unsigned long)e, (long)model.effects[e]);
    }
  }
  printf("bonus.count=%lu\n", (unsigned long)social.bonus_count);
  for (size_t i = 0; i < social.bonus_count; ++i)
    printf("bonus.%lu=%s\n", (unsigned long)i, social.bonus_keys[i]);
  // Exercise lookup boundaries/case folding independently of parsing output.
  if (alpha::social_effect(social, social.effect_keys[10]) != 10 ||
      alpha::social_category(social, social.categories[3]) != 3 ||
      alpha::social_model(social, 3, social.models[3][3].name) != 3 ||
      alpha::bonus_kind(social, social.bonus_keys[social.bonus_count-1]) != (int)social.bonus_count-1 ||
      alpha::social_model(social, 4, "missing") != -1 ||
      alpha::social_effect(social, nullptr) != -1 || alpha::bonus_kind(social, "unknown-not-a-key") != -1) return 20;
  return 0;
}
