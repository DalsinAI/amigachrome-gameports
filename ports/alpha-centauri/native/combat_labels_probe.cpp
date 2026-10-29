/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_labels.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace alpha;
static bool readword(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void word(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static int32_t signedword(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
int main(){uint32_t h[46];while(readword(h[0])){for(unsigned i=1;i<46;++i)if(!readword(h[i]))return 2;if(h[2]>4000000||h[3]>13)return 2;std::vector<uint8_t> b(h[2]);if(fread(b.data(),1,b.size(),stdin)!=b.size())return 2;const auto copy=b;
 OriginalLabelsView labels;memset(&labels,0xa5,sizeof labels);const auto old=labels;auto error=import_original_labels(b.data(),b.size(),(Variant)h[1],labels);
 if(error!=CombatLabelError::ok&&memcmp(&labels,&old,sizeof labels))return 3;
 if(import_original_labels((const uint8_t*)&labels,sizeof labels,(Variant)h[1],labels)!=CombatLabelError::bad_input)return 4;
 word((uint32_t)error);if(error!=CombatLabelError::ok)continue;
 if(!h[0]){word(labels.count);for(uint32_t i=0;i<labels.count;++i){word(labels.labels[i].length);fwrite(b.data()+labels.labels[i].offset,1,labels.labels[i].length,stdout);}continue;}
 WorldBattleReportingInspection r{};r.modifiers.lookup_count=h[3];for(unsigned i=0;i<13;++i)r.modifiers.lookups[i]={h[7+3*i],signedword(h[8+3*i]),h[9+3*i]};
 for(unsigned i=0;i<r.modifiers.lookup_count;++i){const auto&e=r.modifiers.lookups[i];if(e.appended&&r.modifiers.ledger.count<4)r.modifiers.ledger.slots[r.modifiers.ledger.count++]={e.label,e.percent};}
 r.final_ledger=r.modifiers.ledger;r.entry.hasty.string_lookup_required=h[4];r.entry.hasty.penalty=signedword(h[5]);r.entry.hasty.report_append_required=h[4]&&h[5]&&r.final_ledger.count<4;
 if(r.entry.hasty.report_append_required)r.final_ledger.slots[r.final_ledger.count++]={339,signedword(h[5])};
 r.entry.hasty.report_count=(int32_t)r.final_ledger.count;r.lookup_count=h[3]+h[4];
 if(h[6]==1)++r.lookup_count;
 if(h[6]==2)++r.final_ledger.count;
 if(h[6]==3)r.final_ledger.slots[0].label^=1;
 CombatTextLedger prior;memset(&prior,0x5a,sizeof prior);prior.count=17;CombatTextReporting result;memset(&result,0xa5,sizeof result);const auto before=result;const auto rbefore=r;const auto lbefore=labels;const auto pbefore=prior;
 error=resolve_combat_reporting(labels,(Variant)(h[6]==4?1-h[1]:h[1]),r,prior,result);
 if(error!=CombatLabelError::ok&&memcmp(&result,&before,sizeof result))return 5;
 if(b!=copy||memcmp(&r,&rbefore,sizeof r)||memcmp(&labels,&lbefore,sizeof labels)||memcmp(&prior,&pbefore,sizeof prior))return 6;
 // Known overlapping outputs must be refused without modifying any input.
 if(resolve_combat_reporting(labels,(Variant)h[1],r,prior,*reinterpret_cast<CombatTextReporting*>(&r))!=CombatLabelError::bad_input)return 7;
 if(resolve_combat_reporting(labels,(Variant)h[1],r,prior,*reinterpret_cast<CombatTextReporting*>(&prior))!=CombatLabelError::bad_input)return 8;
 if(resolve_combat_reporting(labels,(Variant)h[1],r,prior,*reinterpret_cast<CombatTextReporting*>(&labels))!=CombatLabelError::bad_input)return 9;
 if(memcmp(&r,&rbefore,sizeof r)||memcmp(&labels,&lbefore,sizeof labels)||memcmp(&prior,&pbefore,sizeof prior))return 10;
 word((uint32_t)error);word(result.ledger.count);for(auto n:result.ledger.percentages)word((uint32_t)n);fwrite(result.ledger.names,1,320,stdout);word(result.resolved_lookups);
 }return ferror(stdin)||ferror(stdout)?11:0;}
