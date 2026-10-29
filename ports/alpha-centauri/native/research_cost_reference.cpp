/* SPDX-License-Identifier: GPL-3.0-or-later
 * Verbatim complete OpenSMACX technology.cpp::tech_rate005BE6B0 body,
 * Brendan Casey(C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Test-only globals replace absolute addresses; tech_cost forced-1 to exercise
 * the actual fresh-cost branch. Native/host control, never gameplay authority.
 */
#include "research_progress.h"
#ifndef __cdecl
#define __cdecl
#endif
namespace {
constexpr uint32_t MaxPlayerNum=8,DLVL_LIBRARIAN=3,RULES_TECH_STAGNATION=0x20;
using BOOL=int;
struct Social {int32_t research;};
struct PlayerData {int32_t tech_cost,earned_techs_saved,unk_26,tech_ranking,diff_level;Social soc_effect_base;};
struct Player {int32_t rule_techcost;};
struct RuleData {int32_t tech_discovery_rate_pct_std;};
static PlayerData PlayersData[8];static Player Players[8];static RuleData rule_data;static RuleData*Rules=&rule_data;
static int32_t difficulty,turn,map_root,game_rules;static int32_t*DiffLevelCurrent=&difficulty,*TurnCurrentNum=&turn,*MapAreaSqRoot=&map_root,*GameRules=&game_rules;static uint32_t human;
int range(int input,int min,int max){return input<min?min:input>max?max:input;}
BOOL is_human(uint32_t faction){return faction==human;}
uint32_t __cdecl tech_rate(uint32_t faction_id) {
    if (PlayersData[faction_id].tech_cost >= 0) {
        return PlayersData[faction_id].tech_cost; // already set
    }
    if (!Rules->tech_discovery_rate_pct_std) {
        return 999999999; // max cost
    }
    uint32_t player_factor = range(PlayersData[faction_id].earned_techs_saved * 2
        - PlayersData[faction_id].unk_26 + PlayersData[faction_id].tech_ranking, 2, 9999);
    uint32_t top_factor = 0;
    for (uint32_t i = 1; i < MaxPlayerNum; i++) {
        uint32_t compare = PlayersData[i].earned_techs_saved * 2 + PlayersData[i].tech_ranking;
        if (compare > top_factor) {
            top_factor = compare;
        }
    }
    player_factor /= 2;
    top_factor /= 2;
    BOOL is_human_player = is_human(faction_id);
    uint32_t diff_factor = is_human_player ? PlayersData[faction_id].diff_level : *DiffLevelCurrent;
    diff_factor += (diff_factor < DLVL_LIBRARIAN);
    uint32_t diff_lvl = !is_human_player ? *DiffLevelCurrent : DLVL_LIBRARIAN;
    diff_factor = is_human_player ? diff_factor * 4 + 8 : 29 - diff_factor * 3;
    diff_factor = range(diff_factor, 12 - player_factor, player_factor + 12);
    uint32_t tech_stagnation = *GameRules & RULES_TECH_STAGNATION;
    uint32_t rule_factor = tech_stagnation | 0x40; // 64 or 96
    uint32_t fin_factor = range(player_factor - (*TurnCurrentNum / (rule_factor >> 3)),
        0, (diff_factor * (rule_factor >> 5)) >> 1) + diff_factor;
    int resch_base = range(PlayersData[faction_id].soc_effect_base.research, -1, 1);
    uint32_t discovery_rate = (fin_factor
        - range((top_factor - diff_lvl - player_factor + 7) / (8 - diff_lvl),
            0, diff_lvl * fin_factor / 10 + 1))
        * range(player_factor - resch_base, 1, 99999);
    if (Rules->tech_discovery_rate_pct_std != 100) {
        discovery_rate = 100 * discovery_rate / Rules->tech_discovery_rate_pct_std;
    }
    if (Players[faction_id].rule_techcost != 100) {
        discovery_rate = discovery_rate * Players[faction_id].rule_techcost / 100;
    }
    uint32_t cost = (discovery_rate * *MapAreaSqRoot) / 56;
    if (tech_stagnation) {
        cost += cost / 2; // Slower Rate of Research Discoveries
    }
    return range(cost, 1, 99999999);
}

}
namespace alpha {
int32_t original_research_cost_reference(const OriginalResearchCostFacts&f){
 for(unsigned i=0;i<8;++i){PlayersData[i]={-1,f.deferred_techs[i],i==(unsigned)f.faction?f.ranking_debt:0,f.ranking[i],f.difficulty,{f.research_base}};Players[i].rule_techcost=f.rule_techcost;}
 human=(uint32_t)f.faction;difficulty=f.difficulty;turn=f.current_turn;game_rules=f.stagnation?0x20:0;rule_data.tech_discovery_rate_pct_std=f.discovery_rate_pct;map_root=f.map_tiles;
 if(map_root>1){int next=map_root>>1,current;do{current=next;next=(next+map_root/next)>>1;}while(next<current);map_root=current;}
 return(int32_t)tech_rate((uint32_t)f.faction);
}
}
