/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules_text.h"
int main(int argc, char** argv) {
  if (argc != 3 || (strcmp(argv[1], "base") && strcmp(argv[1], "expansion"))) {
    fprintf(stderr, "usage: unit-probe base|expansion alpha.txt\n"); return 20;
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
  static alpha::UnitDefinitions units;
  if (read != (size_t)length) { free(data); return 20; }
  auto error = alpha::read_technologies(data, read, tech);
  if (error == alpha::Error::ok)
    error = alpha::read_unit_definitions(data, read, tech,
      strcmp(argv[1], "base") ? alpha::Variant::alien_crossfire : alpha::Variant::base, units);
  free(data);
  if (error != alpha::Error::ok) { fprintf(stderr, "units: %s\n", alpha::error_name(error)); return 20; }
  printf("weapon_count=%lu\narmor_count=%lu\nability_count=%lu\nunit_count=%lu\n",
    (unsigned long)units.weapon_count,(unsigned long)units.armor_count,
    (unsigned long)units.ability_count,(unsigned long)units.unit_count);
  for (size_t i=0;i<9;++i) {
    const auto& c=units.chassis[i];
    for(size_t j=0;j<6;++j) {
      printf("chassis.%lu.noun.%lu.name=%s\n",(unsigned long)i,(unsigned long)j,c.nouns[j].name);
      printf("chassis.%lu.noun.%lu.gender=%ld\n",(unsigned long)i,(unsigned long)j,(long)c.nouns[j].gender);
      printf("chassis.%lu.noun.%lu.plural=%ld\n",(unsigned long)i,(unsigned long)j,(long)c.nouns[j].plural);
    }
    printf("chassis.%lu.speed=%ld\n",(unsigned long)i,(long)c.speed);
    printf("chassis.%lu.triad=%ld\n",(unsigned long)i,(long)c.triad);
    printf("chassis.%lu.range=%ld\n",(unsigned long)i,(long)c.range);
    printf("chassis.%lu.missile=%ld\n",(unsigned long)i,(long)c.missile);
    printf("chassis.%lu.cargo=%ld\n",(unsigned long)i,(long)c.cargo);
    printf("chassis.%lu.cost=%ld\n",(unsigned long)i,(long)c.cost);
    printf("chassis.%lu.prerequisite=%ld\n",(unsigned long)i,(long)c.prerequisite);
  }
  for(size_t i=0;i<4;++i) {
    const auto& row=units.reactors[i];
    printf("reactors.%lu.name=%s\n",(unsigned long)i,row.name);
    printf("reactors.%lu.short_name=%s\n",(unsigned long)i,row.short_name);
    printf("reactors.%lu.power=%ld\n",(unsigned long)i,(long)row.power);
    printf("reactors.%lu.prerequisite=%ld\n",(unsigned long)i,(long)row.prerequisite);
  }
  for(size_t i=0;i<units.weapon_count;++i) {
    const auto& row=units.weapons[i];
    printf("weapons.%lu.name=%s\n",(unsigned long)i,row.name);
    printf("weapons.%lu.short_name=%s\n",(unsigned long)i,row.short_name);
    printf("weapons.%lu.offense=%ld\n",(unsigned long)i,(long)row.offense);
    printf("weapons.%lu.mode=%ld\n",(unsigned long)i,(long)row.mode);
    printf("weapons.%lu.cost=%ld\n",(unsigned long)i,(long)row.cost);
    printf("weapons.%lu.icon=%ld\n",(unsigned long)i,(long)row.icon);
    printf("weapons.%lu.prerequisite=%ld\n",(unsigned long)i,(long)row.prerequisite);
  }
  for(size_t i=0;i<units.armor_count;++i) {
    const auto& row=units.armor[i];
    printf("armor.%lu.name=%s\n",(unsigned long)i,row.name);
    printf("armor.%lu.short_name=%s\n",(unsigned long)i,row.short_name);
    printf("armor.%lu.defense=%ld\n",(unsigned long)i,(long)row.defense);
    printf("armor.%lu.mode=%ld\n",(unsigned long)i,(long)row.mode);
    printf("armor.%lu.cost=%ld\n",(unsigned long)i,(long)row.cost);
    printf("armor.%lu.prerequisite=%ld\n",(unsigned long)i,(long)row.prerequisite);
  }
  for(size_t i=0;i<units.ability_count;++i) {
    const auto& row=units.abilities[i];
    printf("abilities.%lu.name=%s\n",(unsigned long)i,row.name);
    printf("abilities.%lu.cost_factor=%ld\n",(unsigned long)i,(long)row.cost_factor);
    printf("abilities.%lu.prerequisite=%ld\n",(unsigned long)i,(long)row.prerequisite);
    printf("abilities.%lu.abbreviation=%s\n",(unsigned long)i,row.abbreviation);
    printf("abilities.%lu.flags=%lu\n",(unsigned long)i,(unsigned long)row.flags);
    printf("abilities.%lu.description=%s\n",(unsigned long)i,row.description);
  }
  for(size_t i=0;i<units.unit_count;++i) {
    const auto& row=units.units[i];
    printf("units.%lu.name=%s\n",(unsigned long)i,row.name);
    printf("units.%lu.chassis=%ld\n",(unsigned long)i,(long)row.chassis);
    printf("units.%lu.weapon=%ld\n",(unsigned long)i,(long)row.weapon);
    printf("units.%lu.armor=%ld\n",(unsigned long)i,(long)row.armor);
    printf("units.%lu.plan=%ld\n",(unsigned long)i,(long)row.plan);
    printf("units.%lu.cost=%ld\n",(unsigned long)i,(long)row.cost);
    printf("units.%lu.carry=%ld\n",(unsigned long)i,(long)row.carry);
    printf("units.%lu.prerequisite=%ld\n",(unsigned long)i,(long)row.prerequisite);
    printf("units.%lu.icon=%ld\n",(unsigned long)i,(long)row.icon);
    printf("units.%lu.reactor=%ld\n",(unsigned long)i,(long)row.reactor);
    printf("units.%lu.abilities=%lu\n",(unsigned long)i,(unsigned long)row.abilities);
  }
  return 0;
}
