/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototype.h"
int main(int argc, char** argv) {
  if (argc != 3 || (strcmp(argv[1], "base") && strcmp(argv[1], "expansion"))) {
    fprintf(stderr, "usage: prototype-probe base|expansion alpha.txt\n"); return 20;
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
  alpha::BasicRules basic;
  alpha::WorldRules world;
  if (read != (size_t)length) { free(data); return 20; }
  const auto variant=strcmp(argv[1], "base") ? alpha::Variant::alien_crossfire : alpha::Variant::base;
  auto error=alpha::read_technologies(data,read,tech);
  if(error==alpha::Error::ok)error=alpha::read_basic_rules(data,read,tech,variant,basic,world);
  if(error==alpha::Error::ok)error=alpha::read_unit_definitions(data,read,tech,variant,units);
  free(data);
  if(error!=alpha::Error::ok){fprintf(stderr,"rules: %s\n",alpha::error_name(error));return 20;}
  for(size_t i=0;i<units.unit_count;++i){
    alpha::PredefinedPrototype result;
    auto status=alpha::derive_predefined_prototype(units,i,basic.move_rate_roads,result);
    if(status!=alpha::LogicError::ok){fprintf(stderr,"prototype %lu: %s\n",(unsigned long)i,status==alpha::LogicError::overflow?"overflow":"bad state");return 20;}
    printf("prototype.%lu.name=%s\n",(unsigned long)i,result.name);
    printf("prototype.%lu.chassis=%ld\n",(unsigned long)i,(long)result.chassis);
    printf("prototype.%lu.weapon=%ld\n",(unsigned long)i,(long)result.weapon);
    printf("prototype.%lu.armor=%ld\n",(unsigned long)i,(long)result.armor);
    printf("prototype.%lu.reactor=%ld\n",(unsigned long)i,(long)result.reactor);
    printf("prototype.%lu.prerequisite=%ld\n",(unsigned long)i,(long)result.prerequisite);
    printf("prototype.%lu.abilities=%lu\n",(unsigned long)i,(unsigned long)result.abilities);
    printf("prototype.%lu.cost=%ld\n",(unsigned long)i,(long)result.cost);
    printf("prototype.%lu.carry=%ld\n",(unsigned long)i,(long)result.carry);
    printf("prototype.%lu.plan=%ld\n",(unsigned long)i,(long)result.plan);
    printf("prototype.%lu.role_code=%ld\n",(unsigned long)i,(long)result.role_code);
    printf("prototype.%lu.icon=%ld\n",(unsigned long)i,(long)result.icon);
    printf("prototype.%lu.obsolete_factions=%ld\n",(unsigned long)i,(long)result.obsolete_factions);
    printf("prototype.%lu.combat_factions=%ld\n",(unsigned long)i,(long)result.combat_factions);
    printf("prototype.%lu.flags=%lu\n",(unsigned long)i,(unsigned long)result.flags);
    printf("prototype.%lu.speed_points=%lu\n",(unsigned long)i,(unsigned long)result.speed_points);
  }
  return 0;
}
