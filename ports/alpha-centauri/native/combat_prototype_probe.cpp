/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_prototype.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static UnitDefinitions units{};static BasicRules rules{};
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static unsigned error(LogicError e){return e==LogicError::ok?0:e==LogicError::overflow?2:1;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char input[100];while(true){const size_t n=fread(input,1,sizeof input,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof input)return 20;int32_t c[25];for(unsigned i=0;i<25;++i)c[i]=word(input+4*i);
 const Variant variant=c[0]==0?Variant::base:c[0]==1?Variant::alien_crossfire:(Variant)42;units.weapon_count=c[0]==0?21:26;units.armor_count=c[0]==0?10:14;units.ability_count=c[0]==0?24:29;
 units.weapons[0].offense=c[3];units.weapons[1].offense=c[4];units.armor[0].defense=c[5];units.armor[1].defense=c[6];units.weapons[0].mode=c[7];units.chassis[0].missile=c[9];units.chassis[0].triad=c[10];units.chassis[1].triad=c[11];
 rules.psi_combat_ratio_atk_land=c[13];rules.psi_combat_ratio_atk_sea=c[14];rules.psi_combat_ratio_atk_air=c[15];rules.psi_combat_ratio_def_land=c[16];rules.psi_combat_ratio_def_sea=c[17];rules.psi_combat_ratio_def_air=c[18];
 OriginalPrototype own{},opponent{};own.chassis_id=own.weapon_id=own.armor_id=0;opponent.chassis_id=opponent.weapon_id=opponent.armor_id=1;opponent.plan=(uint8_t)c[8];
 PrototypeCombatRatings result;memset(&result,0xa5,sizeof result);unsigned char before[sizeof result];memcpy(before,&result,sizeof result);const auto pe=prototype_combat_ratings(variant,units,rules,own,(size_t)c[1],opponent,(size_t)c[2],c[12]!=0,result);unsigned guards=0;
 if(pe!=LogicError::ok){if(memcmp(before,&result,sizeof result))return 21;++guards;}
 if(prototype_combat_ratings(variant,units,rules,own,(size_t)c[1],opponent,(size_t)c[2],c[12]!=0,alias<PrototypeCombatRatings>((uintptr_t)&units))!=LogicError::bad_state)return 22;
 ++guards;
 if(prototype_combat_ratings(variant,units,rules,own,(size_t)c[1],opponent,(size_t)c[2],c[12]!=0,alias<PrototypeCombatRatings>((uintptr_t)&own))!=LogicError::bad_state)return 23;
 ++guards;
 int32_t factor=0x12345678;const auto fe=combat_psi_factor(variant,c[19],c[20],c[23]!=0,c[24]!=0,c[21]!=0,c[22]!=0,factor);if(fe!=LogicError::ok){if(factor!=0x12345678)return 24;++guards;}
 const uint32_t flags=pe==LogicError::ok?((result.offense_psi?1u:0u)|(result.defense_psi?2u:0u)|(result.probe_duel?4u:0u)):0;
 if(!emit(error(pe))||!emit(pe==LogicError::ok?(uint32_t)result.offense:0)||!emit(pe==LogicError::ok?(uint32_t)result.defense:0)||!emit(flags)||!emit(error(fe))||!emit((uint32_t)factor)||!emit(guards))return 25;
 }}
