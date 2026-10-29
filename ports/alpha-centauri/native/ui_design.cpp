/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_design.h"
#include <stdio.h>
namespace alpha {
static const uint8_t fill[3]={7,19,22},edge[3]={92,128,112},cursor[3]={36,64,58};
static const char* const row_names[DESIGN_ROWS]={"Chassis","Weapon","Armor","Reactor","Ability"};
static int pick(const DesignPanelModel& m,int row){const int s=m.selected[row];return s>=0 && s<m.counts[row]?m.choices[row][s].id:-1;}
void design_selection(const DesignPanelModel& m,int& chassis,int& weapon,int& armor,int& reactor,int& abilities){
 chassis=pick(m,DESIGN_CHASSIS);weapon=pick(m,DESIGN_WEAPON);armor=pick(m,DESIGN_ARMOR);reactor=pick(m,DESIGN_REACTOR);
 const int a=pick(m,DESIGN_ABILITY);abilities=a>=0 && a<32?(int)(1u<<a):0;
}
size_t render_design_panel(const DesignPanelModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 size_t n=0;if(!m.valid || !ui_frame_valid(f) || r.w<420 || r.h<260)return 0;
 ui_panel(f,r,fill,edge);char line[96];
 ui_text_item(items,capacity,n,r.x+16,r.y+28,r.w-32,3,true,"DESIGN WORKSHOP");
 for(int i=0;i<DESIGN_ROWS;++i){const int y=r.y+48+i*30;
  if(i==m.row)ui_fill(f,{r.x+12,y,r.w-24,26},cursor);
  ui_text_item(items,capacity,n,r.x+20,y+18,100,1,false,row_names[i]);
  const int s=m.selected[i];
  snprintf(line,sizeof(line),"< %.30s >  (%d of %d)",s>=0 && s<m.counts[i]?m.choices[i][s].name:"none",m.counts[i]?s+1:0,m.counts[i]);
  ui_text_item(items,capacity,n,r.x+130,y+18,r.w-150,i==m.row?2:0,false,line);}
 const int y=r.y+48+DESIGN_ROWS*30+20;
 if(m.preview_ok){snprintf(line,sizeof(line),"%.39s",m.preview_name);ui_text_item(items,capacity,n,r.x+16,y,r.w-32,2,true,line);
  snprintf(line,sizeof(line),"Cost %d minerals",m.preview_cost);ui_text_item(items,capacity,n,r.x+16,y+24,r.w-32,0,false,line);}
 else ui_text_item(items,capacity,n,r.x+16,y,r.w-32,5,false,"These components cannot be combined.");
 ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,r.w-32,0,false,"8/2 row, 4/6 component, ENTER create prototype, ESC close");
 return n;
}
int design_panel_key(DesignPanelModel& m,int key){
 switch(key){
 case UIK_UP:ui_list_step(m.row,DESIGN_ROWS,-1);return UIA_NONE;
 case UIK_DOWN:ui_list_step(m.row,DESIGN_ROWS,1);return UIA_NONE;
 case UIK_LEFT:ui_list_step(m.selected[m.row],m.counts[m.row],-1);return UIA_MOVED;
 case UIK_RIGHT:ui_list_step(m.selected[m.row],m.counts[m.row],1);return UIA_MOVED;
 case UIK_ENTER:return m.preview_ok?UIA_APPLY:UIA_NONE;
 case UIK_BACK:return UIA_CLOSE;
 default:return UIA_NONE;}
}
}
