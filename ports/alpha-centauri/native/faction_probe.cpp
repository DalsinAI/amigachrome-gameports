/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules_text.h"
static uint8_t* read_file(const char* path, size_t& size) {
  FILE* f = fopen(path, "rb");
  if (!f) return nullptr;
  if (fseek(f, 0, SEEK_END)) { fclose(f); return nullptr; }
  long length = ftell(f);
  if (length <= 0 || length > 1024*1024 || fseek(f, 0, SEEK_SET)) { fclose(f); return nullptr; }
  uint8_t* data = (uint8_t*)malloc((size_t)length);
  if (!data) { fclose(f); return nullptr; }
  size = fread(data, 1, (size_t)length, f); fclose(f);
  if (size != (size_t)length) { free(data); return nullptr; }
  return data;
}
int main(int argc, char** argv) {
  if (argc < 6 || argc > 7 || (strcmp(argv[1], "base") && strcmp(argv[1], "expansion")) ||
      (argc == 7 && strcmp(argv[6], "localized"))) {
    fprintf(stderr, "usage: faction-probe base|expansion RULES FACTION SEARCH FILENAME [localized]\n"); return 20;
  }
  size_t length = 0;
  uint8_t* bytes = read_file(argv[2], length);
  if (!bytes) { fprintf(stderr, "cannot read rules\n"); return 20; }
  static alpha::Rules technology;
  static alpha::SocialRules social;
  static alpha::Faction faction;
  auto error = alpha::read_technologies(bytes, length, technology);
  if (error == alpha::Error::ok) error = alpha::read_social_rules(bytes, length, technology,
    strcmp(argv[1], "base") ? alpha::Variant::alien_crossfire : alpha::Variant::base, social);
  free(bytes);
  if (error == alpha::Error::ok) {
    bytes = read_file(argv[3], length);
    if (!bytes) { fprintf(stderr, "cannot read faction\n"); return 20; }
    error = alpha::read_faction(bytes, length, argv[4], argv[5], technology, social,
      argc == 7 ? alpha::IdeologyMatch::localized_first_four : alpha::IdeologyMatch::exact, faction);
    free(bytes);
  }
  if (error != alpha::Error::ok) { fprintf(stderr, "faction: %s\n", alpha::error_name(error)); return 20; }
  printf("formal_name_faction=%s\n", faction.formal_name_faction);
  printf("desc_name_faction=%s\n", faction.desc_name_faction);
  printf("noun_faction=%s\n", faction.noun_faction);
  printf("name_leader=%s\n", faction.name_leader);
  printf("adj_name_faction=%s\n", faction.adj_name_faction);
  printf("abbreviation=%s\n", faction.abbreviation);
  printf("assistant_name=%s\n", faction.assistant_name);
  printf("scientist_name=%s\n", faction.scientist_name);
  printf("assistant_city=%s\n", faction.assistant_city);
  printf("title_leader=%s\n", faction.title_leader);
  printf("adj_leader=%s\n", faction.adj_leader);
  printf("adj_insult_leader=%s\n", faction.adj_insult_leader);
  printf("adj_faction=%s\n", faction.adj_faction);
  printf("adj_insult_faction=%s\n", faction.adj_insult_faction);
  printf("insult_leader=%s\n", faction.insult_leader);
  printf("noun_gender=%ld\n", (long)faction.noun_gender);
  printf("is_noun_plural=%ld\n", (long)faction.is_noun_plural);
  printf("is_leader_female=%ld\n", (long)faction.is_leader_female);
  printf("ai_fight=%ld\n", (long)faction.ai_fight);
  printf("ai_power=%ld\n", (long)faction.ai_power);
  printf("ai_tech=%ld\n", (long)faction.ai_tech);
  printf("ai_wealth=%ld\n", (long)faction.ai_wealth);
  printf("ai_growth=%ld\n", (long)faction.ai_growth);
  printf("rule_tech_selected=%ld\n", (long)faction.rule_tech_selected);
  printf("rule_morale=%ld\n", (long)faction.rule_morale);
  printf("rule_research=%ld\n", (long)faction.rule_research);
  printf("rule_drone=%ld\n", (long)faction.rule_drone);
  printf("rule_talent=%ld\n", (long)faction.rule_talent);
  printf("rule_energy=%ld\n", (long)faction.rule_energy);
  printf("rule_interest=%ld\n", (long)faction.rule_interest);
  printf("rule_population=%ld\n", (long)faction.rule_population);
  printf("rule_hurry=%ld\n", (long)faction.rule_hurry);
  printf("rule_techcost=%ld\n", (long)faction.rule_techcost);
  printf("rule_psi=%ld\n", (long)faction.rule_psi);
  printf("rule_sharetech=%ld\n", (long)faction.rule_sharetech);
  printf("rule_commerce=%ld\n", (long)faction.rule_commerce);
  printf("rule_flags=%lu\n", (unsigned long)faction.rule_flags);
  printf("bonus_count=%lu\n", (unsigned long)faction.bonus_count);
  for (size_t i = 0; i < faction.bonus_count; ++i) {
    printf("bonus.%lu.kind=%ld\n", (unsigned long)i, (long)faction.bonuses[i].kind);
    printf("bonus.%lu.value=%ld\n", (unsigned long)i, (long)faction.bonuses[i].value);
    printf("bonus.%lu.modifier=%ld\n", (unsigned long)i, (long)faction.bonuses[i].modifier);
  }
  for (size_t i = 0; i < 2; ++i) {
    printf("ideology.%lu.category=%ld\n", (unsigned long)i, (long)faction.ideology[i].category);
    printf("ideology.%lu.model=%ld\n", (unsigned long)i, (long)faction.ideology[i].model);
    printf("ideology.%lu.effect=%ld\n", (unsigned long)i, (long)faction.ideology[i].effect);
  }
  return 0;
}
