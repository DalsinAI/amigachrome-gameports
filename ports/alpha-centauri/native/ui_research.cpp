/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_research.h"
#include <stdio.h>
namespace alpha {
static const uint8_t fill[3]={7,19,22},edge[3]={92,128,112},cursor[3]={36,64,58};
size_t render_research_panel(const ResearchPanelModel& m,const UiFrame& f,const UiRect& r,UiTextItem* items,size_t capacity){
 size_t n=0;if(!m.valid || !ui_frame_valid(f) || r.w<360 || r.h<260)return 0;
 ui_panel(f,r,fill,edge);char line[96];
 ui_text_item(items,capacity,n,r.x+16,r.y+28,r.w-32,3,true,"RESEARCH");
 snprintf(line,sizeof(line),"Goal: %.40s",m.current>=0?m.current_name:"(none chosen)");ui_text_item(items,capacity,n,r.x+16,r.y+56,r.w-32,2,false,line);
 snprintf(line,sizeof(line),"Progress %d / %d   Labs %d   Technologies known %d",m.accumulated,m.cost,m.labs,m.known);ui_text_item(items,capacity,n,r.x+16,r.y+80,r.w-32,0,false,line);
 const int32_t values[3]={m.accumulated,m.cost>m.accumulated?m.cost-m.accumulated:0,0};const uint8_t colours[3][3]={{64,160,220},{30,48,60},{0,0,0}};
 ui_bar(f,{r.x+16,r.y+88,r.w-32,8},values,colours,m.cost>0?m.cost:1);
 ui_text_item(items,capacity,n,r.x+16,r.y+120,r.w-32,1,false,m.count?"Available technologies (8/2 select, ENTER research, ESC close)":"No technology is available to research.");
 const int line_h=20,rows=(r.h-150)/line_h;int first=m.selected-rows+1;if(first<0)first=0;
 for(int i=first;i<m.count && i<first+rows;++i){const int y=r.y+132+(i-first)*line_h;
  if(i==m.selected)ui_fill(f,{r.x+12,y,r.w-24,line_h},cursor);
  snprintf(line,sizeof(line),"%s%.40s  (%.10s)",m.ids[i]==m.current?"* ":"  ",m.names[i],m.categories[i]);
  ui_text_item(items,capacity,n,r.x+20,y+15,r.w-40,i==m.selected?2:0,false,line);}
 if(m.confirm)ui_text_item(items,capacity,n,r.x+16,r.y+r.h-12,r.w-32,5,false,"Changing research halves the progress. ENTER again confirms.");
 return n;
}
int research_panel_key(ResearchPanelModel& m,int key){
 switch(key){
 case UIK_UP:ui_list_step(m.selected,m.count,-1);m.confirm=false;return UIA_MOVED;
 case UIK_DOWN:ui_list_step(m.selected,m.count,1);m.confirm=false;return UIA_MOVED;
 case UIK_ENTER:return m.count?UIA_APPLY:UIA_NONE;
 case UIK_BACK:return UIA_CLOSE;
 default:return UIA_NONE;}
}
}
