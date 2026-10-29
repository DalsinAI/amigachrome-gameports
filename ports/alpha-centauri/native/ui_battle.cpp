/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_battle.h"
#include <stdio.h>
namespace alpha {
static const uint8_t fill[3]={22,10,8},edge[3]={160,96,64};
size_t render_battle_panel(const BattlePanelModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 size_t n=0;if(!m.valid || !ui_frame_valid(f) || r.w<420 || r.h<220)return 0;
 ui_panel(f,r,fill,edge);char line[96];const int w=r.w-32;
 ui_text_item(items,capacity,n,r.x+16,r.y+28,w,3,true,m.preview?"ATTACK?":"BATTLE RESULT");
 snprintf(line,sizeof(line),"%.31s (%.23s)",m.attacker,m.attacker_owner);ui_text_item(items,capacity,n,r.x+16,r.y+56,w,2,false,line);
 snprintf(line,sizeof(line),"vs %.31s (%.23s) at %d, %d",m.defender,m.defender_owner,m.x,m.y);ui_text_item(items,capacity,n,r.x+16,r.y+78,w,2,false,line);
 if(m.preview){
  if(m.odds>0){snprintf(line,sizeof(line),"Odds %d.%d : 1",m.odds/8,(m.odds%8)*10/8);ui_text_item(items,capacity,n,r.x+16,r.y+110,w,m.odds<8?5:0,true,line);}
  else ui_text_item(items,capacity,n,r.x+16,r.y+110,w,5,false,"No attack is possible here.");
  ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,w,0,false,m.odds>0?"ENTER attacks, ESC cancels":"ESC closes");
  return n;}
 ui_text_item(items,capacity,n,r.x+16,r.y+108,w,m.outcome==4 || m.outcome==8 || m.outcome==11?2:5,true,m.outcome_text);
 snprintf(line,sizeof(line),"Strength %d vs %d   Rounds %d (hits %d / %d)",m.offense,m.defense,m.rounds,m.attacker_hits,m.defender_hits);ui_text_item(items,capacity,n,r.x+16,r.y+132,w,0,false,line);
 snprintf(line,sizeof(line),"Damage: attacker %d -> %d, defender %d -> %d",m.hp_atk_before,m.hp_atk_after,m.hp_def_before,m.hp_def_after);ui_text_item(items,capacity,n,r.x+16,r.y+152,w,0,false,line);
 int y=r.y+176;
 for(int side=0;side<2;++side)for(int i=0;i<m.mod_count[side] && i<4 && y<r.y+r.h-28;++i,y+=18){
  snprintf(line,sizeof(line),"%s %.46s",side?"Def:":"Att:",m.mods[side][i]);ui_text_item(items,capacity,n,r.x+16,y,w,1,false,line);}
 if(m.killed){snprintf(line,sizeof(line),"Units destroyed: %d%s%s",m.killed,m.promoted_atk?"  attacker promoted":"",m.base_captured?"  base captured":m.base_destroyed?"  base destroyed":"");ui_text_item(items,capacity,n,r.x+16,y,w,0,false,line);}
 ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,w,0,false,"ENTER or ESC closes");
 return n;
}
int battle_panel_key(const BattlePanelModel& m,int key){
 if(key==UIK_BACK)return UIA_CLOSE;
 if(key==UIK_ENTER)return m.preview && m.odds>0?UIA_APPLY:UIA_CLOSE;
 return UIA_NONE;
}
}
