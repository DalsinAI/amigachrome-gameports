/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_diplowin.h"
#include <stdio.h>
namespace alpha {
static const uint8_t fill[3]={8,12,26},edge[3]={96,112,170},cursor[3]={36,44,80};
size_t render_diplo_panel(const DiploPanelModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 size_t n=0;if(!m.valid || !ui_frame_valid(f) || r.w<420 || r.h<260)return 0;
 ui_panel(f,r,fill,edge);char line[96];const int w=r.w-32;
 ui_text_item(items,capacity,n,r.x+16,r.y+28,w,3,true,"COMM LINK");
 snprintf(line,sizeof(line),"%.47s",m.leader);ui_text_item(items,capacity,n,r.x+16,r.y+54,w,2,false,line);
 snprintf(line,sizeof(line),"%.39s   Mood: %.23s",m.other_name,m.mood);ui_text_item(items,capacity,n,r.x+16,r.y+76,w,0,false,line);
 snprintf(line,sizeof(line),"Relations: %.47s",m.status);ui_text_item(items,capacity,n,r.x+16,r.y+96,w,0,false,line);
 if(m.reply[0]){snprintf(line,sizeof(line),"\"%.90s\"",m.reply);ui_text_item(items,capacity,n,r.x+16,r.y+122,w,1,false,line);}
 for(int i=0;i<m.count && i<16;++i){const int y=r.y+138+i*20;if(y>r.y+r.h-36)break;
  if(i==m.selected)ui_fill(f,{r.x+12,y,r.w-24,20},cursor);
  ui_text_item(items,capacity,n,r.x+20,y+15,w-8,i==m.selected?2:0,false,m.labels[i]);}
 if(!m.count)ui_text_item(items,capacity,n,r.x+16,r.y+150,w,0,false,"Nothing can be proposed now.");
 ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,w,0,false,"8/2 choose, ENTER propose, ESC close the comm link");
 return n;
}
int diplo_panel_key(DiploPanelModel& m,int key){
 switch(key){
 case UIK_UP:ui_list_step(m.selected,m.count,-1);return UIA_MOVED;
 case UIK_DOWN:ui_list_step(m.selected,m.count,1);return UIA_MOVED;
 case UIK_ENTER:return m.count?UIA_APPLY:UIA_NONE;
 case UIK_BACK:return UIA_CLOSE;
 default:return UIA_NONE;}
}
}
