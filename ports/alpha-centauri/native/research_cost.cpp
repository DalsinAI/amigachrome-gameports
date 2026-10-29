/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned bounded interpretation of original tech_rate, described in
 * OpenSMACX technology.cpp (C)2013-2021 Brendan Casey, pin2f7f4e24.
 * Recovered signed arithmetic checked privately against original instructions;
 * no proprietary code/table copied. See RESEARCH-COST.md.
 */
#include "research_cost.h"
namespace alpha {
static int32_t clamp(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
static bool narrow(int64_t n,int32_t& out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
LogicError original_research_cost(const ResearchCostContext& c,ResearchCostResult& output){
 if(c.faction<0 || c.faction>=8)return LogicError::bad_state;
 const auto& f=c.factions[c.faction];
 if(f.cached_cost>=0){output={f.cached_cost,true,false};return LogicError::ok;}
 if(!c.discovery_rate){output={999999999,false,true};return LogicError::ok;}
 if(c.discovery_rate<0 || c.discovery_rate>1000 || !c.map_area_sqrt_known || c.map_area_sqrt<0)return LogicError::bad_state;
 const bool human=(c.human_mask&(1u<<c.faction))!=0;
 if((!human && (c.global_difficulty<0 || c.global_difficulty>5)) || (human && (f.difficulty<0 || f.difficulty>5)))return LogicError::bad_state;
 int32_t player,twice,top=0;
 if(!narrow((int64_t)f.earned*2,twice) || !narrow((int64_t)twice-f.ranking_adjustment,player) || !narrow((int64_t)player+f.ranking,player))return LogicError::overflow;
 player=clamp(player,2,9999)/2;
 for(unsigned i=1;i<8;++i){int32_t candidate;if(!narrow((int64_t)c.factions[i].earned*2,twice) || !narrow((int64_t)twice+c.factions[i].ranking,candidate))return LogicError::overflow;if(candidate>top)top=candidate;}
 top/=2;
 int32_t difficulty=human?f.difficulty:c.global_difficulty;difficulty+=difficulty<3;
 const int32_t level=human?3:c.global_difficulty;
 const int32_t factor=clamp(human?difficulty*4+8:29-difficulty*3,12-player,player+12);
 const int32_t penalty=clamp(player-c.turn/(c.stagnation?12:8),0,factor*(c.stagnation?3:2)/2);
 const int32_t adjusted=factor+penalty;
 const int32_t reduction=clamp((top-level-player+7)/(8-level),0,level*adjusted/10+1);
 const int32_t multiplier=clamp(player-clamp(f.research_base,-1,1),1,99999);
 int32_t cost;if(!narrow((int64_t)multiplier*(adjusted-reduction),cost))return LogicError::overflow;
 if(c.discovery_rate!=100){if(!narrow((int64_t)cost*100,cost))return LogicError::overflow;cost/=c.discovery_rate;}
 if(f.rule_techcost!=100){if(!narrow((int64_t)f.rule_techcost*cost,cost))return LogicError::overflow;cost/=100;}
 if(!narrow((int64_t)c.map_area_sqrt*cost,cost))return LogicError::overflow;
 cost/=56;
 if(c.stagnation && !narrow((int64_t)cost+cost/2,cost))return LogicError::overflow;
 output={clamp(cost,1,99999999),false,false};return LogicError::ok;
}
LogicError original_research_map_root(int32_t input,int32_t& output){
 if(input<0)return LogicError::bad_state;
 if(input<=1){output=input;return LogicError::ok;}
 int32_t next=input/2,root;
 do{root=next;next=(next+input/next)/2;}while(next<root);
 output=root;return LogicError::ok;
}
LogicError world_research_cost(const OriginalWorld& world,const BasicRules& rules,int32_t faction,ResearchCostResult& output){
 if(!world.loaded() || faction<0 || faction>=8)return LogicError::bad_state;
 if(world.game().map.info.tile_count>2147483647u)return LogicError::overflow;
 int32_t root;auto error=original_research_map_root((int32_t)world.game().map.info.tile_count,root);if(error!=LogicError::ok)return error;
 ResearchCostContext c{};c.faction=faction;c.discovery_rate=rules.tech_discovery_rate_pct_std;c.turn=world.game().preferences.CurrentTurn;c.global_difficulty=world.game().preferences.DiffLevel;c.human_mask=world.game().preferences.FactionStatus[0];c.stagnation=(world.game().preferences.GameRules&0x20)!=0;c.map_area_sqrt=root;c.map_area_sqrt_known=true;
 for(unsigned i=0;i<8;++i){const auto* f=world.faction(i);const auto* d=world.faction_definition(i);if(!f||!d)return LogicError::bad_state;c.factions[i]={f->tech_cost,f->tech_ranking,f->earned_techs_saved,f->unk_27,f->diff_level,f->SE_research_base,d->rule_techcost};}
 return original_research_cost(c,output);
}
}
